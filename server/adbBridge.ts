import { execSync, spawn } from 'node:child_process';
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
          let height = 2400;
          const match = sizeOut.match(/(\d+)x(\d+)/);
          if (match) {
            width = parseInt(match[1], 10);
            height = parseInt(match[2], 10);
          }

          let batteryLevel = 85;
          let isCharging = false;
          try {
            const batteryOut = execSync(`"${adbPath}" -s ${device} shell dumpsys battery`, { encoding: 'utf-8' });
            const levelMatch = batteryOut.match(/level:\s*(\d+)/);
            if (levelMatch) batteryLevel = parseInt(levelMatch[1], 10);
            const statusMatch = batteryOut.match(/status:\s*(\d+)/);
            if (statusMatch && statusMatch[1] === '2') isCharging = true;
          } catch {
            // fallback
          }

          let wifiIp = '';
          try {
            const ipOut = execSync(`"${adbPath}" -s ${device} shell ip -f inet addr show wlan0`, { encoding: 'utf-8' });
            const ipMatch = ipOut.match(/inet\s+(\d+\.\d+\.\d+\.\d+)/);
            if (ipMatch) wifiIp = ipMatch[1];
          } catch {
            // fallback
          }

          res.setHeader('Content-Type', 'application/json');
          res.end(JSON.stringify({
            device,
            model,
            manufacturer,
            osVersion,
            width,
            height,
            batteryLevel,
            isCharging,
            wifiIp,
          }));
        } catch (err: any) {
          res.statusCode = 500;
          res.setHeader('Content-Type', 'application/json');
          res.end(JSON.stringify({ error: err.message }));
        }
      });

      // 3. Screen Frame Capture (Instant Cached Buffer & Continuous Pipeline)
      let cachedFrame: Buffer | null = null;
      let currentPromise: Promise<Buffer | null> | null = null;

      const triggerCapture = (device: string): Promise<Buffer | null> => {
        if (currentPromise) return currentPromise;

        currentPromise = new Promise<Buffer | null>((resolve) => {
          const proc = spawn(adbPath, ['-s', device, 'exec-out', 'screencap', '-p']);
          const chunks: Buffer[] = [];
          proc.stdout.on('data', chunk => chunks.push(chunk));
          proc.stderr.on('data', () => {});
          proc.on('close', (code) => {
            currentPromise = null;
            if (code === 0 && chunks.length > 0) {
              cachedFrame = Buffer.concat(chunks);
              resolve(cachedFrame);
            } else {
              resolve(cachedFrame);
            }
          });
          proc.on('error', () => {
            currentPromise = null;
            resolve(cachedFrame);
          });
        });

        return currentPromise;
      };

      // Pre-warm initial frame on server startup
      const initialDev = resolveTargetDevice();
      if (initialDev) {
        triggerCapture(initialDev);
      }

      server.middlewares.use('/api/frame', async (req, res) => {
        const url = new URL(req.url || '', 'http://localhost');
        const requestedDevice = url.searchParams.get('device');
        const device = resolveTargetDevice(requestedDevice);

        if (!device) {
          res.statusCode = 404;
          return res.end('No device connected');
        }

        // Return cached frame immediately for instant response (< 5ms)
        if (cachedFrame && cachedFrame.length > 0) {
          res.setHeader('Content-Type', 'image/png');
          res.setHeader('Content-Length', cachedFrame.length);
          res.setHeader('Cache-Control', 'no-cache, no-store, must-revalidate');
          res.end(cachedFrame);

          // Trigger background capture to keep frame fresh
          triggerCapture(device);
          return;
        }

        // First frame: await capture
        const frame = await triggerCapture(device);
        if (frame && frame.length > 0) {
          res.setHeader('Content-Type', 'image/png');
          res.setHeader('Content-Length', frame.length);
          res.setHeader('Cache-Control', 'no-cache, no-store, must-revalidate');
          return res.end(frame);
        } else {
          res.statusCode = 500;
          return res.end('Failed to capture frame');
        }
      });

      // 4. Action Dispatcher (Tap, Swipe, Keyevent, Text Typing, Rotate, Power)
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
            } else if (data.type === 'text' && data.text) {
              const escaped = data.text.replace(/([ "$\\])/g, '\\$1');
              cmd = `"${adbPath}" -s ${device} shell input text "${escaped}"`;
            }

            if (cmd) {
              execSync(cmd);
              // Trigger fresh frame capture after action
              setTimeout(() => triggerCapture(device), 150);
            }

            res.setHeader('Content-Type', 'application/json');
            res.end(JSON.stringify({ ok: true }));
          } catch (err: any) {
            res.statusCode = 500;
            res.end(JSON.stringify({ error: err.message }));
          }
        });
      });

      // 5. Wireless ADB Toggle & Connect
      server.middlewares.use('/api/wireless', (req, res) => {
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

            if (data.action === 'enable-tcpip') {
              if (!device) throw new Error('No device connected via USB to enable TCP/IP mode');
              execSync(`"${adbPath}" -s ${device} tcpip 5555`);
              res.setHeader('Content-Type', 'application/json');
              return res.end(JSON.stringify({ ok: true, message: 'TCP/IP 5555 enabled. You can now disconnect USB.' }));
            }

            if (data.action === 'connect' && data.ip) {
              const target = data.ip.includes(':') ? data.ip : `${data.ip}:5555`;
              const out = execSync(`"${adbPath}" connect ${target}`, { encoding: 'utf-8' });
              res.setHeader('Content-Type', 'application/json');
              return res.end(JSON.stringify({ ok: true, output: out.trim() }));
            }

            res.statusCode = 400;
            res.end(JSON.stringify({ error: 'Invalid wireless action' }));
          } catch (err: any) {
            res.statusCode = 500;
            res.end(JSON.stringify({ error: err.message }));
          }
        });
      });
    },
  };
}
