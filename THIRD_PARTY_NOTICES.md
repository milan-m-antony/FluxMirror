# Third-Party Software Notices and Licenses

FluxMirror incorporates or distributes third-party software components subject to separate license terms. This document details the upstream projects, copyright holders, and license conditions governing each component.

---

## 1. Upstream Engine: scrcpy & scrcpy-server

- **Project**: [scrcpy](https://github.com/Genymobile/scrcpy)
- **Copyright**: 
  - Copyright (C) 2018 Genymobile
  - Copyright (C) 2018–2024 Romain Vimont (`rom@rom1v.com`)
  - scrcpy contributors
- **License**: [Apache License, Version 2.0](http://www.apache.org/licenses/LICENSE-2.0)
- **Notice**: FluxMirror is a derivative work extending scrcpy with native Windows title bar camera notch screenshot capture, unified brand control center, and wireless device switching.

---

## 2. Android Debug Bridge (ADB) & Platform-Tools

- **Project**: [Android Open Source Project (AOSP)](https://android.googlesource.com/platform/packages/modules/adb/)
- **Copyright**: Copyright (C) 2006–2024 The Android Open Source Project / Google LLC
- **License**: [Apache License, Version 2.0](http://www.apache.org/licenses/LICENSE-2.0)
- **Distributed Binaries**: `adb.exe`, `AdbWinApi.dll`, `AdbWinUsbApi.dll`

---

## 3. Simple DirectMedia Layer (SDL3)

- **Project**: [SDL (Simple DirectMedia Layer)](https://www.libsdl.org/)
- **Copyright**: Copyright (C) 1997–2024 Sam Lantinga and SDL contributors
- **License**: [zlib License](https://www.zlib.net/zlib_license.html)
- **Notice**:
  ```text
  This software is provided 'as-is', without any express or implied warranty. In no event will the authors be held liable for any damages arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose, including commercial applications, and to alter it and redistribute it freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not claim that you wrote the original software. If you use this software in a product, an acknowledgment in the product documentation would be appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.
  ```

---

## 4. FFmpeg (libavcodec, libavformat, libavutil, libswresample)

- **Project**: [FFmpeg](https://ffmpeg.org/)
- **Copyright**: Copyright (C) 2000–2024 FFmpeg developers
- **License**: [GNU Lesser General Public License (LGPL), version 2.1 or later](https://www.gnu.org/licenses/old-licenses/lgpl-2.1.html)
- **Notice**: FluxMirror links dynamically against shared FFmpeg libraries (`avcodec-*.dll`, `avformat-*.dll`, `avutil-*.dll`, `swresample-*.dll`). FFmpeg source code can be obtained at <https://ffmpeg.org/download.html>. FluxMirror does not modify FFmpeg source code.

---

## 5. libusb

- **Project**: [libusb](https://libusb.info/)
- **Copyright**: Copyright (C) 2007–2024 libusb contributors
- **License**: [GNU Lesser General Public License (LGPL), version 2.1 or later](https://www.gnu.org/licenses/old-licenses/lgpl-2.1.html)
- **Notice**: FluxMirror links dynamically against `libusb-1.0.dll` for USB AOA input emulation. Source code for libusb is available at <https://github.com/libusb/libusb>.
