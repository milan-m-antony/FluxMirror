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
  const [device, setDevice] = useState<string>('');
  const [deviceModel, setDeviceModel] = useState<string>('Connecting...');
  const [isConnected, setIsConnected] = useState<boolean>(false);
  const [toastMessage, setToastMessage] = useState<string | null>(null);
  const [isCopied, setIsCopied] = useState<boolean>(false);

  const canvasRef = useRef<HTMLCanvasElement | null>(null);
  const pointerStartRef = useRef<{ x: number; y: number; time: number } | null>(null);
  const isFetchingRef = useRef<boolean>(false);

  const showToast = (msg: string) => {
    setToastMessage(msg);
    setTimeout(() => setToastMessage(null), 2000);
  };

  // Check device status
  const checkDevice = useCallback(async () => {
    try {
      const res = await fetch('/api/device-info');
      if (res.ok) {
        const data = await res.json();
        setDevice(data.device);
        setDeviceModel(data.model || 'Android Device');
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

  // Frame streaming loop
  useEffect(() => {
    let isCancelled = false;

    const pullNextFrame = async () => {
      if (isCancelled || isFetchingRef.current) return;
      isFetchingRef.current = true;

      try {
        const res = await fetch(`/api/frame?t=${Date.now()}`);
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
          setIsConnected(true);
        }
      } catch {
        // retry
      } finally {
        isFetchingRef.current = false;
        if (!isCancelled) {
          setTimeout(pullNextFrame, 75);
        }
      }
    };

    pullNextFrame();

    return () => {
      isCancelled = true;
    };
  }, []);

  // Send action to phone
  const sendAction = async (payload: any) => {
    try {
      await fetch('/api/action', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ ...payload, device }),
      });
    } catch (err) {
      console.error('Action failed', err);
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
  };

  const handlePointerUp = (e: React.PointerEvent<HTMLCanvasElement>) => {
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

  // Instant in-memory screenshot copy to clipboard
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
          setIsCopied(true);
          showToast('Copied to Clipboard!');
          setTimeout(() => setIsCopied(false), 1800);
        } catch {
          showToast('Clipboard blocked');
        }
      }, 'image/png');
    } catch {
      showToast('Capture failed');
    }
  };

  return (
    <div className="mirror-window">
      {/* Top Floating Control Bar */}
      <div className="mirror-topbar">
        <div className="device-status">
          <span className="status-dot" style={{ background: isConnected ? '#10b981' : '#ef4444' }} />
          <span className="device-name">{deviceModel}</span>
        </div>

        <div className="topbar-actions">
          {/* Quick Screenshot Button */}
          <button
            className={`btn-screenshot ${isCopied ? 'copied' : ''}`}
            onClick={handleCopyScreenshot}
            title="Copy screenshot directly to clipboard (Ctrl + V)"
          >
            <Camera size={14} />
            <span>{isCopied ? 'Copied!' : 'Copy Screenshot'}</span>
          </button>

          {/* Quick Rotate Key */}
          <button
            className="btn-icon"
            onClick={() => sendAction({ type: 'key', code: 130 })}
            title="Rotate Screen"
          >
            <RotateCw size={13} />
          </button>

          {/* Quick Power Key */}
          <button
            className="btn-icon"
            onClick={() => sendAction({ type: 'key', code: 26 })}
            title="Power / Lock"
          >
            <Power size={13} color="#ef4444" />
          </button>
        </div>
      </div>

      {/* Screen Mirror Viewport */}
      <div className="mirror-screen">
        <canvas
          ref={canvasRef}
          className="mirror-canvas"
          onPointerDown={handlePointerDown}
          onPointerUp={handlePointerUp}
          onWheel={handleWheel}
        />
      </div>

      {/* Android Bottom Navigation Bar */}
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

      {/* Toast Notification */}
      {toastMessage && (
        <div className="mirror-toast">
          <CheckCircle2 size={16} color="#10b981" />
          <span>{toastMessage}</span>
        </div>
      )}
    </div>
  );
}

export default App;
