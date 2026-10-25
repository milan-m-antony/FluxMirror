#ifdef _WIN32
#include "titlebar_camera.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <dwmapi.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_system.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <process.h>

#include "icon.h"
#include "util/log.h"

#ifndef WM_GETTITLEBARINFOEX
# define WM_GETTITLEBARINFOEX 0x033F
#endif

#ifndef DWMWA_USE_IMMERSIVE_DARK_MODE
# define DWMWA_USE_IMMERSIVE_DARK_MODE 20
#endif

#define TOAST_TIMER_ID 1001
#define TOAST_DURATION_MS 2200
#define SYNC_TIMER_ID 1002
#define WM_SHOW_TOAST (WM_USER + 101)

typedef struct {
    DWORD cbSize;
    RECT rcTitleBar;
    DWORD rgstate[6];
    RECT rgrect[6];
} TITLEBARINFOEX_LOCAL;

typedef struct {
    char serial[64];
    char model[64];
    bool is_wifi;
    bool is_active;
} adb_device_info_t;

static HWND s_main_hwnd = NULL;
static HWND s_cam_hwnd = NULL;
static HWND s_menu_hwnd = NULL;
static HWND s_toast_hwnd = NULL;
static HWND s_tooltip_hwnd = NULL;
static HICON s_app_icon = NULL;
static WNDPROC s_orig_wndproc = NULL;

static char s_serial[128] = {0};
static char s_device_model[128] = "Device";
static char s_toast_message[128] = "Copied to clipboard";

static bool s_cam_hovered = false;
static bool s_cam_pressed = false;
static bool s_menu_hovered = false;
static bool s_menu_pressed = false;

static bool s_is_capturing = false;
static bool s_is_switching_wifi = false;

static void reposition_controls(void);
static void show_custom_toast(const char *msg);
static void switch_to_target_device(const char *target_serial);
static void show_flux_menu_at_point(int screenX, int screenY);
static void trigger_capture(void);

// ---------------------------------------------------------------------------
// RESOLVE BUNDLED ADB PATH & SILENT PROCESS EXECUTION
// ---------------------------------------------------------------------------
static void
get_adb_path(char *out, size_t max_len) {
    char exe_path[MAX_PATH];
    if (GetModuleFileNameA(NULL, exe_path, MAX_PATH)) {
        char *last_slash = strrchr(exe_path, '\\');
        if (last_slash) {
            *last_slash = '\0';
            if (strlen(exe_path) + 10 < max_len) {
                snprintf(out, max_len, "%s\\adb.exe", exe_path);
                DWORD attr = GetFileAttributesA(out);
                if (attr != INVALID_FILE_ATTRIBUTES && !(attr & FILE_ATTRIBUTE_DIRECTORY)) {
                    return;
                }
            }
        }
    }
    strncpy(out, "adb.exe", max_len - 1);
    out[max_len - 1] = '\0';
}

static bool
run_command_silent(const char *cmdline, DWORD timeout_ms) {
    STARTUPINFOA si;
    memset(&si, 0, sizeof(si));
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;

    PROCESS_INFORMATION pi;
    memset(&pi, 0, sizeof(pi));

    char cmd_copy[1024];
    strncpy(cmd_copy, cmdline, sizeof(cmd_copy) - 1);
    cmd_copy[sizeof(cmd_copy) - 1] = '\0';

    if (CreateProcessA(NULL, cmd_copy, NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
        WaitForSingleObject(pi.hProcess, timeout_ms);
        CloseHandle(pi.hThread);
        CloseHandle(pi.hProcess);
        return true;
    }
    return false;
}

// ---------------------------------------------------------------------------
// ADB DEVICE DISCOVERY & WI-FI IP DETECTION
// ---------------------------------------------------------------------------
static int
get_device_list(const char *current_serial, adb_device_info_t *devices, int max_devices) {
    char adb_path[MAX_PATH];
    get_adb_path(adb_path, sizeof(adb_path));

    char cmd[512];
    snprintf(cmd, sizeof(cmd), "\"%s\" devices -l", adb_path);

    FILE *fp = _popen(cmd, "r");
    if (!fp) return 0;

    char line[512];
    int count = 0;
    while (fgets(line, sizeof(line), fp)) {
        if (strstr(line, "List of devices") || strlen(line) < 5) continue;
        char serial[64] = {0};
        char state[32] = {0};
        if (sscanf(line, "%63s %31s", serial, state) < 2) continue;
        if (strcmp(state, "device") != 0) continue;

        snprintf(devices[count].serial, sizeof(devices[count].serial), "%s", serial);
        devices[count].is_wifi = (strchr(serial, ':') != NULL);
        devices[count].is_active = (current_serial && strcmp(serial, current_serial) == 0);

        char *model_pos = strstr(line, "model:");
        if (model_pos) {
            sscanf(model_pos + 6, "%63s", devices[count].model);
        } else {
            snprintf(devices[count].model, sizeof(devices[count].model), "%s", serial);
        }

        count++;
        if (count >= max_devices) break;
    }
    _pclose(fp);
    return count;
}

static bool
get_device_wifi_ip(const char *serial, char *out_ip, size_t max_len) {
    char adb_path[MAX_PATH];
    get_adb_path(adb_path, sizeof(adb_path));

    char cmd[512];
    if (serial && serial[0] != '\0') {
        snprintf(cmd, sizeof(cmd), "\"%s\" -s %s shell ip -br a", adb_path, serial);
    } else {
        snprintf(cmd, sizeof(cmd), "\"%s\" shell ip -br a", adb_path);
    }

    FILE *fp = _popen(cmd, "r");
    if (!fp) return false;

    char line[512];
    while (fgets(line, sizeof(line), fp)) {
        if (strstr(line, "127.0.0.1") || strstr(line, "DOWN")) continue;
        char iface[32] = {0}, state[32] = {0}, ip_cidr[64] = {0};
        if (sscanf(line, "%31s %31s %63s", iface, state, ip_cidr) >= 3) {
            if (strcmp(state, "UP") == 0) {
                char *slash = strchr(ip_cidr, '/');
                if (slash) *slash = '\0';
                if (strchr(ip_cidr, '.')) {
                    snprintf(out_ip, max_len, "%s", ip_cidr);
                    _pclose(fp);
                    return true;
                }
            }
        }
    }
    _pclose(fp);
    return false;
}

static unsigned __stdcall
device_model_discovery_worker(void *arg) {
    (void) arg;
    adb_device_info_t devs[8];
    int count = get_device_list(s_serial, devs, 8);
    for (int i = 0; i < count; i++) {
        if (devs[i].is_active || count == 1) {
            snprintf(s_device_model, sizeof(s_device_model), "%s", devs[i].model);
            if (s_menu_hwnd && IsWindow(s_menu_hwnd)) {
                InvalidateRect(s_menu_hwnd, NULL, FALSE);
            }
            return 0;
        }
    }
    return 0;
}

static void
start_device_model_discovery(void) {
    if (s_serial[0] != '\0') {
        snprintf(s_device_model, sizeof(s_device_model), "%s", s_serial);
    }
    uintptr_t th = _beginthreadex(NULL, 0, device_model_discovery_worker, NULL, 0, NULL);
    if (th) {
        CloseHandle((HANDLE) th);
    }
}

// ---------------------------------------------------------------------------
// 1-CLICK WIRELESS ADB SWITCHER WORKER
// ---------------------------------------------------------------------------
static unsigned __stdcall
wireless_switch_worker(void *arg) {
    (void) arg;

    char ip[64] = {0};
    bool found_ip = get_device_wifi_ip(s_serial, ip, sizeof(ip));
    if (!found_ip || ip[0] == '\0') {
        LOGW("Could not detect device Wi-Fi IP address");
        show_custom_toast("No Wi-Fi IP found. Connect phone to Wi-Fi.");
        s_is_switching_wifi = false;
        return 0;
    }

    char adb_path[MAX_PATH];
    get_adb_path(adb_path, sizeof(adb_path));

    // 1. adb -s <serial> tcpip 5555
    char cmd_tcpip[512];
    snprintf(cmd_tcpip, sizeof(cmd_tcpip), "\"%s\" -s %s tcpip 5555", adb_path, s_serial);
    run_command_silent(cmd_tcpip, 3000);

    Sleep(500);

    // 2. adb connect <ip>:5555
    char cmd_connect[512];
    snprintf(cmd_connect, sizeof(cmd_connect), "\"%s\" connect %s:5555", adb_path, ip);
    run_command_silent(cmd_connect, 4000);

    // 3. Verify connection
    char target_serial[64];
    snprintf(target_serial, sizeof(target_serial), "%s:5555", ip);

    adb_device_info_t devs[8];
    int count = get_device_list(NULL, devs, 8);
    bool connected = false;
    for (int i = 0; i < count; i++) {
        if (strcmp(devs[i].serial, target_serial) == 0) {
            connected = true;
            break;
        }
    }

    if (connected) {
        char toast_msg[128];
        snprintf(toast_msg, sizeof(toast_msg), "Connected to Wireless: %s", target_serial);
        show_custom_toast(toast_msg);

        // Seamlessly switch to wireless endpoint
        switch_to_target_device(target_serial);
    } else {
        char toast_msg[128];
        snprintf(toast_msg, sizeof(toast_msg), "Port 5555 open on %s. Check Wi-Fi subnet.", ip);
        show_custom_toast(toast_msg);
    }

    s_is_switching_wifi = false;
    return 0;
}

static void
trigger_wireless_adb_switch(void) {
    if (s_is_switching_wifi) return;
    s_is_switching_wifi = true;
    show_custom_toast("Switching to Wireless ADB (5555)...");
    uintptr_t th = _beginthreadex(NULL, 0, wireless_switch_worker, NULL, 0, NULL);
    if (th) {
        CloseHandle((HANDLE) th);
    } else {
        s_is_switching_wifi = false;
    }
}

static void
switch_to_target_device(const char *target_serial) {
    char exe_path[MAX_PATH];
    if (!GetModuleFileNameA(NULL, exe_path, MAX_PATH)) {
        return;
    }

    char cmdline[1024];
    snprintf(cmdline, sizeof(cmdline),
             "\"%s\" -s %s --window-title \"FluxMirror\" --always-on-top --max-fps 120 --video-buffer 0 --video-codec h264 --video-bit-rate 16M --stay-awake --print-fps",
             exe_path, target_serial);

    STARTUPINFOA si;
    memset(&si, 0, sizeof(si));
    si.cb = sizeof(si);
    PROCESS_INFORMATION pi;
    memset(&pi, 0, sizeof(pi));

    if (CreateProcessA(NULL, cmdline, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
        CloseHandle(pi.hThread);
        CloseHandle(pi.hProcess);
        PostMessage(s_main_hwnd, WM_CLOSE, 0, 0);
    }
}

// ---------------------------------------------------------------------------
// FLUXMIRROR BRAND CONTROL MENU
// ---------------------------------------------------------------------------
static void
show_flux_menu_at_point(int screenX, int screenY) {
    adb_device_info_t devices[16];
    int count = get_device_list(s_serial, devices, 16);

    HMENU hMenu = CreatePopupMenu();

    // 1. Devices submenu
    HMENU hDevMenu = CreatePopupMenu();
    if (count == 0) {
        AppendMenuA(hDevMenu, MF_STRING | MF_GRAYED, 0, "No Devices Found via ADB");
    } else {
        for (int i = 0; i < count; i++) {
            char item_text[256];
            snprintf(item_text, sizeof(item_text), "%.63s [%.63s] (%s)",
                     devices[i].model,
                     devices[i].serial,
                     devices[i].is_wifi ? "Wi-Fi" : "USB");
            UINT flags = MF_STRING;
            if (devices[i].is_active) {
                flags |= MF_CHECKED;
            }
            AppendMenuA(hDevMenu, flags, 100 + i, item_text);
        }
    }
    AppendMenuA(hDevMenu, MF_SEPARATOR, 0, NULL);
    AppendMenuA(hDevMenu, MF_STRING, 202, "Refresh Device List");

    char active_label[256];
    snprintf(active_label, sizeof(active_label), "Active Device: %s", s_device_model);
    AppendMenuA(hMenu, MF_POPUP, (UINT_PTR) hDevMenu, active_label);

    AppendMenuA(hMenu, MF_SEPARATOR, 0, NULL);

    // 2. Wireless ADB Switcher
    AppendMenuA(hMenu, MF_STRING, 201, "1-Click Wireless ADB Switcher (Port 5555)");

    // 3. Screenshot Capture
    AppendMenuA(hMenu, MF_STRING, 203, "Take Screenshot (Copy to Clipboard)");

    AppendMenuA(hMenu, MF_SEPARATOR, 0, NULL);

    // 4. Quick Device Shortcuts
    AppendMenuA(hMenu, MF_STRING, 204, "Turn Device Screen Off (Keep Mirror Active)");

    AppendMenuA(hMenu, MF_SEPARATOR, 0, NULL);
    AppendMenuA(hMenu, MF_STRING | MF_GRAYED, 0, "FluxMirror v5.0.1 - Native Engine");

    int cmd = TrackPopupMenu(hMenu, TPM_RETURNCMD | TPM_NONOTIFY | TPM_LEFTALIGN | TPM_TOPALIGN,
                             screenX, screenY, 0, s_main_hwnd, NULL);
    DestroyMenu(hMenu);

    if (cmd >= 100 && cmd < 100 + count) {
        int idx = cmd - 100;
        if (!devices[idx].is_active) {
            char toast[128];
            snprintf(toast, sizeof(toast), "Switching to %s...", devices[idx].model);
            show_custom_toast(toast);
            switch_to_target_device(devices[idx].serial);
        }
    } else if (cmd == 201) {
        trigger_wireless_adb_switch();
    } else if (cmd == 202) {
        start_device_model_discovery();
        if (s_menu_hwnd && IsWindow(s_menu_hwnd)) InvalidateRect(s_menu_hwnd, NULL, FALSE);
    } else if (cmd == 203) {
        trigger_capture();
    } else if (cmd == 204) {
        char adb_path[MAX_PATH];
        get_adb_path(adb_path, sizeof(adb_path));
        char cmd_power[512];
        if (s_serial[0] != '\0') {
            snprintf(cmd_power, sizeof(cmd_power), "\"%s\" -s %s shell input keyevent 223", adb_path, s_serial);
        } else {
            snprintf(cmd_power, sizeof(cmd_power), "\"%s\" shell input keyevent 223", adb_path);
        }
        run_command_silent(cmd_power, 2000);
        show_custom_toast("Turned device display off");
    }
}

// ---------------------------------------------------------------------------
// SCREENSHOT CAPTURE THREAD (Silent ADB -> PNG -> Clipboard)
// ---------------------------------------------------------------------------
static unsigned __stdcall
capture_screenshot_worker(void *arg) {
    (void) arg;

    char temp_dir[MAX_PATH];
    DWORD len = GetTempPathA(sizeof(temp_dir), temp_dir);
    if (!len) {
        strcpy(temp_dir, ".");
    }
    char temp_file[MAX_PATH];
    snprintf(temp_file, sizeof(temp_file), "%sflux_snap_%lu.png", temp_dir, GetTickCount());

    SECURITY_ATTRIBUTES sa;
    memset(&sa, 0, sizeof(sa));
    sa.nLength = sizeof(sa);
    sa.bInheritHandle = TRUE;

    HANDLE hFile = CreateFileA(temp_file, GENERIC_WRITE, 0, &sa, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
        LOGE("Failed to create temporary file for screenshot: %s", temp_file);
        s_is_capturing = false;
        return 0;
    }

    char adb_path[MAX_PATH];
    get_adb_path(adb_path, sizeof(adb_path));

    char cmdline[1024];
    if (s_serial[0] != '\0') {
        snprintf(cmdline, sizeof(cmdline), "\"%s\" -s %s exec-out screencap -p", adb_path, s_serial);
    } else {
        snprintf(cmdline, sizeof(cmdline), "\"%s\" exec-out screencap -p", adb_path);
    }

    STARTUPINFOA si;
    memset(&si, 0, sizeof(si));
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
    si.hStdOutput = hFile;
    si.hStdError = GetStdHandle(STD_ERROR_HANDLE);
    si.wShowWindow = SW_HIDE;

    PROCESS_INFORMATION pi;
    memset(&pi, 0, sizeof(pi));

    BOOL spawned = CreateProcessA(NULL, cmdline, NULL, NULL, TRUE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi);
    if (!spawned) {
        LOGE("Failed to spawn ADB process: %lu", GetLastError());
        CloseHandle(hFile);
        DeleteFileA(temp_file);
        s_is_capturing = false;
        return 0;
    }

    WaitForSingleObject(pi.hProcess, 5000);
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
    CloseHandle(hFile);

    void *png_bytes = NULL;
    long png_file_len = 0;
    FILE *fp = fopen(temp_file, "rb");
    if (fp) {
        fseek(fp, 0, SEEK_END);
        png_file_len = ftell(fp);
        fseek(fp, 0, SEEK_SET);
        if (png_file_len > 0) {
            png_bytes = malloc(png_file_len);
            if (png_bytes) {
                fread(png_bytes, 1, png_file_len, fp);
            }
        }
        fclose(fp);
    }

    SDL_Surface *surface = sc_icon_load_from_full_path(temp_file);
    DeleteFileA(temp_file);

    if (!surface) {
        LOGE("Failed to decode screenshot image");
        if (png_bytes) free(png_bytes);
        s_is_capturing = false;
        return 0;
    }

    int w = surface->w;
    int h = surface->h;
    size_t raw_stride = (size_t) w * 4;
    size_t raw_pixels_size = raw_stride * (size_t) h;

    void *top_down_bgra = malloc(raw_pixels_size);
    if (!top_down_bgra) {
        LOG_OOM();
        SDL_DestroySurface(surface);
        if (png_bytes) free(png_bytes);
        s_is_capturing = false;
        return 0;
    }

    bool conv_ok = SDL_ConvertPixels(w, h, surface->format, surface->pixels, surface->pitch,
                                     SDL_PIXELFORMAT_BGRA32, top_down_bgra, raw_stride);
    SDL_DestroySurface(surface);

    if (!conv_ok) {
        LOGE("Failed to convert screenshot to BGRA32");
        free(top_down_bgra);
        if (png_bytes) free(png_bytes);
        s_is_capturing = false;
        return 0;
    }

    // 1. Allocate CF_DIB (standard bottom-up Windows DIB)
    size_t dib_size = sizeof(BITMAPINFOHEADER) + raw_pixels_size;
    HGLOBAL hDib = GlobalAlloc(GMEM_MOVEABLE, dib_size);
    if (!hDib) {
        LOG_OOM();
        free(top_down_bgra);
        if (png_bytes) free(png_bytes);
        s_is_capturing = false;
        return 0;
    }

    void *pDib = GlobalLock(hDib);
    BITMAPINFOHEADER *hdr = (BITMAPINFOHEADER *) pDib;
    memset(hdr, 0, sizeof(BITMAPINFOHEADER));
    hdr->biSize = sizeof(BITMAPINFOHEADER);
    hdr->biWidth = w;
    hdr->biHeight = h;
    hdr->biPlanes = 1;
    hdr->biBitCount = 32;
    hdr->biCompression = BI_RGB;
    hdr->biSizeImage = raw_pixels_size;
    hdr->biXPelsPerMeter = 2835;
    hdr->biYPelsPerMeter = 2835;

    uint8_t *dst_pixels = (uint8_t *) pDib + sizeof(BITMAPINFOHEADER);
    for (int y = 0; y < h; y++) {
        uint8_t *dst_row = dst_pixels + (size_t) (h - 1 - y) * raw_stride;
        const uint8_t *src_row = (const uint8_t *) top_down_bgra + (size_t) y * raw_stride;
        memcpy(dst_row, src_row, raw_stride);
    }
    GlobalUnlock(hDib);

    // 2. Create CF_BITMAP (GDI compatible bitmap DDB)
    HDC screen_dc = GetDC(NULL);
    HBITMAP hBmp = CreateCompatibleBitmap(screen_dc, w, h);
    if (hBmp) {
        BITMAPINFO bmi;
        memset(&bmi, 0, sizeof(bmi));
        bmi.bmiHeader = *hdr;
        SetDIBits(screen_dc, hBmp, 0, h, dst_pixels, &bmi, DIB_RGB_COLORS);
    }
    ReleaseDC(NULL, screen_dc);

    free(top_down_bgra);

    // 3. Allocate PNG format
    UINT cf_png = RegisterClipboardFormatA("PNG");
    HGLOBAL hPng = NULL;
    if (png_bytes && png_file_len > 0) {
        hPng = GlobalAlloc(GMEM_MOVEABLE, png_file_len);
        if (hPng) {
            void *pPng = GlobalLock(hPng);
            memcpy(pPng, png_bytes, png_file_len);
            GlobalUnlock(hPng);
        }
        free(png_bytes);
    }

    // 4. Inject all formats into Windows Clipboard
    if (OpenClipboard(s_main_hwnd)) {
        EmptyClipboard();
        SetClipboardData(CF_DIB, hDib);
        if (hBmp) SetClipboardData(CF_BITMAP, hBmp);
        if (hPng) SetClipboardData(cf_png, hPng);
        CloseClipboard();
        MessageBeep(MB_OK);
        LOGI("Screenshot successfully copied to clipboard: %dx%d (CF_DIB, CF_BITMAP, PNG)", w, h);
        show_custom_toast("Copied to clipboard");
    } else {
        LOGE("Failed to open Windows clipboard");
        GlobalFree(hDib);
        if (hBmp) DeleteObject(hBmp);
        if (hPng) GlobalFree(hPng);
    }

    s_is_capturing = false;
    return 0;
}

static void
trigger_capture(void) {
    if (s_is_capturing) return;
    s_is_capturing = true;
    uintptr_t th = _beginthreadex(NULL, 0, capture_screenshot_worker, NULL, 0, NULL);
    if (th) {
        CloseHandle((HANDLE) th);
    } else {
        s_is_capturing = false;
    }
}

// ---------------------------------------------------------------------------
// TOAST NOTIFICATION WINDOW PROCEDURE
// ---------------------------------------------------------------------------
static void
show_custom_toast(const char *msg) {
    if (msg) {
        strncpy(s_toast_message, msg, sizeof(s_toast_message) - 1);
        s_toast_message[sizeof(s_toast_message) - 1] = '\0';
    }
    if (s_cam_hwnd && IsWindow(s_cam_hwnd)) {
        PostMessage(s_cam_hwnd, WM_SHOW_TOAST, 0, 0);
    }
}

static LRESULT CALLBACK
ToastWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_TIMER: {
            if (wParam == TOAST_TIMER_ID) {
                KillTimer(hwnd, TOAST_TIMER_ID);
                ShowWindow(hwnd, SW_HIDE);
            }
            return 0;
        }
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);

            RECT r;
            GetClientRect(hwnd, &r);

            HDC memDC = CreateCompatibleDC(hdc);
            HBITMAP memBmp = CreateCompatibleBitmap(hdc, r.right, r.bottom);
            HBITMAP oldBmp = (HBITMAP) SelectObject(memDC, memBmp);

            HBRUSH bgBrush = CreateSolidBrush(RGB(30, 41, 59));
            FillRect(memDC, &r, bgBrush);
            DeleteObject(bgBrush);

            HPEN borderPen = CreatePen(PS_SOLID, 2, RGB(255, 107, 0));
            HPEN oldPen = (HPEN) SelectObject(memDC, borderPen);
            HBRUSH nullBrush = (HBRUSH) GetStockObject(NULL_BRUSH);
            HBRUSH oldBrush = (HBRUSH) SelectObject(memDC, nullBrush);
            RoundRect(memDC, r.left + 1, r.top + 1, r.right - 1, r.bottom - 1, 18, 18);
            SelectObject(memDC, oldBrush);
            SelectObject(memDC, oldPen);
            DeleteObject(borderPen);

            HBRUSH badgeBrush = CreateSolidBrush(RGB(16, 185, 129));
            HBRUSH oldBadgeBrush = (HBRUSH) SelectObject(memDC, badgeBrush);
            HPEN nullPen = (HPEN) GetStockObject(NULL_PEN);
            HPEN oldBadgePen = (HPEN) SelectObject(memDC, nullPen);
            Ellipse(memDC, 14, 10, 32, 28);
            SelectObject(memDC, oldBadgeBrush);
            SelectObject(memDC, oldBadgePen);
            DeleteObject(badgeBrush);

            SetBkMode(memDC, TRANSPARENT);
            SetTextColor(memDC, RGB(255, 255, 255));
            HFONT hFontSymbol = CreateFontW(14, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
                                            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
                                            CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
                                            DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI Symbol");
            HFONT oldFont = (HFONT) SelectObject(memDC, hFontSymbol);
            RECT checkRect = { 14, 9, 32, 29 };
            DrawTextW(memDC, L"\x2713", -1, &checkRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
            SelectObject(memDC, oldFont);
            DeleteObject(hFontSymbol);

            HFONT hFontText = CreateFontW(13, 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE,
                                          DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
                                          CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
                                          DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
            oldFont = (HFONT) SelectObject(memDC, hFontText);
            RECT textRect = { 38, 0, r.right - 10, r.bottom };
            DrawTextA(memDC, s_toast_message, -1, &textRect, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
            SelectObject(memDC, oldFont);
            DeleteObject(hFontText);

            BitBlt(hdc, 0, 0, r.right, r.bottom, memDC, 0, 0, SRCCOPY);

            SelectObject(memDC, oldBmp);
            DeleteObject(memBmp);
            DeleteDC(memDC);

            EndPaint(hwnd, &ps);
            return 0;
        }
        case WM_ERASEBKGND:
            return 1;
        default:
            return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
}

// ---------------------------------------------------------------------------
// CAMERA BUTTON WINDOW PROCEDURE
// ---------------------------------------------------------------------------
static LRESULT CALLBACK
CameraBtnWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_TIMER: {
            if (wParam == SYNC_TIMER_ID) {
                reposition_controls();
            }
            return 0;
        }
        case WM_SHOW_TOAST: {
            if (s_toast_hwnd && s_main_hwnd && IsWindow(s_main_hwnd)) {
                HDC hdc = GetDC(s_toast_hwnd);
                HFONT hFont = CreateFontA(13, 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE,
                                          DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
                                          CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
                                          DEFAULT_PITCH | FF_DONTCARE, "Segoe UI");
                HFONT old = (HFONT) SelectObject(hdc, hFont);
                RECT calcRect = {0, 0, 0, 0};
                DrawTextA(hdc, s_toast_message, -1, &calcRect, DT_CALCRECT | DT_SINGLELINE);
                SelectObject(hdc, old);
                DeleteObject(hFont);
                ReleaseDC(s_toast_hwnd, hdc);

                int toastW = calcRect.right + 55;
                if (toastW < 200) toastW = 200;
                int toastH = 38;

                HRGN hRgn = CreateRoundRectRgn(0, 0, toastW, toastH, 19, 19);
                SetWindowRgn(s_toast_hwnd, hRgn, TRUE);

                RECT winRect;
                GetWindowRect(s_main_hwnd, &winRect);
                int winW = winRect.right - winRect.left;
                int toastX = winRect.left + (winW - toastW) / 2;
                int toastY = winRect.top + 48;

                SetWindowPos(s_toast_hwnd, HWND_TOPMOST, toastX, toastY, toastW, toastH,
                             SWP_NOACTIVATE | SWP_SHOWWINDOW);
                InvalidateRect(s_toast_hwnd, NULL, TRUE);
                SetTimer(s_toast_hwnd, TOAST_TIMER_ID, TOAST_DURATION_MS, NULL);
            }
            return 0;
        }
        case WM_MOUSEMOVE: {
            if (!s_cam_hovered) {
                s_cam_hovered = true;
                InvalidateRect(hwnd, NULL, FALSE);

                TRACKMOUSEEVENT tme;
                tme.cbSize = sizeof(tme);
                tme.dwFlags = TME_LEAVE;
                tme.hwndTrack = hwnd;
                tme.dwHoverTime = 0;
                TrackMouseEvent(&tme);
            }
            return 0;
        }
        case WM_MOUSELEAVE: {
            s_cam_hovered = false;
            s_cam_pressed = false;
            InvalidateRect(hwnd, NULL, FALSE);
            return 0;
        }
        case WM_LBUTTONDOWN: {
            s_cam_pressed = true;
            SetCapture(hwnd);
            InvalidateRect(hwnd, NULL, FALSE);
            return 0;
        }
        case WM_LBUTTONUP: {
            if (s_cam_pressed) {
                ReleaseCapture();
                s_cam_pressed = false;
                InvalidateRect(hwnd, NULL, FALSE);

                RECT r;
                GetClientRect(hwnd, &r);
                POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
                if (PtInRect(&r, pt)) {
                    trigger_capture();
                }
            }
            return 0;
        }
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);

            RECT r;
            GetClientRect(hwnd, &r);

            HDC memDC = CreateCompatibleDC(hdc);
            HBITMAP memBmp = CreateCompatibleBitmap(hdc, r.right, r.bottom);
            HBITMAP oldBmp = (HBITMAP) SelectObject(memDC, memBmp);

            HWND activeWnd = GetActiveWindow();
            bool isActive = (activeWnd == s_main_hwnd || activeWnd == s_cam_hwnd);

            COLORREF bgColor;
            COLORREF glyphColor;

            if (s_cam_pressed) {
                bgColor = RGB(38, 38, 38);
                glyphColor = RGB(255, 140, 50);
            } else if (s_cam_hovered) {
                bgColor = RGB(50, 50, 50);
                glyphColor = RGB(255, 107, 0);
            } else {
                bgColor = isActive ? RGB(32, 32, 32) : RGB(43, 43, 43);
                glyphColor = isActive ? RGB(235, 235, 235) : RGB(170, 170, 170);
            }

            HBRUSH bgBrush = CreateSolidBrush(bgColor);
            FillRect(memDC, &r, bgBrush);
            DeleteObject(bgBrush);

            HPEN camPen = CreatePen(PS_SOLID, 1, glyphColor);
            HPEN oldPen = (HPEN) SelectObject(memDC, camPen);
            HBRUSH nullBrush = (HBRUSH) GetStockObject(NULL_BRUSH);
            HBRUSH oldBrush = (HBRUSH) SelectObject(memDC, nullBrush);

            int cx = r.right / 2;
            int cy = r.bottom / 2;

            RoundRect(memDC, cx - 8, cy - 4, cx + 8, cy + 8, 3, 3);

            POINT topPts[4] = {
                { cx - 4, cy - 4 },
                { cx - 2, cy - 7 },
                { cx + 2, cy - 7 },
                { cx + 4, cy - 4 }
            };
            Polyline(memDC, topPts, 4);

            Ellipse(memDC, cx - 3, cy - 1, cx + 3, cy + 5);

            HBRUSH dotBrush = CreateSolidBrush(glyphColor);
            HBRUSH oldDotBrush = (HBRUSH) SelectObject(memDC, dotBrush);
            Ellipse(memDC, cx + 4, cy - 3, cx + 6, cy - 1);
            SelectObject(memDC, oldDotBrush);
            DeleteObject(dotBrush);

            SelectObject(memDC, oldBrush);
            SelectObject(memDC, oldPen);
            DeleteObject(camPen);

            BitBlt(hdc, 0, 0, r.right, r.bottom, memDC, 0, 0, SRCCOPY);

            SelectObject(memDC, oldBmp);
            DeleteObject(memBmp);
            DeleteDC(memDC);

            EndPaint(hwnd, &ps);
            return 0;
        }
        case WM_ERASEBKGND:
            return 1;
        default:
            return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
}

// ---------------------------------------------------------------------------
// BRAND MENU BUTTON WINDOW PROCEDURE
// ---------------------------------------------------------------------------
static LRESULT CALLBACK
MenuBtnWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    static POINT s_down_pt = {0};
    static bool s_is_dragging = false;

    switch (uMsg) {
        case WM_MOUSEMOVE: {
            POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
            if (s_menu_pressed && (wParam & MK_LBUTTON)) {
                if (abs(pt.x - s_down_pt.x) > 4 || abs(pt.y - s_down_pt.y) > 4) {
                    s_is_dragging = true;
                    ReleaseCapture();
                    s_menu_pressed = false;
                    s_menu_hovered = false;
                    InvalidateRect(hwnd, NULL, FALSE);
                    POINT screenPt = pt;
                    ClientToScreen(hwnd, &screenPt);
                    SendMessage(s_main_hwnd, WM_NCLBUTTONDOWN, HTCAPTION, MAKELPARAM(screenPt.x, screenPt.y));
                    return 0;
                }
            }

            if (!s_menu_hovered) {
                s_menu_hovered = true;
                InvalidateRect(hwnd, NULL, FALSE);

                TRACKMOUSEEVENT tme;
                tme.cbSize = sizeof(tme);
                tme.dwFlags = TME_LEAVE;
                tme.hwndTrack = hwnd;
                tme.dwHoverTime = 0;
                TrackMouseEvent(&tme);
            }
            return 0;
        }
        case WM_MOUSELEAVE: {
            s_menu_hovered = false;
            s_menu_pressed = false;
            InvalidateRect(hwnd, NULL, FALSE);
            return 0;
        }
        case WM_LBUTTONDOWN: {
            s_menu_pressed = true;
            s_is_dragging = false;
            s_down_pt.x = GET_X_LPARAM(lParam);
            s_down_pt.y = GET_Y_LPARAM(lParam);
            SetCapture(hwnd);
            InvalidateRect(hwnd, NULL, FALSE);
            return 0;
        }
        case WM_LBUTTONUP: {
            if (s_menu_pressed) {
                ReleaseCapture();
                s_menu_pressed = false;
                InvalidateRect(hwnd, NULL, FALSE);
                if (!s_is_dragging) {
                    RECT r;
                    GetWindowRect(hwnd, &r);
                    show_flux_menu_at_point(r.left, r.bottom + 2);
                }
            }
            return 0;
        }
        case WM_RBUTTONUP: {
            RECT r;
            GetWindowRect(hwnd, &r);
            show_flux_menu_at_point(r.left + GET_X_LPARAM(lParam), r.top + GET_Y_LPARAM(lParam));
            return 0;
        }
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);

            RECT r;
            GetClientRect(hwnd, &r);

            HDC memDC = CreateCompatibleDC(hdc);
            HBITMAP memBmp = CreateCompatibleBitmap(hdc, r.right, r.bottom);
            HBITMAP oldBmp = (HBITMAP) SelectObject(memDC, memBmp);

            HWND activeWnd = GetActiveWindow();
            bool isActive = (activeWnd == s_main_hwnd || activeWnd == hwnd);

            COLORREF tbBg = isActive ? RGB(32, 32, 32) : RGB(43, 43, 43);
            HBRUSH bgBrush = CreateSolidBrush(tbBg);
            FillRect(memDC, &r, bgBrush);
            DeleteObject(bgBrush);

            if (s_menu_hovered || s_menu_pressed) {
                COLORREF pillBg = s_menu_pressed ? RGB(55, 65, 81) : RGB(45, 55, 72);
                COLORREF pillBorder = s_menu_pressed ? RGB(255, 140, 50) : RGB(255, 107, 0);

                HBRUSH pillBrush = CreateSolidBrush(pillBg);
                HBRUSH oldBrush = (HBRUSH) SelectObject(memDC, pillBrush);
                HPEN borderPen = CreatePen(PS_SOLID, 1, pillBorder);
                HPEN oldPen = (HPEN) SelectObject(memDC, borderPen);

                RoundRect(memDC, r.left + 1, r.top + 1, r.right - 1, r.bottom - 1, 8, 8);

                SelectObject(memDC, oldPen);
                SelectObject(memDC, oldBrush);
                DeleteObject(borderPen);
                DeleteObject(pillBrush);
            }

            // Draw application icon
            if (s_app_icon) {
                DrawIconEx(memDC, r.left + 5, (r.bottom - 16) / 2, s_app_icon, 16, 16, 0, NULL, DI_NORMAL);
            }

            // Draw "FluxMirror" text
            SetBkMode(memDC, TRANSPARENT);
            SetTextColor(memDC, isActive ? RGB(245, 245, 245) : RGB(190, 190, 190));
            HFONT hFont = CreateFontW(12, 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE,
                                      DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
                                      CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
                                      DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
            HFONT oldFont = (HFONT) SelectObject(memDC, hFont);
            RECT txtRect = { r.left + 25, r.top, r.right - 14, r.bottom };
            DrawTextA(memDC, "FluxMirror", -1, &txtRect, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
            SelectObject(memDC, oldFont);
            DeleteObject(hFont);

            // Draw subtle dropdown chevron "v"
            HFONT hArrowFont = CreateFontW(9, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                                           DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
                                           CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
                                           DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI Symbol");
            HFONT oldAFont = (HFONT) SelectObject(memDC, hArrowFont);
            SetTextColor(memDC, s_menu_hovered ? RGB(255, 107, 0) : (isActive ? RGB(180, 180, 180) : RGB(130, 130, 130)));
            RECT arrowRect = { r.right - 14, r.top, r.right - 3, r.bottom };
            DrawTextW(memDC, L"\x25BC", -1, &arrowRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
            SelectObject(memDC, oldAFont);
            DeleteObject(hArrowFont);

            BitBlt(hdc, 0, 0, r.right, r.bottom, memDC, 0, 0, SRCCOPY);

            SelectObject(memDC, oldBmp);
            DeleteObject(memBmp);
            DeleteDC(memDC);

            EndPaint(hwnd, &ps);
            return 0;
        }
        case WM_ERASEBKGND:
            return 1;
        default:
            return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
}

// ---------------------------------------------------------------------------
// WINDOW MESSAGE HOOK & POSITION SYNCHRONIZATION
// ---------------------------------------------------------------------------
static void
reposition_controls(void) {
    static bool s_in_reposition = false;
    if (s_in_reposition) {
        return;
    }
    s_in_reposition = true;

    if (!s_main_hwnd || !IsWindow(s_main_hwnd)) {
        s_in_reposition = false;
        return;
    }

    static int s_last_cam_x = -9999, s_last_cam_y = -9999, s_last_cam_w = -1, s_last_cam_h = -1;
    static int s_last_menu_x = -9999, s_last_menu_y = -9999, s_last_menu_w = -1, s_last_menu_h = -1;

    if (IsIconic(s_main_hwnd) || !IsWindowVisible(s_main_hwnd)) {
        if (s_cam_hwnd && IsWindow(s_cam_hwnd)) ShowWindow(s_cam_hwnd, SW_HIDE);
        if (s_menu_hwnd && IsWindow(s_menu_hwnd)) ShowWindow(s_menu_hwnd, SW_HIDE);
        if (s_toast_hwnd && IsWindow(s_toast_hwnd)) ShowWindow(s_toast_hwnd, SW_HIDE);
        s_last_cam_x = -9999;
        s_last_menu_x = -9999;
        s_in_reposition = false;
        return;
    }

    TITLEBARINFOEX_LOCAL tbi;
    memset(&tbi, 0, sizeof(tbi));
    tbi.cbSize = sizeof(tbi);
    SendMessage(s_main_hwnd, WM_GETTITLEBARINFOEX, 0, (LPARAM) &tbi);

    RECT minRect = tbi.rgrect[2]; // Index 2 is Minimize button
    RECT maxRect = tbi.rgrect[3]; // Index 3 is Maximize button
    RECT winRect;
    GetWindowRect(s_main_hwnd, &winRect);
    int winWidth = winRect.right - winRect.left;

    // 1. Camera button replaces Maximize button on the right
    int camX, camY, camW, camH;
    if (maxRect.right > maxRect.left && maxRect.bottom > maxRect.top) {
        camX = maxRect.left;
        camY = maxRect.top;
        camW = maxRect.right - maxRect.left;
        camH = maxRect.bottom - maxRect.top;
    } else if (minRect.right > minRect.left && minRect.bottom > minRect.top) {
        // Fallback: place immediately to left of Minimize
        camW = minRect.right - minRect.left;
        camH = minRect.bottom - minRect.top;
        camX = minRect.left - camW;
        camY = minRect.top;
    } else {
        camW = 47;
        camH = 29;
        camX = winRect.right - 94;
        camY = winRect.top + 3;
    }

    // 2. FluxMirror Brand Menu Button on top-left of titlebar
    int tbTop = (tbi.rcTitleBar.bottom > tbi.rcTitleBar.top) ? tbi.rcTitleBar.top : (winRect.top + 1);
    int tbH = (tbi.rcTitleBar.bottom > tbi.rcTitleBar.top) ? (tbi.rcTitleBar.bottom - tbi.rcTitleBar.top) : 31;
    int menuX = winRect.left + 6;
    int menuY = tbTop + 2;
    int menuW = 114;
    int menuH = (tbH > 6) ? (tbH - 4) : 26;

    if (s_cam_hwnd && IsWindow(s_cam_hwnd)) {
        if (!IsWindowVisible(s_cam_hwnd) || camX != s_last_cam_x || camY != s_last_cam_y || camW != s_last_cam_w || camH != s_last_cam_h) {
            s_last_cam_x = camX; s_last_cam_y = camY; s_last_cam_w = camW; s_last_cam_h = camH;
            SetWindowPos(s_cam_hwnd, HWND_TOP, camX, camY, camW, camH,
                         SWP_NOACTIVATE | SWP_SHOWWINDOW);
        }
    }

    if (s_menu_hwnd && IsWindow(s_menu_hwnd)) {
        if (!IsWindowVisible(s_menu_hwnd) || menuX != s_last_menu_x || menuY != s_last_menu_y || menuW != s_last_menu_w || menuH != s_last_menu_h) {
            s_last_menu_x = menuX; s_last_menu_y = menuY; s_last_menu_w = menuW; s_last_menu_h = menuH;
            SetWindowPos(s_menu_hwnd, HWND_TOP, menuX, menuY, menuW, menuH,
                         SWP_NOACTIVATE | SWP_SHOWWINDOW);
        }
    }

    if (s_toast_hwnd && IsWindow(s_toast_hwnd) && IsWindowVisible(s_toast_hwnd)) {
        RECT r;
        GetWindowRect(s_toast_hwnd, &r);
        int toastW = r.right - r.left;
        int toastH = r.bottom - r.top;
        int toastX = winRect.left + (winWidth - toastW) / 2;
        int toastY = winRect.top + 48;
        SetWindowPos(s_toast_hwnd, HWND_TOPMOST, toastX, toastY, toastW, toastH,
                     SWP_NOACTIVATE | SWP_NOZORDER);
    }

    s_in_reposition = false;
}

static LRESULT CALLBACK
MainWndSubclassProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_WINDOWPOSCHANGED:
        case WM_MOVE:
        case WM_MOVING:
        case WM_SIZE:
        case WM_SIZING:
        case WM_ENTERSIZEMOVE:
        case WM_EXITSIZEMOVE:
        case WM_SHOWWINDOW: {
            reposition_controls();
            break;
        }
        case WM_KILLFOCUS:
        case WM_SETFOCUS:
        case WM_ACTIVATE:
        case WM_NCACTIVATE: {
            if (s_cam_hwnd && IsWindow(s_cam_hwnd)) {
                SetWindowPos(s_cam_hwnd, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
                InvalidateRect(s_cam_hwnd, NULL, FALSE);
            }
            if (s_menu_hwnd && IsWindow(s_menu_hwnd)) {
                SetWindowPos(s_menu_hwnd, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
                InvalidateRect(s_menu_hwnd, NULL, FALSE);
            }
            break;
        }
        case WM_NCRBUTTONUP: {
            if (wParam == HTCAPTION) {
                POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
                show_flux_menu_at_point(pt.x, pt.y);
                return 0;
            }
            break;
        }
        case WM_DESTROY: {
            sc_titlebar_camera_destroy();
            break;
        }
    }
    if (s_orig_wndproc) {
        return CallWindowProc(s_orig_wndproc, hwnd, uMsg, wParam, lParam);
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

// ---------------------------------------------------------------------------
// PUBLIC API IMPLEMENTATION
// ---------------------------------------------------------------------------
void
sc_titlebar_camera_init(SDL_Window *window, const char *serial) {
    if (!window) {
        return;
    }

    SDL_PropertiesID props = SDL_GetWindowProperties(window);
    s_main_hwnd = (HWND) SDL_GetPointerProperty(props, SDL_PROP_WINDOW_WIN32_HWND_POINTER, NULL);
    if (!s_main_hwnd || !IsWindow(s_main_hwnd)) {
        LOGE("Failed to obtain Win32 HWND from SDL_Window");
        return;
    }

    if (serial && serial[0] != '\0') {
        strncpy(s_serial, serial, sizeof(s_serial) - 1);
    }

    start_device_model_discovery();

    // 1. Enable Windows 10/11 immersive dark mode on title bar
    BOOL darkMode = TRUE;
    DwmSetWindowAttribute(s_main_hwnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &darkMode, sizeof(darkMode));
    // 1b. Suppress native DWM title text and icon
    typedef struct {
        DWORD dwFlags;
        DWORD dwMask;
    } WTA_OPTIONS_LOCAL;
    typedef HRESULT (WINAPI *PFN_SetWindowThemeAttribute)(HWND, DWORD, PVOID, DWORD);
    HMODULE hUxTheme = LoadLibraryA("uxtheme.dll");
    if (hUxTheme) {
        PFN_SetWindowThemeAttribute pfn = (PFN_SetWindowThemeAttribute) GetProcAddress(hUxTheme, "SetWindowThemeAttribute");
        if (pfn) {
            WTA_OPTIONS_LOCAL opts;
            opts.dwFlags = 0x00000001 /* WTNCA_NODRAWCAPTION */ | 0x00000002 /* WTNCA_NODRAWICON */;
            opts.dwMask = 0x00000001 | 0x00000002;
            pfn(s_main_hwnd, 1 /* WTA_NONCLIENT */, &opts, sizeof(opts));
        }
        FreeLibrary(hUxTheme);
    }

    // 2. Remove Maximize button from Window Style & enforce HWND_TOPMOST
    LONG_PTR style = GetWindowLongPtr(s_main_hwnd, GWL_STYLE);
    style &= ~WS_MAXIMIZEBOX;
    SetWindowLongPtr(s_main_hwnd, GWL_STYLE, style);
    SetWindowPos(s_main_hwnd, HWND_TOPMOST, 0, 0, 0, 0,
                 SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_FRAMECHANGED);

    // 3. Remove SC_MAXIMIZE from system menu
    HMENU hMenu = GetSystemMenu(s_main_hwnd, FALSE);
    if (hMenu) {
        DeleteMenu(hMenu, SC_MAXIMIZE, MF_BYCOMMAND);
    }

    // 4. Set application icon
    HINSTANCE hInstance = GetModuleHandle(NULL);
    s_app_icon = (HICON) LoadImage(hInstance, MAKEINTRESOURCE(0), IMAGE_ICON, 16, 16, 0);
    if (s_app_icon) {
        SendMessage(s_main_hwnd, WM_SETICON, ICON_SMALL, (LPARAM) s_app_icon);
    }
    HICON hBigIcon = (HICON) LoadImage(hInstance, MAKEINTRESOURCE(0), IMAGE_ICON, 32, 32, 0);
    if (hBigIcon) {
        SendMessage(s_main_hwnd, WM_SETICON, ICON_BIG, (LPARAM) hBigIcon);
    }

    // 5. Register window classes
    WNDCLASSEX wcCam;
    memset(&wcCam, 0, sizeof(wcCam));
    wcCam.cbSize = sizeof(WNDCLASSEX);
    wcCam.lpfnWndProc = CameraBtnWndProc;
    wcCam.hInstance = hInstance;
    wcCam.hCursor = LoadCursor(NULL, IDC_HAND);
    wcCam.lpszClassName = "FluxCameraTitleBtn";
    RegisterClassEx(&wcCam);

    WNDCLASSEX wcMenu;
    memset(&wcMenu, 0, sizeof(wcMenu));
    wcMenu.cbSize = sizeof(WNDCLASSEX);
    wcMenu.lpfnWndProc = MenuBtnWndProc;
    wcMenu.hInstance = hInstance;
    wcMenu.hCursor = LoadCursor(NULL, IDC_HAND);
    wcMenu.lpszClassName = "FluxBrandMenuBtn";
    RegisterClassEx(&wcMenu);

    WNDCLASSEX wcToast;
    memset(&wcToast, 0, sizeof(wcToast));
    wcToast.cbSize = sizeof(WNDCLASSEX);
    wcToast.lpfnWndProc = ToastWndProc;
    wcToast.hInstance = hInstance;
    wcToast.hCursor = LoadCursor(NULL, IDC_ARROW);
    wcToast.lpszClassName = "FluxToastNotification";
    RegisterClassEx(&wcToast);

    // 6. Create controls (owned by s_main_hwnd, always topmost)
    s_cam_hwnd = CreateWindowEx(
        WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE | WS_EX_TOPMOST,
        "FluxCameraTitleBtn", "CameraBtn",
        WS_POPUP | WS_VISIBLE,
        -1000, -1000, 47, 29, s_main_hwnd, NULL, hInstance, NULL
    );

    s_menu_hwnd = CreateWindowEx(
        WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE | WS_EX_TOPMOST,
        "FluxBrandMenuBtn", "BrandMenuBtn",
        WS_POPUP | WS_VISIBLE,
        -1000, -1000, 114, 26, s_main_hwnd, NULL, hInstance, NULL
    );

    // 7. Tooltips
    s_tooltip_hwnd = CreateWindowEx(
        WS_EX_TOPMOST,
        TOOLTIPS_CLASS, NULL,
        WS_POPUP | TTS_NOPREFIX | TTS_ALWAYSTIP,
        CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
        s_main_hwnd, NULL, hInstance, NULL
    );
    if (s_tooltip_hwnd) {
        TOOLINFOA ti;
        memset(&ti, 0, sizeof(ti));
        ti.cbSize = sizeof(ti);
        ti.uFlags = TTF_SUBCLASS | TTF_IDISHWND;

        ti.hwnd = s_cam_hwnd;
        ti.uId = (UINT_PTR) s_cam_hwnd;
        ti.lpszText = "Take Screenshot (Copy to Clipboard)";
        SendMessage(s_tooltip_hwnd, TTM_ADDTOOLA, 0, (LPARAM) &ti);

        ti.hwnd = s_menu_hwnd;
        ti.uId = (UINT_PTR) s_menu_hwnd;
        ti.lpszText = "FluxMirror Menu (Devices, Wireless ADB, Controls)";
        SendMessage(s_tooltip_hwnd, TTM_ADDTOOLA, 0, (LPARAM) &ti);
    }

    // 8. Create Toast Window
    s_toast_hwnd = CreateWindowEx(
        WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE | WS_EX_TOPMOST,
        "FluxToastNotification", "FluxToast",
        WS_POPUP,
        -1000, -1000, 210, 38, s_main_hwnd, NULL, hInstance, NULL
    );

    // 9. Subclass main window procedure
    s_orig_wndproc = (WNDPROC) SetWindowLongPtr(s_main_hwnd, GWLP_WNDPROC, (LONG_PTR) MainWndSubclassProc);

    // 10. Start continuous heartbeat timer on camera window to follow fast dragging
    SetTimer(s_cam_hwnd, SYNC_TIMER_ID, 33, NULL);

    reposition_controls();
    LOGI("Native titlebar camera and brand menu initialized successfully");
}

void
sc_titlebar_camera_destroy(void) {
    if (s_tooltip_hwnd && IsWindow(s_tooltip_hwnd)) {
        DestroyWindow(s_tooltip_hwnd);
        s_tooltip_hwnd = NULL;
    }
    if (s_cam_hwnd && IsWindow(s_cam_hwnd)) {
        KillTimer(s_cam_hwnd, SYNC_TIMER_ID);
        DestroyWindow(s_cam_hwnd);
        s_cam_hwnd = NULL;
    }
    if (s_menu_hwnd && IsWindow(s_menu_hwnd)) {
        DestroyWindow(s_menu_hwnd);
        s_menu_hwnd = NULL;
    }
    if (s_toast_hwnd && IsWindow(s_toast_hwnd)) {
        DestroyWindow(s_toast_hwnd);
        s_toast_hwnd = NULL;
    }
    if (s_main_hwnd && IsWindow(s_main_hwnd) && s_orig_wndproc) {
        SetWindowLongPtr(s_main_hwnd, GWLP_WNDPROC, (LONG_PTR) s_orig_wndproc);
        s_orig_wndproc = NULL;
    }
    s_main_hwnd = NULL;
}

#endif // _WIN32
