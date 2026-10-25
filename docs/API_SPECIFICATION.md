# Android Web Studio — API Specification

The Node.js Vite middleware (`server/adbBridge.ts`) exposes the following local HTTP REST endpoints:

---

## 1. List Connected Devices

### `GET /api/devices`
Returns all devices currently attached to the ADB daemon.

#### Response `200 OK`
```json
{
  "devices": [
    {
      "id": "RZCX71Q65BB",
      "status": "device"
    }
  ]
}
```

---

## 2. Get Device Telemetry

### `GET /api/device-info`
Queries device properties (`getprop`), window manager dimensions (`wm size`), and battery metrics (`dumpsys battery`).

#### Response `200 OK`
```json
{
  "device": "RZCX71Q65BB",
  "model": "SM-G990B2",
  "manufacturer": "samsung",
  "osVersion": "14",
  "width": 1080,
  "height": 2340,
  "batteryLevel": 85,
  "isCharging": true
}
```

#### Response `404 Not Found`
```json
{
  "error": "No device connected"
}
```

---

## 3. Capture Screen Frame

### `GET /api/frame`
Executes an on-the-fly screencap on the active Android device and pipes the raw PNG binary stream directly to the client.

#### Query Parameters
- `t`: (Optional) Timestamp integer (e.g. `?t=1730000000000`) used for browser cache busting.

#### Response Headers
- `Content-Type: image/png`
- `Cache-Control: no-cache, no-store, must-revalidate`

---

## 4. Dispatch Input Action

### `POST /api/action`
Dispatches input interactions to the device via `adb shell input`.

#### Request Headers
- `Content-Type: application/json`

#### Request Payloads

##### A. Touch Tap
```json
{
  "type": "tap",
  "x": 540,
  "y": 1170
}
```

##### B. Swipe / Drag / Scroll
```json
{
  "type": "swipe",
  "x1": 540,
  "y1": 1500,
  "x2": 540,
  "y2": 700,
  "duration": 250
}
```

##### C. Key Event (Hardware / Navigation Key)
```json
{
  "type": "key",
  "code": 4
}
```
*Common KeyCodes: `3` (Home), `4` (Back), `24` (Volume Up), `25` (Volume Down), `26` (Power), `187` (App Switch), `224` (Wake).*

##### D. Text Input
```json
{
  "type": "text",
  "text": "Hello World"
}
```

#### Response `200 OK`
```json
{
  "ok": true
}
```
