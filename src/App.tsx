import React, { useEffect, useRef, useState, useCallback } from 'react';
import { createPortal } from 'react-dom';
import {
  Camera,
  ChevronLeft,
  Circle,
  Square,
  CheckCircle2,
  RotateCw,
  Pin,
  PinOff,
} from 'lucide-react';

export function App() {
  const [deviceModel, setDeviceModel] = useState<string>('Android Device');
  const [deviceSerial, setDeviceSerial] = useState<string>('');
  const [isConnected, setIsConnected] = useState<boolean>(false);
  const [isCopied, setIsCopied] = useState<boolean>(false);
  const [toastMessage, setToastMessage] = useState<string | null>(null);
  const [isLandscape, setIsLandscape] = useState<boolean>(false);
  const [isPinned, setIsPinned] = useState<boolean>(false);
  const [pipWindow, setPipWindow] = useState<Window | null>(null);
  const [aspectRatio, setAspectRatio] = useState<number>(1080 / 2340);
  const [hasFirstFrame, setHasFirstFrame] = useState<boolean>(false);

  const canvasRef = useRef<HTMLCanvasElement | null>(null);
  const pointerStartRef = useRef<{ x: number; y: number; time: number } | null>(null);
  const isFetchingRef = useRef<boolean>(false);

  const showToast = (msg: string) => {
    setToastMessage(msg);
    setTimeout(() => setToastMessage(null), 2000);
  };

  // 1. Fetch connected device metadata
  const checkDevice = useCallback(async () => {
    try {
      const res = await fetch('/api/device-info');
      if (res.ok) {
        const data = await res.json();
        setDeviceSerial(data.device || '');
        setDeviceModel(data.model || 'SM-G990B2');
        if (data.width && data.height) {
          setAspectRatio(data.width / data.height);
        }
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

  // 2. High-performance, zero-flicker frame puller to HTML5 Canvas
  const pullFrame = useCallback(() => {
    if (isFetchingRef.current) return;
    isFetchingRef.current = true;

    const img = new Image();
    img.crossOrigin = 'anonymous';

    img.onload = () => {
      isFetchingRef.current = false;
      const canvas = canvasRef.current;
      if (canvas) {
        const naturalW = img.naturalWidth || 1080;
        const naturalH = img.naturalHeight || 2340;

        if (canvas.width !== naturalW || canvas.height !== naturalH) {
          canvas.width = naturalW;
          canvas.height = naturalH;
          setAspectRatio(naturalW / naturalH);
        }

        const ctx = canvas.getContext('2d');
        if (ctx) {
          ctx.drawImage(img, 0, 0);
        }
      }
      setIsConnected(true);
      setHasFirstFrame(true);
    };

    img.onerror = () => {
      isFetchingRef.current = false;
      // Fail silently and keep the current display active
    };

    img.src = `/api/frame?t=${Date.now()}`;
  }, []);

  // 3. Continuous frame loop
  useEffect(() => {
    let isCancelled = false;

    const loop = () => {
      if (isCancelled) return;
      pullFrame();
      setTimeout(loop, 220);
    };

    loop();

    return () => {
      isCancelled = true;
    };
  }, [pullFrame]);

  // 4. Send action to phone & trigger immediate visual refresh
  const sendAction = async (payload: any) => {
    try {
      await fetch('/api/action', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ ...payload, device: deviceSerial }),
      });

      // Rapidly pull updated frames to reflect gesture change immediately
      setTimeout(pullFrame, 120);
      setTimeout(pullFrame, 350);
      setTimeout(pullFrame, 700);
    } catch (err) {
      console.error('Action failed', err);
    }
  };

  // 5. Convert canvas pointer coordinates to phone native coordinates
  const getDeviceCoords = (e: React.PointerEvent<HTMLCanvasElement>) => {
    const canvas = canvasRef.current;
    if (!canvas) return { x: 0, y: 0 };
    const rect = canvas.getBoundingClientRect();
    const naturalWidth = canvas.width || 1080;
    const naturalHeight = canvas.height || 2340;

    const scaleX = naturalWidth / rect.width;
    const scaleY = naturalHeight / rect.height;

    const x = Math.max(0, Math.min(naturalWidth, (e.clientX - rect.left) * scaleX));
    const y = Math.max(0, Math.min(naturalHeight, (e.clientY - rect.top) * scaleY));
    return { x, y };
  };

  const handlePointerDown = (e: React.PointerEvent<HTMLCanvasElement>) => {
    e.preventDefault();
    const coords = getDeviceCoords(e);
    pointerStartRef.current = { x: coords.x, y: coords.y, time: performance.now() };
  };

  const handlePointerUp = (e: React.PointerEvent<HTMLCanvasElement>) => {
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

  // 6. Instant in-memory screenshot copy to clipboard
  const handleCopyScreenshot = () => {
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
          showToast('Clipboard permission blocked');
        }
      }, 'image/png');
    } catch {
      showToast('Capture failed');
    }
  };

  // 7. Toggle Screen Rotation
  const handleToggleRotate = () => {
    const next = !isLandscape;
    setIsLandscape(next);
    sendAction({ type: 'rotate' });
    showToast(next ? 'Landscape Mode' : 'Portrait Mode');
  };

  // 8. Always-On-Top Pin Toggle (via Document Picture-in-Picture)
  const handleTogglePin = async () => {
    if (pipWindow) {
      pipWindow.close();
      setPipWindow(null);
      setIsPinned(false);
      return;
    }

    if ('documentPictureInPicture' in window) {
      try {
        const pip = await (window as any).documentPictureInPicture.requestWindow({
          width: 380,
          height: 820,
        });

        // Copy styles into PiP window
        [...document.styleSheets].forEach((sheet) => {
          try {
            const rules = [...sheet.cssRules].map((r) => r.cssText).join('');
            const style = document.createElement('style');
            style.textContent = rules;
            pip.document.head.appendChild(style);
          } catch {
            if (sheet.href) {
              const link = document.createElement('link');
              link.rel = 'stylesheet';
              link.href = sheet.href;
              pip.document.head.appendChild(link);
            }
          }
        });

        pip.document.body.style.margin = '0';
        pip.document.body.style.padding = '0';
        pip.document.body.style.background = '#000';
        pip.document.body.style.overflow = 'hidden';

        pip.addEventListener('pagehide', () => {
          setPipWindow(null);
          setIsPinned(false);
        });

        setPipWindow(pip);
        setIsPinned(true);
        showToast('Pinned on top of Antigravity!');
      } catch (err) {
        console.error('PiP error', err);
        showToast('PiP not available');
      }
    } else {
      showToast('Always-on-top requires Chrome or Edge');
    }
  };

  // 9. Pure Phone Screen Content (Strictly Locked Aspect Ratio, Proportional Responsive)
  const phoneContent = (
    <div className="mirror-root">
      <div
        className="phone-container"
        style={{
          width: `min(100vw, calc((100vh - 78px) * ${aspectRatio}))`,
          height: `min(100vh, calc(min(100vw, calc((100vh - 78px) * ${aspectRatio})) / ${aspectRatio} + 78px))`,
        }}
      >
        {/* Top Control Bar */}
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
              <Camera size={13} />
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
              className={`btn-icon ${isPinned ? 'pinned' : ''}`}
              onClick={handleTogglePin}
              title={isPinned ? 'Unpin Screen' : 'Pin Always-On-Top over Antigravity'}
            >
              {isPinned ? <PinOff size={13} color="#ff6b00" /> : <Pin size={13} />}
            </button>
          </div>
        </div>

        {/* Screen Canvas Viewport (100% Exact Phone Display, Never Stretches) */}
        <div className="mirror-viewport">
          <canvas
            ref={canvasRef}
            className="mirror-canvas"
            onPointerDown={handlePointerDown}
            onPointerUp={handlePointerUp}
            onWheel={handleWheel}
          />

          {!hasFirstFrame && (
            <div className="screen-loading">
              <div className="loading-spinner" />
              <span>Connecting live display...</span>
            </div>
          )}
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
            <CheckCircle2 size={15} color="#10b981" />
            <span>{toastMessage}</span>
          </div>
        )}
      </div>
    </div>
  );

  if (pipWindow) {
    return (
      <div className="mirror-pinned-msg">
        {createPortal(phoneContent, pipWindow.document.body)}
        <div className="pinned-placeholder">
          <Pin size={32} color="#ff6b00" />
          <span>Screen Pinned Always-On-Top!</span>
          <button className="btn-unpin" onClick={handleTogglePin}>
            Unpin Back to Window
          </button>
        </div>
      </div>
    );
  }

  return phoneContent;
}

export default App;
