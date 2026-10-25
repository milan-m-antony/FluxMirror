import { execSync, execFile } from 'node:child_process';
import fs from 'node:fs';
import path from 'node:path';
import type { Plugin, ViteDevServer } from 'vite';

function findAdb(): string {
  const candidates = [
    'C:\\Android\\Sdk\\platform-tools\\adb.exe',
    path.join(process.env.LOCALAPPDATA || '', 'Android', 'Sdk', 'platform-tools', 'adb.exe'),
    'adb',
  ];
  for (const cand of candidates) {
    if (cand === 'adb') return 'adb';
    if (fs.existsSync(cand)) return cand;
  }
  return 'adb';
}

const adbPath = findAdb();

function getDevicesList(): { id: string; status: string; model?: string }[] {
  try {
    const out = execSync(`"${adbPath}" devices -l`, { encoding: 'utf-8' });
    const devices: { id: string; status: string; model?: string }[] = [];
    for (const line of out.split('\n')) {
      const trimmed = line.trim();
      if (trimmed && !trimmed.startsWith('List of')) {
        const parts = trimmed.split(/\s+/);
        const id = parts[0];
        const status = parts[1];
        if (id && status) {
          const modelMatch = trimmed.match(/model:(\S+)/);
          devices.push({
            id,
            status,
            model: modelMatch ? modelMatch[1] : undefined,
          });
        }
      }
    }
    return devices;
  } catch {
    return [];
  }
}

function resolveTargetDevice(target?: string | null): string | null {
  const list = getDevicesList().filter(d => d.status === 'device');
  if (!list.length) return null;
  if (target) {
    const matched = list.find(d => d.id === target);
    if (matched) return matched.id;
  }
  return list[0].id;
}

export function adbBridgePlugin(): Plugin {
  return {
    name: 'android-web-studio-bridge',
    configureServer(server: ViteDevServer) {
      // 1. Devices List
      server.middlewares.use('/api/devices', (_req, res) => {
        const devices = getDevicesList();
        res.setHeader('Content-Type', 'application/json');
        res.end(JSON.stringify({ devices }));
      });

      // 2. Device Information & Telemetry
      server.middlewares.use('/api/device-info', (req, res) => {
        const url = new URL(req.url || '', 'http://localhost');
        const requestedDevice = url.searchParams.get('device');
        const device = resolveTargetDevice(requestedDevice);

        if (!device) {
          res.statusCode = 404;
          res.setHeader('Content-Type', 'application/json');
          return res.end(JSON.stringify({ error: 'No Android device connected' }));
        }

        try {
          const model = execSync(`"${adbPath}" -s ${device} shell getprop ro.product.model`, { encoding: 'utf-8' }).trim();
          const manufacturer = execSync(`"${adbPath}" -s ${device} shell getprop ro.product.manufacturer`, { encoding: 'utf-8' }).trim();
          const osVersion = execSync(`"${adbPath}" -s ${device} shell getprop ro.build.version.release`, { encoding: 'utf-8' }).trim();
          const sizeOut = execSync(`"${adbPath}" -s ${device} shell wm size`, { encoding: 'utf-8' }).trim();

          let width = 1080;
          let height = 2340;
          const match = sizeOut.match(/(\d+)x(\d+)/);
          if (match) {
            width = parseInt(match[1], 10);
            height = parseInt(match[2], 10);
          }

          res.setHeader('Content-Type', 'application/json');
          res.end(JSON.stringify({
            device,
            model,
            manufacturer,
            osVersion,
            width,
            height,
          }));
        } catch (err: any) {
          res.statusCode = 500;
          res.setHeader('Content-Type', 'application/json');
          res.end(JSON.stringify({ error: err.message }));
        }
      });

      // 3. High-Performance Screen Capture Engine
      let lastValidFrame: Buffer | null = null;
      let capturePromise: Promise<Buffer | null> | null = null;

      const captureScreen = (device: string): Promise<Buffer | null> => {
        if (capturePromise) {
          if (lastValidFrame) return Promise.resolve(lastValidFrame);
          return capturePromise;
        }

        capturePromise = new Promise<Buffer | null>((resolve) => {
          execFile(adbPath, ['-s', device, 'exec-out', 'screencap', '-p'], {
            encoding: 'buffer',
            maxBuffer: 25 * 1024 * 1024
          }, (err, stdout) => {
            capturePromise = null;
            // Verify valid PNG header (89 50 4E 47)
            if (!err && stdout && stdout.length > 1000 && stdout[0] === 0x89 && stdout[1] === 0x50) {
              lastValidFrame = stdout;
              resolve(stdout);
            } else {
              resolve(lastValidFrame);
            }
          });
        });

        return capturePromise;
      };

      // Pre-capture frame on startup
      const initialDev = resolveTargetDevice();
      if (initialDev) {
        captureScreen(initialDev);
      }

      server.middlewares.use('/api/frame', async (req, res) => {
        const url = new URL(req.url || '', 'http://localhost');
        const requestedDevice = url.searchParams.get('device');
        const device = resolveTargetDevice(requestedDevice);

        if (!device) {
          res.statusCode = 404;
          return res.end('No device connected');
        }

        // Always return the latest valid frame immediately if available
        if (lastValidFrame && lastValidFrame.length > 1000) {
          res.setHeader('Content-Type', 'image/png');
          res.setHeader('Content-Length', lastValidFrame.length);
          res.setHeader('Cache-Control', 'no-cache, no-store, must-revalidate');
          res.end(lastValidFrame);

          // Asynchronously trigger refresh in background
          captureScreen(device);
          return;
        }

        // If no frame yet, wait for first capture
        const frame = await captureScreen(device);
        if (frame && frame.length > 1000) {
          res.setHeader('Content-Type', 'image/png');
          res.setHeader('Content-Length', frame.length);
          res.setHeader('Cache-Control', 'no-cache, no-store, must-revalidate');
          res.end(frame);
        } else {
          res.statusCode = 503;
          res.end('Device screen not ready');
        }
      });

      // 4. Action Dispatcher (Tap, Swipe, Keyevent, Rotate)
      server.middlewares.use('/api/action', (req, res) => {
        if (req.method !== 'POST') {
          res.statusCode = 405;
          return res.end('Method Not Allowed');
        }

        let body = '';
        req.on('data', chunk => { body += chunk; });
        req.on('end', () => {
          try {
            const data = JSON.parse(body || '{}');
            const device = resolveTargetDevice(data.device);
            if (!device) {
              res.statusCode = 404;
              return res.end(JSON.stringify({ error: 'No device available' }));
            }

            let cmd = '';
            if (data.type === 'tap' && typeof data.x === 'number' && typeof data.y === 'number') {
              cmd = `"${adbPath}" -s ${device} shell input tap ${Math.round(data.x)} ${Math.round(data.y)}`;
            } else if (data.type === 'swipe') {
              const dur = data.duration || 250;
              cmd = `"${adbPath}" -s ${device} shell input swipe ${Math.round(data.x1)} ${Math.round(data.y1)} ${Math.round(data.x2)} ${Math.round(data.y2)} ${dur}`;
            } else if (data.type === 'key' && data.code) {
              cmd = `"${adbPath}" -s ${device} shell input keyevent ${data.code}`;
            } else if (data.type === 'rotate') {
              cmd = `"${adbPath}" -s ${device} shell "settings put system accelerometer_rotation 1"`;
            }

            if (cmd) {
              execSync(cmd);
              // Trigger fast background capture so the updated screen is captured immediately
              setTimeout(() => {
                captureScreen(device);
              }, 100);
            }

            res.setHeader('Content-Type', 'application/json');
            res.end(JSON.stringify({ ok: true }));
          } catch (err: any) {
            res.statusCode = 500;
            res.end(JSON.stringify({ error: err.message }));
          }
        });
      });
    },
  };
}
