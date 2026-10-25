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
#define TOAST_DURATION_MS 1800
#define SYNC_TIMER_ID 1002
#define WM_SHOW_TOAST (WM_USER + 101)

typedef struct {
    DWORD cbSize;
    RECT rcTitleBar;
    DWORD rgstate[6];
    RECT rgrect[6];
} TITLEBARINFOEX_LOCAL;

static HWND s_main_hwnd = NULL;
static HWND s_cam_hwnd = NULL;
static HWND s_toast_hwnd = NULL;
static HWND s_tooltip_hwnd = NULL;
static WNDPROC s_orig_wndproc = NULL;
static char s_serial[128] = {0};

static bool s_btn_hovered = false;
static bool s_btn_pressed = false;
static bool s_is_capturing = false;

static void reposition_controls(void);

// ---------------------------------------------------------------------------
// RESOLVE BUNDLED ADB PATH
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

// ---------------------------------------------------------------------------
// SCREENSHOT CAPTURE THREAD (Silent ADB -> PNG -> FFmpeg/SDL -> Clipboard)
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

    SDL_Surface *surface = sc_icon_load_from_full_path(temp_file);
    DeleteFileA(temp_file);

    if (!surface) {
        LOGE("Failed to decode screenshot image");
        s_is_capturing = false;
        return 0;
    }

    int w = surface->w;
    int h = surface->h;

    BITMAPINFO bmi;
    memset(&bmi, 0, sizeof(bmi));
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = w;
    bmi.bmiHeader.biHeight = -h; // Top-down DIB
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    HDC hdc = GetDC(NULL);
    void *pBits = NULL;
    HBITMAP hBitmap = CreateDIBSection(hdc, &bmi, DIB_RGB_COLORS, &pBits, NULL, 0);
    if (hBitmap && pBits) {
        bool ok = SDL_ConvertPixels(w, h, surface->format, surface->pixels, surface->pitch,
                                    SDL_PIXELFORMAT_BGRA32, pBits, w * 4);
        if (ok) {
            if (OpenClipboard(s_main_hwnd)) {
                EmptyClipboard();
                SetClipboardData(CF_BITMAP, hBitmap);
                CloseClipboard();
                MessageBeep(MB_OK);
                LOGI("Screenshot copied to Windows clipboard: %dx%d", w, h);

                if (s_cam_hwnd && IsWindow(s_cam_hwnd)) {
                    PostMessage(s_cam_hwnd, WM_SHOW_TOAST, 0, 0);
                }
            } else {
                DeleteObject(hBitmap);
            }
        } else {
            DeleteObject(hBitmap);
        }
    }
    ReleaseDC(NULL, hdc);
    SDL_DestroySurface(surface);

    s_is_capturing = false;
    return 0;
}

static void
trigger_capture(void) {
    if (s_is_capturing) {
        return;
    }
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
static LRESULT CALLBACK
ToastWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_CREATE: {
            HRGN hRgn = CreateRoundRectRgn(0, 0, 210, 38, 19, 19);
            SetWindowRgn(hwnd, hRgn, TRUE);
            return 0;
        }
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

            // Double buffering
            HDC memDC = CreateCompatibleDC(hdc);
            HBITMAP memBmp = CreateCompatibleBitmap(hdc, r.right, r.bottom);
            HBITMAP oldBmp = (HBITMAP) SelectObject(memDC, memBmp);

            // Obsidian dark background (#1E293B)
            HBRUSH bgBrush = CreateSolidBrush(RGB(30, 41, 59));
            FillRect(memDC, &r, bgBrush);
            DeleteObject(bgBrush);

            // Flux Orange border (#FF6B00)
            HPEN borderPen = CreatePen(PS_SOLID, 2, RGB(255, 107, 0));
            HPEN oldPen = (HPEN) SelectObject(memDC, borderPen);
            HBRUSH nullBrush = (HBRUSH) GetStockObject(NULL_BRUSH);
            HBRUSH oldBrush = (HBRUSH) SelectObject(memDC, nullBrush);
            RoundRect(memDC, r.left + 1, r.top + 1, r.right - 1, r.bottom - 1, 18, 18);
            SelectObject(memDC, oldBrush);
            SelectObject(memDC, oldPen);
            DeleteObject(borderPen);

            // Emerald green checkmark badge (#10B981)
            HBRUSH badgeBrush = CreateSolidBrush(RGB(16, 185, 129));
            HBRUSH oldBadgeBrush = (HBRUSH) SelectObject(memDC, badgeBrush);
            HPEN nullPen = (HPEN) GetStockObject(NULL_PEN);
            HPEN oldBadgePen = (HPEN) SelectObject(memDC, nullPen);
            Ellipse(memDC, 14, 10, 32, 28);
            SelectObject(memDC, oldBadgeBrush);
            SelectObject(memDC, oldBadgePen);
            DeleteObject(badgeBrush);

            // Checkmark symbol
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

            // Text "Copied to clipboard"
            HFONT hFontText = CreateFontW(14, 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE,
                                          DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
                                          CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
                                          DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
            oldFont = (HFONT) SelectObject(memDC, hFontText);
            RECT textRect = { 38, 0, r.right - 10, r.bottom };
            DrawTextW(memDC, L"Copied to clipboard", -1, &textRect, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
            SelectObject(memDC, oldFont);
            DeleteObject(hFontText);

            // Blit to screen
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
                RECT winRect;
                GetWindowRect(s_main_hwnd, &winRect);
                int winW = winRect.right - winRect.left;
                int toastW = 210;
                int toastH = 38;
                int toastX = winRect.left + (winW - toastW) / 2;
                int toastY = winRect.top + 48; // Centered below title bar

                SetWindowPos(s_toast_hwnd, HWND_TOPMOST, toastX, toastY, toastW, toastH,
                             SWP_NOACTIVATE | SWP_SHOWWINDOW);
                SetTimer(s_toast_hwnd, TOAST_TIMER_ID, TOAST_DURATION_MS, NULL);
            }
            return 0;
        }
        case WM_MOUSEMOVE: {
            if (!s_btn_hovered) {
                s_btn_hovered = true;
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
            s_btn_hovered = false;
            s_btn_pressed = false;
            InvalidateRect(hwnd, NULL, FALSE);
            return 0;
        }
        case WM_LBUTTONDOWN: {
            s_btn_pressed = true;
            SetCapture(hwnd);
            InvalidateRect(hwnd, NULL, FALSE);
            return 0;
        }
        case WM_LBUTTONUP: {
            if (s_btn_pressed) {
                ReleaseCapture();
                s_btn_pressed = false;
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

            // Match active/inactive caption background
            HWND activeWnd = GetActiveWindow();
            bool isActive = (activeWnd == s_main_hwnd || activeWnd == s_cam_hwnd);

            COLORREF bgColor;
            COLORREF glyphColor;

            if (s_btn_pressed) {
                bgColor = RGB(38, 38, 38);
                glyphColor = RGB(255, 140, 50); // Lighter Flux Orange on press
            } else if (s_btn_hovered) {
                bgColor = RGB(50, 50, 50);      // DWM hover highlight
                glyphColor = RGB(255, 107, 0);  // Signature Flux Orange
            } else {
                bgColor = isActive ? RGB(32, 32, 32) : RGB(43, 43, 43);
                glyphColor = isActive ? RGB(235, 235, 235) : RGB(170, 170, 170);
            }

            HBRUSH bgBrush = CreateSolidBrush(bgColor);
            FillRect(memDC, &r, bgBrush);
            DeleteObject(bgBrush);

            // Draw crisp camera vector glyph with top notch
            HPEN camPen = CreatePen(PS_SOLID, 1, glyphColor);
            HPEN oldPen = (HPEN) SelectObject(memDC, camPen);
            HBRUSH nullBrush = (HBRUSH) GetStockObject(NULL_BRUSH);
            HBRUSH oldBrush = (HBRUSH) SelectObject(memDC, nullBrush);

            int cx = r.right / 2;
            int cy = r.bottom / 2;

            // Camera body outline
            RoundRect(memDC, cx - 8, cy - 4, cx + 8, cy + 8, 3, 3);

            // Camera top notch
            POINT topPts[4] = {
                { cx - 4, cy - 4 },
                { cx - 2, cy - 7 },
                { cx + 2, cy - 7 },
                { cx + 4, cy - 4 }
            };
            Polyline(memDC, topPts, 4);

            // Lens circle
            Ellipse(memDC, cx - 3, cy - 1, cx + 3, cy + 5);

            // Flash dot
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
// WINDOW MESSAGE HOOK & POSITION SYNCHRONIZATION
// ---------------------------------------------------------------------------
static void
reposition_controls(void) {
    if (!s_main_hwnd || !IsWindow(s_main_hwnd)) {
        return;
    }

    if (IsIconic(s_main_hwnd) || !IsWindowVisible(s_main_hwnd)) {
        if (s_cam_hwnd && IsWindow(s_cam_hwnd)) {
            ShowWindow(s_cam_hwnd, SW_HIDE);
        }
        if (s_toast_hwnd && IsWindow(s_toast_hwnd)) {
            ShowWindow(s_toast_hwnd, SW_HIDE);
        }
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

    int camX, camY, camW, camH;

    // Directly replace Maximize button rectangle if reported
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

    if (s_cam_hwnd && IsWindow(s_cam_hwnd)) {
        SetWindowPos(s_cam_hwnd, s_main_hwnd, camX, camY, camW, camH,
                     SWP_NOACTIVATE | SWP_NOZORDER | SWP_SHOWWINDOW);
    }

    if (s_toast_hwnd && IsWindow(s_toast_hwnd) && IsWindowVisible(s_toast_hwnd)) {
        int winW = winRect.right - winRect.left;
        int toastW = 210;
        int toastH = 38;
        int toastX = winRect.left + (winW - toastW) / 2;
        int toastY = winRect.top + 48;
        SetWindowPos(s_toast_hwnd, HWND_TOPMOST, toastX, toastY, toastW, toastH,
                     SWP_NOACTIVATE | SWP_NOZORDER);
    }
}

static LRESULT CALLBACK
MainWndSubclassProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_WINDOWPOSCHANGING:
        case WM_WINDOWPOSCHANGED:
        case WM_MOVE:
        case WM_MOVING:
        case WM_SIZE:
        case WM_SIZING:
        case WM_ENTERSIZEMOVE:
        case WM_EXITSIZEMOVE:
        case WM_SHOWWINDOW:
        case WM_ACTIVATE:
        case WM_NCACTIVATE: {
            reposition_controls();
            if (s_cam_hwnd && IsWindow(s_cam_hwnd)) {
                InvalidateRect(s_cam_hwnd, NULL, FALSE);
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

    // 1. Enable Windows 10/11 immersive dark mode on title bar
    BOOL darkMode = TRUE;
    DwmSetWindowAttribute(s_main_hwnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &darkMode, sizeof(darkMode));
    DwmSetWindowAttribute(s_main_hwnd, 19, &darkMode, sizeof(darkMode)); // Fallback for Win10 1903/1909

    // 2. Remove Maximize button from Window Style
    LONG_PTR style = GetWindowLongPtr(s_main_hwnd, GWL_STYLE);
    style &= ~WS_MAXIMIZEBOX;
    SetWindowLongPtr(s_main_hwnd, GWL_STYLE, style);
    SetWindowPos(s_main_hwnd, NULL, 0, 0, 0, 0,
                 SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);

    // 3. Remove SC_MAXIMIZE from system menu
    HMENU hMenu = GetSystemMenu(s_main_hwnd, FALSE);
    if (hMenu) {
        DeleteMenu(hMenu, SC_MAXIMIZE, MF_BYCOMMAND);
    }

    // 4. Set Black Flux application icon
    HINSTANCE hInstance = GetModuleHandle(NULL);
    HICON hIcon = (HICON) LoadImage(hInstance, MAKEINTRESOURCE(0), IMAGE_ICON, 16, 16, 0);
    if (hIcon) {
        SendMessage(s_main_hwnd, WM_SETICON, ICON_SMALL, (LPARAM) hIcon);
    }
    HICON hBigIcon = (HICON) LoadImage(hInstance, MAKEINTRESOURCE(0), IMAGE_ICON, 32, 32, 0);
    if (hBigIcon) {
        SendMessage(s_main_hwnd, WM_SETICON, ICON_BIG, (LPARAM) hBigIcon);
    }

    // 5. Register Camera Button Class
    WNDCLASSEX wcCam;
    memset(&wcCam, 0, sizeof(wcCam));
    wcCam.cbSize = sizeof(WNDCLASSEX);
    wcCam.lpfnWndProc = CameraBtnWndProc;
    wcCam.hInstance = hInstance;
    wcCam.hCursor = LoadCursor(NULL, IDC_HAND);
    wcCam.lpszClassName = "FluxCameraTitleBtn";
    RegisterClassEx(&wcCam);

    // 6. Register Toast Class
    WNDCLASSEX wcToast;
    memset(&wcToast, 0, sizeof(wcToast));
    wcToast.cbSize = sizeof(WNDCLASSEX);
    wcToast.lpfnWndProc = ToastWndProc;
    wcToast.hInstance = hInstance;
    wcToast.hCursor = LoadCursor(NULL, IDC_ARROW);
    wcToast.lpszClassName = "FluxToastNotification";
    RegisterClassEx(&wcToast);

    // 7. Create Camera Button Window (owned by s_main_hwnd)
    s_cam_hwnd = CreateWindowEx(
        WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE,
        "FluxCameraTitleBtn",
        "CameraBtn",
        WS_POPUP | WS_VISIBLE,
        -1000, -1000, 47, 29,
        s_main_hwnd,
        NULL,
        hInstance,
        NULL
    );

    // 8. Create Native Tooltip
    s_tooltip_hwnd = CreateWindowEx(
        WS_EX_TOPMOST,
        TOOLTIPS_CLASS,
        NULL,
        WS_POPUP | TTS_NOPREFIX | TTS_ALWAYSTIP,
        CW_USEDEFAULT, CW_USEDEFAULT,
        CW_USEDEFAULT, CW_USEDEFAULT,
        s_cam_hwnd,
        NULL,
        hInstance,
        NULL
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
    }

    // 9. Create Toast Window (owned by s_main_hwnd)
    s_toast_hwnd = CreateWindowEx(
        WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE,
        "FluxToastNotification",
        "FluxToast",
        WS_POPUP,
        -1000, -1000, 210, 38,
        s_main_hwnd,
        NULL,
        hInstance,
        NULL
    );

    // 10. Subclass main window procedure for instant synchronous position updates
    s_orig_wndproc = (WNDPROC) SetWindowLongPtr(s_main_hwnd, GWLP_WNDPROC, (LONG_PTR) MainWndSubclassProc);

    // 11. Start continuous 60 FPS heartbeat timer on camera window to follow fast dragging
    SetTimer(s_cam_hwnd, SYNC_TIMER_ID, 16, NULL);

    // Position immediately
    reposition_controls();

    LOGI("Native titlebar camera screenshot button initialized successfully");
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
