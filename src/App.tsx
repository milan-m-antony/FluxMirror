import React, { useEffect, useRef, useState, useCallback } from 'react';
import {
  Camera,
  ChevronLeft,
  Circle,
  Square,
  CheckCircle2,
  RotateCw,
  Power,
  Smartphone
} from 'lucide-react';

export function App() {
  const [deviceModel, setDeviceModel] = useState<string>('Connecting...');
  const [deviceSerial, setDeviceSerial] = useState<string>('');
  const [isConnected, setIsConnected] = useState<boolean>(false);
  const [frameUrl, setFrameUrl] = useState<string | null>(null);
  const [isCopied, setIsCopied] = useState<boolean>(false);
  const [toastMessage, setToastMessage] = useState<string | null>(null);
  const [aspectRatio, setAspectRatio] = useState<number>(9 / 19.5);

  const imgRef = useRef<HTMLImageElement | null>(null);
  const pointerStartRef = useRef<{ x: number; y: number; time: number } | null>(null);
  const isFetchingRef = useRef<boolean>(false);
  const currentBlobUrlRef = useRef<string | null>(null);

  const showToast = (msg: string) => {
    setToastMessage(msg);
    setTimeout(() => setToastMessage(null), 2000);
  };

  // Fetch device information & resolution
  const checkDevice = useCallback(async () => {
    try {
      const res = await fetch('/api/device-info');
      if (res.ok) {
        const data = await res.json();
        setDeviceSerial(data.device || '');
        setDeviceModel(data.model || 'Android Device');
        if (data.width && data.height) {
          setAspectRatio(data.width / data.height);
        }
        setIsConnected(true);
      } else {
        setIsConnected(false);
        setDeviceModel('No Device');
      }
    } catch {
      setIsConnected(false);
      setDeviceModel('Searching...');
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

            // Clean up previous blob URL to avoid memory leak
            if (currentBlobUrlRef.current) {
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
          setTimeout(pullNextFrame, 100);
        }
      }
    };

    pullNextFrame();

    return () => {
      isCancelled = true;
      if (currentBlobUrlRef.current) {
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
      // Tap
      sendAction({ type: 'tap', x: endCoords.x, y: endCoords.y });
    } else {
      // Swipe / Drag
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

  return (
    <div className="mirror-app">
      {/* Centered Phone Chassis with Exact Responsive Aspect Ratio */}
      <div className="phone-container" style={{ aspectRatio }}>
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
              onClick={() => sendAction({ type: 'key', code: 130 })}
              title="Rotate Screen"
            >
              <RotateCw size={13} />
            </button>

            <button
              className="btn-icon"
              onClick={() => sendAction({ type: 'key', code: 26 })}
              title="Power / Lock"
            >
              <Power size={13} color="#ef4444" />
            </button>
          </div>
        </div>

        {/* Phone Viewport */}
        <div className="phone-viewport">
          {frameUrl ? (
            <img
              ref={imgRef}
              src={frameUrl}
              alt="Android Screen"
              className="phone-display-img"
              onPointerDown={handlePointerDown}
              onPointerUp={handlePointerUp}
              onWheel={handleWheel}
              draggable={false}
            />
          ) : (
            <div className="loading-display">
              <Smartphone size={36} color="#ff6b00" className="pulse-icon" />
              <span>Connecting to {deviceModel}...</span>
            </div>
          )}
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
