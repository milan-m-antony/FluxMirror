import React, { useEffect, useRef, useState, useCallback } from 'react';
import {
  Camera,
  ChevronLeft,
  Circle,
  Square,
  CheckCircle2,
  RotateCw,
  Power
} from 'lucide-react';

export function App() {
  const [deviceModel, setDeviceModel] = useState<string>('Android Device');
  const [deviceSerial, setDeviceSerial] = useState<string>('');
  const [isConnected, setIsConnected] = useState<boolean>(false);
  const [frameUrl, setFrameUrl] = useState<string>('/api/frame');
  const [isCopied, setIsCopied] = useState<boolean>(false);
  const [toastMessage, setToastMessage] = useState<string | null>(null);
  const [isLandscape, setIsLandscape] = useState<boolean>(false);

  const imgRef = useRef<HTMLImageElement | null>(null);
  const pointerStartRef = useRef<{ x: number; y: number; time: number } | null>(null);
  const isFetchingRef = useRef<boolean>(false);
  const currentBlobUrlRef = useRef<string | null>(null);

  const showToast = (msg: string) => {
    setToastMessage(msg);
    setTimeout(() => setToastMessage(null), 2000);
  };

  // Fetch device information
  const checkDevice = useCallback(async () => {
    try {
      const res = await fetch('/api/device-info');
      if (res.ok) {
        const data = await res.json();
        setDeviceSerial(data.device || '');
        setDeviceModel(data.model || 'SM-G990B2');
        setIsConnected(true);
      } else {
        setIsConnected(false);
      }
    } catch {
      setIsConnected(false);
    }
  }, []);

  useEffect(() => {
    checkDevice();
    const interval = setInterval(checkDevice, 8000);
    return () => clearInterval(interval);
  }, [checkDevice]);

  // Frame streaming loop with object URL recycling
  useEffect(() => {
    let isCancelled = false;

    const pullNextFrame = async () => {
      if (isCancelled || isFetchingRef.current) return;
      isFetchingRef.current = true;

      try {
        const res = await fetch(`/api/frame?t=${Date.now()}`);
        if (res.ok && !isCancelled) {
          const blob = await res.blob();
          if (blob.size > 1000) {
            const newUrl = URL.createObjectURL(blob);
            setFrameUrl(newUrl);

            // Clean up previous blob URL to prevent memory accumulation
            if (currentBlobUrlRef.current && currentBlobUrlRef.current !== '/api/frame') {
              URL.revokeObjectURL(currentBlobUrlRef.current);
            }
            currentBlobUrlRef.current = newUrl;
            setIsConnected(true);
          }
        }
      } catch {
        // Stream retry
      } finally {
        isFetchingRef.current = false;
        if (!isCancelled) {
          setTimeout(pullNextFrame, 120);
        }
      }
    };

    pullNextFrame();

    return () => {
      isCancelled = true;
      if (currentBlobUrlRef.current && currentBlobUrlRef.current !== '/api/frame') {
        URL.revokeObjectURL(currentBlobUrlRef.current);
      }
    };
  }, []);

  // Send action to phone
  const sendAction = async (payload: any) => {
    try {
      await fetch('/api/action', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ ...payload, device: deviceSerial }),
      });
    } catch (err) {
      console.error('Action failed', err);
    }
  };

  // Convert image pointer coordinates to phone native coordinates
  const getDeviceCoords = (e: React.PointerEvent<HTMLImageElement>) => {
    const img = imgRef.current;
    if (!img) return { x: 0, y: 0 };
    const rect = img.getBoundingClientRect();
    const naturalWidth = img.naturalWidth || 1080;
    const naturalHeight = img.naturalHeight || 2340;

    const scaleX = naturalWidth / rect.width;
    const scaleY = naturalHeight / rect.height;

    const x = (e.clientX - rect.left) * scaleX;
    const y = (e.clientY - rect.top) * scaleY;
    return { x, y };
  };

  const handlePointerDown = (e: React.PointerEvent<HTMLImageElement>) => {
    e.preventDefault();
    const coords = getDeviceCoords(e);
    pointerStartRef.current = { x: coords.x, y: coords.y, time: performance.now() };
  };

  const handlePointerUp = (e: React.PointerEvent<HTMLImageElement>) => {
    e.preventDefault();
    if (!pointerStartRef.current) return;
    const endCoords = getDeviceCoords(e);
    const startCoords = pointerStartRef.current;
    const duration = Math.min(600, Math.max(100, Math.round(performance.now() - startCoords.time)));

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

  const handleWheel = (e: React.WheelEvent<HTMLImageElement>) => {
    e.preventDefault();
    const img = imgRef.current;
    if (!img) return;
    const midX = (img.naturalWidth || 1080) / 2;
    const midY = (img.naturalHeight || 2340) / 2;
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

  // Instant in-memory screenshot copy to clipboard
  const handleCopyScreenshot = async () => {
    const img = imgRef.current;
    if (!img) return;

    try {
      const offscreen = document.createElement('canvas');
      offscreen.width = img.naturalWidth || 1080;
      offscreen.height = img.naturalHeight || 2340;
      const ctx = offscreen.getContext('2d');
      if (!ctx) return;
      ctx.drawImage(img, 0, 0);

      offscreen.toBlob(async (blob) => {
        if (!blob) return;
        try {
          await navigator.clipboard.write([
            new ClipboardItem({ 'image/png': blob })
          ]);
          setIsCopied(true);
          showToast('Copied to Clipboard!');
          setTimeout(() => setIsCopied(false), 1800);
        } catch {
          showToast('Clipboard permission blocked');
        }
      }, 'image/png');
    } catch {
      showToast('Capture failed');
    }
  };

  // Toggle Screen Rotation
  const handleToggleRotate = () => {
    const next = !isLandscape;
    setIsLandscape(next);
    sendAction({ type: 'rotate' });
    showToast(next ? 'Landscape Mode' : 'Portrait Mode');
  };

  // Power / Lock Screen
  const handlePowerLock = () => {
    sendAction({ type: 'key', code: 26 });
    showToast('Power / Lock Screen');
  };

  const activeAspectRatio = isLandscape ? 19.5 / 9 : 9 / 19.5;

  return (
    <div className="mirror-app">
      {/* Centered Phone Chassis with Responsive Fixed Aspect Ratio */}
      <div className="phone-container" style={{ aspectRatio: activeAspectRatio }}>
        {/* Top Floating Control Bar */}
        <div className="mirror-topbar">
          <div className="device-status">
            <span
              className="status-dot"
              style={{ background: isConnected ? '#10b981' : '#ef4444' }}
            />
            <span className="device-name">{deviceModel}</span>
          </div>

          <div className="topbar-actions">
            <button
              className={`btn-screenshot ${isCopied ? 'copied' : ''}`}
              onClick={handleCopyScreenshot}
              title="Copy screenshot directly to clipboard (Ctrl + V)"
            >
              <Camera size={14} />
              <span>{isCopied ? 'Copied!' : 'Copy Screenshot'}</span>
            </button>

            <button
              className="btn-icon"
              onClick={handleToggleRotate}
              title="Rotate Screen"
            >
              <RotateCw size={13} />
            </button>

            <button
              className="btn-icon"
              onClick={handlePowerLock}
              title="Power / Lock"
            >
              <Power size={13} color="#ef4444" />
            </button>
          </div>
        </div>

        {/* Live Phone Display Viewport */}
        <div className="phone-viewport">
          <img
            ref={imgRef}
            src={frameUrl}
            alt="Android Device Screen"
            className="phone-display-img"
            onPointerDown={handlePointerDown}
            onPointerUp={handlePointerUp}
            onWheel={handleWheel}
            draggable={false}
          />
        </div>

        {/* Android Bottom Navigation Dock */}
        <div className="mirror-nav">
          <button
            className="nav-btn"
            onClick={() => sendAction({ type: 'key', code: 4 })}
            title="Back"
          >
            <ChevronLeft size={22} />
          </button>
          <button
            className="nav-btn"
            onClick={() => sendAction({ type: 'key', code: 3 })}
            title="Home"
          >
            <Circle size={15} />
          </button>
          <button
            className="nav-btn"
            onClick={() => sendAction({ type: 'key', code: 187 })}
            title="Recents"
          >
            <Square size={14} />
          </button>
        </div>

        {/* Toast */}
        {toastMessage && (
          <div className="mirror-toast">
            <CheckCircle2 size={16} color="#10b981" />
            <span>{toastMessage}</span>
          </div>
        )}
      </div>
    </div>
  );
}

export default App;
