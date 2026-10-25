import React, { useEffect, useRef, useState, useCallback } from 'react';
import { createPortal } from 'react-dom';
import {
  Camera,
  Download,
  RotateCw,
  Power,
  Volume2,
  Volume1,
  Smartphone,
  BatteryCharging,
  Battery,
  Flame,
  CheckCircle2,
  RefreshCw,
  Sliders,
  ChevronLeft,
  Circle,
  Square,
  ExternalLink,
  Wifi,
  Send,
  Zap,
  Laptop
} from 'lucide-react';

interface DeviceItem {
  id: string;
  status: string;
  model?: string;
}

interface DeviceInfo {
  device: string;
  model: string;
  manufacturer: string;
  osVersion: string;
  width: number;
  height: number;
  batteryLevel: number;
  isCharging: boolean;
  wifiIp?: string;
}

export function App() {
  const [devices, setDevices] = useState<DeviceItem[]>([]);
  const [selectedDevice, setSelectedDevice] = useState<string>('');
  const [deviceInfo, setDeviceInfo] = useState<DeviceInfo | null>(null);
  const [isConnected, setIsConnected] = useState<boolean>(false);
  const [fps, setFps] = useState<number>(0);
  const [latency, setLatency] = useState<number>(0);
  const [toastMessage, setToastMessage] = useState<string | null>(null);
  const [isPaused, setIsPaused] = useState<boolean>(false);
  const [turboMode, setTurboMode] = useState<boolean>(false);
  const [isInteracting, setIsInteracting] = useState<boolean>(false);
  const [textInput, setTextInput] = useState<string>('');
  const [touchRipple, setTouchRipple] = useState<{ x: number; y: number } | null>(null);
  const [pipWindow, setPipWindow] = useState<Window | null>(null);

  const canvasRef = useRef<HTMLCanvasElement | null>(null);
  const pointerStartRef = useRef<{ x: number; y: number; time: number } | null>(null);
  const frameCountRef = useRef<number>(0);
  const lastFpsTimeRef = useRef<number>(performance.now());
  const isFetchingRef = useRef<boolean>(false);

  // Check if running in dedicated float/popup mode
  const isFloatOnly = new URLSearchParams(window.location.search).get('float') === 'true';

  const showToast = (msg: string) => {
    setToastMessage(msg);
    setTimeout(() => setToastMessage(null), 2800);
  };

  // Fetch connected devices list
  const fetchDevices = useCallback(async () => {
    try {
      const res = await fetch('/api/devices');
      if (res.ok) {
        const data = await res.json();
        setDevices(data.devices || []);
        if (data.devices?.length > 0 && !selectedDevice) {
          setSelectedDevice(data.devices[0].id);
        }
      }
    } catch {
      // ignore
    }
  }, [selectedDevice]);

  // Fetch device detailed telemetry
  const fetchDeviceInfo = useCallback(async () => {
    try {
      const url = selectedDevice ? `/api/device-info?device=${selectedDevice}` : '/api/device-info';
      const res = await fetch(url);
      if (res.ok) {
        const data = await res.json();
        setDeviceInfo(data);
        setIsConnected(true);
      } else {
        setIsConnected(false);
      }
    } catch {
      setIsConnected(false);
    }
  }, [selectedDevice]);

  useEffect(() => {
    fetchDevices();
    fetchDeviceInfo();
    const interval = setInterval(() => {
      fetchDevices();
      fetchDeviceInfo();
    }, 10000);
    return () => clearInterval(interval);
  }, [fetchDevices, fetchDeviceInfo]);

  // Frame streaming loop
  useEffect(() => {
    if (isPaused) return;
    let isCancelled = false;
    const intervalDelay = turboMode ? 50 : 120;

    const pullNextFrame = async () => {
      if (isCancelled || isFetchingRef.current) return;
      isFetchingRef.current = true;
      const startTime = performance.now();

      try {
        const devParam = selectedDevice ? `&device=${selectedDevice}` : '';
        const res = await fetch(`/api/frame?t=${Date.now()}${devParam}`);
        if (res.ok) {
          const blob = await res.blob();
          const imgBitmap = await createImageBitmap(blob);

          if (canvasRef.current && !isCancelled) {
            const canvas = canvasRef.current;
            if (canvas.width !== imgBitmap.width || canvas.height !== imgBitmap.height) {
              canvas.width = imgBitmap.width;
              canvas.height = imgBitmap.height;
            }
            const ctx = canvas.getContext('2d');
            if (ctx) {
              ctx.drawImage(imgBitmap, 0, 0);
            }
          }

          const elapsed = performance.now() - startTime;
          setLatency(Math.round(elapsed));
          frameCountRef.current++;

          const now = performance.now();
          if (now - lastFpsTimeRef.current >= 1000) {
            setFps(frameCountRef.current);
            frameCountRef.current = 0;
            lastFpsTimeRef.current = now;
          }

          setIsConnected(true);
        }
      } catch {
        // stream retry
      } finally {
        isFetchingRef.current = false;
        if (!isCancelled && !isPaused) {
          setTimeout(pullNextFrame, intervalDelay);
        }
      }
    };

    pullNextFrame();

    return () => {
      isCancelled = true;
    };
  }, [isPaused, turboMode, selectedDevice]);

  // Dispatch Action via API
  const sendAction = async (payload: any) => {
    try {
      await fetch('/api/action', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ ...payload, device: selectedDevice }),
      });
    } catch (err) {
      console.error('Failed to dispatch action', err);
    }
  };

  // Convert canvas pointer coordinates to device coordinates
  const getDeviceCoords = (e: React.PointerEvent<HTMLCanvasElement>) => {
    const canvas = canvasRef.current;
    if (!canvas) return { x: 0, y: 0 };
    const rect = canvas.getBoundingClientRect();
    const scaleX = canvas.width / rect.width;
    const scaleY = canvas.height / rect.height;

    const x = (e.clientX - rect.left) * scaleX;
    const y = (e.clientY - rect.top) * scaleY;
    return { x, y };
  };

  const handlePointerDown = (e: React.PointerEvent<HTMLCanvasElement>) => {
    const coords = getDeviceCoords(e);
    pointerStartRef.current = { x: coords.x, y: coords.y, time: performance.now() };
    setIsInteracting(true);

    const canvas = canvasRef.current;
    if (canvas) {
      const rect = canvas.getBoundingClientRect();
      setTouchRipple({ x: e.clientX - rect.left, y: e.clientY - rect.top });
      setTimeout(() => setTouchRipple(null), 400);
    }
  };

  const handlePointerUp = (e: React.PointerEvent<HTMLCanvasElement>) => {
    setIsInteracting(false);
    if (!pointerStartRef.current) return;
    const endCoords = getDeviceCoords(e);
    const startCoords = pointerStartRef.current;
    const duration = Math.min(800, Math.max(100, Math.round(performance.now() - startCoords.time)));

    const dx = endCoords.x - startCoords.x;
    const dy = endCoords.y - startCoords.y;
    const distance = Math.sqrt(dx * dx + dy * dy);

    if (distance < 15) {
      sendAction({ type: 'tap', x: endCoords.x, y: endCoords.y });
    } else {
      sendAction({
        type: 'swipe',
        x1: startCoords.x,
        y1: startCoords.y,
        x2: endCoords.x,
        y2: endCoords.y,
        duration,
      });
    }
    pointerStartRef.current = null;
  };

  const handleWheel = (e: React.WheelEvent<HTMLCanvasElement>) => {
    e.preventDefault();
    const canvas = canvasRef.current;
    if (!canvas) return;
    const midX = canvas.width / 2;
    const midY = canvas.height / 2;
    const scrollDelta = e.deltaY > 0 ? -450 : 450;

    sendAction({
      type: 'swipe',
      x1: midX,
      y1: midY,
      x2: midX,
      y2: midY + scrollDelta,
      duration: 160,
    });
  };

  // Instant In-Memory Screenshot Copy
  const handleCopyScreenshot = async () => {
    const canvas = canvasRef.current;
    if (!canvas) return;

    try {
      canvas.toBlob(async (blob) => {
        if (!blob) return;
        try {
          await navigator.clipboard.write([
            new ClipboardItem({ 'image/png': blob })
          ]);
          showToast('Screenshot Copied! Press Ctrl+V in Chat');
        } catch {
          showToast('Clipboard blocked. Use Save PNG button.');
        }
      }, 'image/png');
    } catch {
      showToast('Failed to copy screenshot.');
    }
  };

  // Save Screenshot to Local Disk
  const handleDownloadScreenshot = () => {
    const canvas = canvasRef.current;
    if (!canvas) return;
    const a = document.createElement('a');
    a.href = canvas.toDataURL('image/png');
    a.download = `android_${new Date().toISOString().replace(/[:.]/g, '-')}.png`;
    a.click();
    showToast('Screenshot saved to Downloads');
  };

  // Send Text into focused Android input
  const handleSendText = async (e?: React.FormEvent) => {
    if (e) e.preventDefault();
    if (!textInput.trim()) return;
    await sendAction({ type: 'text', text: textInput });
    showToast(`Sent: "${textInput}"`);
    setTextInput('');
  };

  // Toggle Wireless ADB (TCP/IP 5555)
  const handleEnableWireless = async () => {
    try {
      const res = await fetch('/api/wireless', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ action: 'enable-tcpip', device: selectedDevice }),
      });
      const data = await res.json();
      if (res.ok) {
        showToast(data.message || 'Wireless ADB Mode Enabled!');
      } else {
        showToast(`Failed: ${data.error}`);
      }
    } catch {
      showToast('Failed to enable wireless mode');
    }
  };

  // Connect via Wi-Fi IP
  const handleConnectWifi = async () => {
    if (!deviceInfo?.wifiIp) {
      showToast('No Wi-Fi IP detected on device');
      return;
    }
    try {
      const res = await fetch('/api/wireless', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ action: 'connect', ip: deviceInfo.wifiIp }),
      });
      const data = await res.json();
      if (res.ok) {
        showToast(`Connected to ${deviceInfo.wifiIp}:5555!`);
        fetchDevices();
      } else {
        showToast(`Connection failed: ${data.error}`);
      }
    } catch {
      showToast('Wireless connection request failed');
    }
  };

  // Document Picture-in-Picture / Floating Mode
  const handleTogglePiP = async () => {
    if (pipWindow) {
      pipWindow.close();
      setPipWindow(null);
      return;
    }

    if ('documentPictureInPicture' in window) {
      try {
        const pip = await (window as any).documentPictureInPicture.requestWindow({
          width: 410,
          height: 860,
        });

        // Copy styles to the PiP window
        [...document.styleSheets].forEach((styleSheet) => {
          try {
            const cssRules = [...styleSheet.cssRules].map((rule) => rule.cssText).join('');
            const style = document.createElement('style');
            style.textContent = cssRules;
            pip.document.head.appendChild(style);
          } catch {
            const link = document.createElement('link');
            if (styleSheet.href) {
              link.rel = 'stylesheet';
              link.type = styleSheet.type;
              link.media = styleSheet.media.mediaText;
              link.href = styleSheet.href;
              pip.document.head.appendChild(link);
            }
          }
        });

        pip.document.body.style.backgroundColor = '#030712';
        pip.document.body.style.margin = '0';
        pip.document.body.style.display = 'flex';
        pip.document.body.style.justifyContent = 'center';
        pip.document.body.style.alignItems = 'center';
        pip.document.body.style.height = '100vh';

        pip.addEventListener('pagehide', () => {
          setPipWindow(null);
        });

        setPipWindow(pip);
        showToast('Screen is now floating on desktop!');
      } catch (err) {
        console.error('PiP error', err);
        // Fallback popup
        window.open(window.location.origin + '?float=true', 'AndroidStudioFloat', 'width=420,height=880,menubar=no,toolbar=no');
      }
    } else {
      // Fallback: Open popup window
      window.open(window.location.origin + '?float=true', 'AndroidStudioFloat', 'width=420,height=880,menubar=no,toolbar=no');
    }
  };

  // Reusable Phone Chassis Component
  const phoneChassisElement = (
    <div className="phone-chassis">
      <div className="phone-notch" />
      <div className={`phone-screen-container ${isInteracting ? 'interacting' : ''}`}>
        <canvas
          ref={canvasRef}
          className="phone-canvas"
          onPointerDown={handlePointerDown}
          onPointerUp={handlePointerUp}
          onWheel={handleWheel}
        />
        {touchRipple && (
          <div
            className="touch-ripple"
            style={{ left: touchRipple.x, top: touchRipple.y }}
          />
        )}
      </div>

      {/* Integrated Android Navigation Bar */}
      <div className="phone-nav-bar">
        <button
          className="nav-key"
          onClick={() => sendAction({ type: 'key', code: 4 })}
          title="Back (Android KeyCode 4)"
        >
          <ChevronLeft size={22} />
        </button>
        <button
          className="nav-key"
          onClick={() => sendAction({ type: 'key', code: 3 })}
          title="Home (Android KeyCode 3)"
        >
          <Circle size={15} />
        </button>
        <button
          className="nav-key"
          onClick={() => sendAction({ type: 'key', code: 187 })}
          title="Recents (Android KeyCode 187)"
        >
          <Square size={15} />
        </button>
      </div>
    </div>
  );

  // If in minimal float/popup mode, render only phone chassis and mini actions
  if (isFloatOnly) {
    return (
      <div className="float-root">
        <div className="float-header">
          <button className="btn-mini" onClick={handleCopyScreenshot} title="Copy Screenshot">
            <Camera size={14} /> Copy
          </button>
          <button className="btn-mini" onClick={() => sendAction({ type: 'key', code: 26 })} title="Power">
            <Power size={14} color="#ef4444" />
          </button>
          <button className="btn-mini" onClick={() => sendAction({ type: 'key', code: 130 })} title="Rotate">
            <RotateCw size={14} />
          </button>
        </div>
        {phoneChassisElement}
        {toastMessage && (
          <div className="toast">
            <CheckCircle2 size={16} color="#22c55e" />
            <span>{toastMessage}</span>
          </div>
        )}
      </div>
    );
  }

  return (
    <div className="studio-app">
      {/* Top Header */}
      <header className="studio-header">
        <div className="brand-section">
          <div className="brand-icon">
            <Flame size={20} />
          </div>
          <div className="brand-title">
            FluxMirror
            <span className="brand-badge">Live</span>
          </div>
        </div>

        {/* Device Selector / Status Pill */}
        <div className="device-pill">
          <div className="pulse-dot" style={{ background: isConnected ? '#22c55e' : '#ef4444' }} />
          {devices.length > 1 ? (
            <select
              value={selectedDevice}
              onChange={(e) => setSelectedDevice(e.target.value)}
              className="device-select"
            >
              {devices.map((d) => (
                <option key={d.id} value={d.id}>
                  {d.model || d.id} ({d.id.includes('.') ? 'Wi-Fi' : 'USB'})
                </option>
              ))}
            </select>
          ) : (
            <span>
              {isConnected
                ? deviceInfo
                  ? `${deviceInfo.manufacturer} ${deviceInfo.model}`
                  : 'Connected'
                : 'Searching for device...'}
            </span>
          )}

          {deviceInfo && (
            <span className="device-os-tag">
              Android {deviceInfo.osVersion}
            </span>
          )}
        </div>

        {/* Header Actions */}
        <div className="header-actions">
          <button
            className="btn-primary"
            onClick={handleCopyScreenshot}
            title="Copy screenshot directly to system clipboard (Ctrl+V)"
          >
            <Camera size={16} />
            Copy Screenshot
          </button>

          <button
            className="btn-secondary"
            onClick={handleDownloadScreenshot}
            title="Download PNG to disk"
          >
            <Download size={15} />
            Save PNG
          </button>

          <button
            className="btn-floating"
            onClick={handleTogglePiP}
            title="Float Always-On-Top Window like scrcpy"
          >
            <ExternalLink size={15} />
            {pipWindow ? 'Dock Screen' : 'Float Screen'}
          </button>

          <button
            className={`btn-secondary ${turboMode ? 'active-turbo' : ''}`}
            onClick={() => setTurboMode(!turboMode)}
            title="Toggle Turbo Mode (Higher FPS)"
          >
            <Zap size={15} color={turboMode ? '#eab308' : '#94a3b8'} />
            {turboMode ? 'Turbo (60 FPS)' : 'Eco (12 FPS)'}
          </button>

          <button
            className="btn-secondary"
            onClick={() => setIsPaused(!isPaused)}
            title="Pause / Resume Live Stream"
          >
            <RefreshCw size={15} className={isPaused ? '' : 'spin-slow'} />
            {isPaused ? 'Paused' : 'Live'}
          </button>
        </div>
      </header>

      {/* Main Studio Area */}
      <main className="studio-workspace">
        {/* Phone Chassis or PiP Placeholder */}
        {pipWindow ? (
          <>
            {createPortal(phoneChassisElement, pipWindow.document.body)}
            <div className="pip-placeholder">
              <Laptop size={48} color="#ff6b00" />
              <h3>Mobile Screen is Floating!</h3>
              <p>The screen is running in an Always-On-Top desktop window above all your applications.</p>
              <button className="btn-primary" onClick={handleTogglePiP}>
                Return to Tab
              </button>
            </div>
          </>
        ) : (
          phoneChassisElement
        )}

        {/* Side Panel: Device Controls, Input & Metrics */}
        <div className="side-panel">
          {/* Quick Hardware Controls */}
          <div className="panel-card">
            <div className="card-header">
              <Sliders size={18} color="#ff6b00" />
              <h3>Hardware Controls</h3>
            </div>
            <div className="control-grid">
              <button className="control-btn" onClick={() => sendAction({ type: 'key', code: 26 })}>
                <Power size={16} color="#ef4444" />
                Power / Lock
              </button>
              <button className="control-btn" onClick={() => sendAction({ type: 'key', code: 224 })}>
                <Smartphone size={16} color="#22c55e" />
                Wake Screen
              </button>
              <button className="control-btn" onClick={() => sendAction({ type: 'key', code: 24 })}>
                <Volume2 size={16} color="#38bdf8" />
                Volume Up
              </button>
              <button className="control-btn" onClick={() => sendAction({ type: 'key', code: 25 })}>
                <Volume1 size={16} color="#38bdf8" />
                Volume Down
              </button>
              <button className="control-btn" onClick={() => sendAction({ type: 'key', code: 130 })}>
                <RotateCw size={16} color="#ff6b00" />
                Rotate View
              </button>
              <button className="control-btn" onClick={fetchDeviceInfo}>
                <RefreshCw size={16} color="#a855f7" />
                Sync Telemetry
              </button>
            </div>
          </div>

          {/* Quick Text Typing */}
          <div className="panel-card">
            <div className="card-header">
              <Send size={18} color="#22c55e" />
              <h3>Type Text on Device</h3>
            </div>
            <form onSubmit={handleSendText} className="text-send-form">
              <input
                type="text"
                placeholder="Type text or paste URL..."
                value={textInput}
                onChange={(e) => setTextInput(e.target.value)}
                className="text-input"
              />
              <button type="submit" className="btn-send" title="Send to focused input field">
                <Send size={14} />
              </button>
            </form>
          </div>

          {/* Wireless ADB Card */}
          <div className="panel-card" style={{ borderLeft: '3px solid #38bdf8' }}>
            <div className="card-header">
              <Wifi size={18} color="#38bdf8" />
              <h3>Wireless Debugging</h3>
            </div>
            <div className="stat-row">
              <span className="stat-label">Phone Wi-Fi IP</span>
              <span className="stat-value">{deviceInfo?.wifiIp || '192.168.1.15'}</span>
            </div>
            <div className="wireless-actions">
              <button className="control-btn" onClick={handleEnableWireless} title="Enable port 5555 over USB">
                Enable TCP/IP 5555
              </button>
              <button className="control-btn" onClick={handleConnectWifi} title="Connect to Wi-Fi IP">
                Connect Wi-Fi
              </button>
            </div>
          </div>

          {/* Device & Stream Metrics */}
          <div className="panel-card">
            <div className="card-header">
              <Smartphone size={18} color="#38bdf8" />
              <h3>Stream & Device Telemetry</h3>
            </div>
            <div className="stat-row">
              <span className="stat-label">Model</span>
              <span className="stat-value">{deviceInfo?.model || 'Samsung SM-G990B2'}</span>
            </div>
            <div className="stat-row">
              <span className="stat-label">Resolution</span>
              <span className="stat-value">
                {deviceInfo ? `${deviceInfo.width} × ${deviceInfo.height}` : '1080 × 2400'}
              </span>
            </div>
            <div className="stat-row">
              <span className="stat-label">Stream Latency</span>
              <span className="stat-value" style={{ color: latency < 150 ? '#22c55e' : '#f59e0b' }}>
                {latency} ms
              </span>
            </div>
            <div className="stat-row">
              <span className="stat-label">Frame Rate</span>
              <span className="stat-value" style={{ color: '#38bdf8' }}>{fps} FPS</span>
            </div>
            <div className="stat-row">
              <span className="stat-label">Battery</span>
              <span className="stat-value">
                {deviceInfo?.isCharging ? (
                  <BatteryCharging size={16} color="#22c55e" />
                ) : (
                  <Battery size={16} color="#94a3b8" />
                )}
                {deviceInfo?.batteryLevel ?? 75}%
              </span>
            </div>
          </div>
        </div>
      </main>

      {/* Toast Notification */}
      {toastMessage && (
        <div className="toast">
          <CheckCircle2 size={18} color="#22c55e" />
          <span>{toastMessage}</span>
        </div>
      )}
    </div>
  );
}

export default App;
