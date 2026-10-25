import { execSync } from 'node:child_process';
import fs from 'node:fs';
import path from 'node:path';
import os from 'node:os';

export interface DeviceCapabilities {
  serial: string;
  model: string;
  manufacturer: string;
  androidVersion: string;
  width: number;
  height: number;
  maxFps: number;
  supportedModes: { fps: number; width: number; height: number }[];
  ipAddress?: string;
}

export function findAdb(): string {
  if (process.env.ADB && fs.existsSync(process.env.ADB)) return process.env.ADB;

  const candidates = os.platform() === 'win32' ? [
    'C:\\Android\\Sdk\\platform-tools\\adb.exe',
    path.join(process.env.LOCALAPPDATA || '', 'Android', 'Sdk', 'platform-tools', 'adb.exe'),
    'adb',
  ] : [
    '/opt/homebrew/bin/adb',
    '/usr/local/bin/adb',
    '/usr/bin/adb',
    path.join(os.homedir(), 'Library/Android/sdk/platform-tools/adb'),
    'adb',
  ];

  for (const cand of candidates) {
    if (cand === 'adb') {
      try {
        execSync(os.platform() === 'win32' ? 'where.exe adb' : 'which adb', { stdio: 'ignore' });
        return 'adb';
      } catch {
        continue;
      }
    }
    if (fs.existsSync(cand)) return cand;
  }
  return 'adb';
}

export function findScrcpy(): string | null {
  if (process.env.SCRCPY_PATH && fs.existsSync(process.env.SCRCPY_PATH)) {
    return process.env.SCRCPY_PATH;
  }

  const candidates = os.platform() === 'win32' ? [
    'C:\\Users\\minua\\AppData\Local\\Microsoft\\WinGet\\Packages\\Genymobile.scrcpy_Microsoft.Winget.Source_8wekyb3d8bbwe\\scrcpy-win64-v5.0.1\\scrcpy.exe',
    path.join(process.env.LOCALAPPDATA || '', 'Microsoft', 'WinGet', 'Packages', 'Genymobile.scrcpy_Microsoft.Winget.Source_8wekyb3d8bbwe', 'scrcpy-win64-v5.0.1', 'scrcpy.exe'),
    'scrcpy',
  ] : [
    '/opt/homebrew/bin/scrcpy',
    '/usr/local/bin/scrcpy',
    '/usr/bin/scrcpy',
    '/snap/bin/scrcpy',
    'scrcpy',
  ];

  for (const cand of candidates) {
    if (cand === 'scrcpy') {
      try {
        execSync(os.platform() === 'win32' ? 'where.exe scrcpy' : 'which scrcpy', { stdio: 'ignore' });
        return 'scrcpy';
      } catch {
        continue;
      }
    }
    if (fs.existsSync(cand)) return cand;
  }
  return null;
}

export function getConnectedDevices(): string[] {
  const adb = findAdb();
  try {
    const out = execSync(`"${adb}" devices`, { encoding: 'utf-8' });
    const devices: string[] = [];
    for (const line of out.split('\n')) {
      const trimmed = line.trim();
      if (trimmed && !trimmed.startsWith('List of')) {
        const parts = trimmed.split(/\s+/);
        if (parts.length >= 2 && parts[1] === 'device') {
          devices.push(parts[0]);
        }
      }
    }
    return devices;
  } catch {
    return [];
  }
}

export function inspectDeviceCapabilities(serial?: string): DeviceCapabilities | null {
  const adb = findAdb();
  const devices = getConnectedDevices();
  const target = serial || devices[0];
  if (!target) return null;

  try {
    const model = execSync(`"${adb}" -s ${target} shell getprop ro.product.model`, { encoding: 'utf-8' }).trim();
    const manufacturer = execSync(`"${adb}" -s ${target} shell getprop ro.product.manufacturer`, { encoding: 'utf-8' }).trim();
    const androidVersion = execSync(`"${adb}" -s ${target} shell getprop ro.build.version.release`, { encoding: 'utf-8' }).trim();

    // Screen dimensions
    const sizeOut = execSync(`"${adb}" -s ${target} shell wm size`, { encoding: 'utf-8' }).trim();
    let width = 1080;
    let height = 2340;
    const sizeMatch = sizeOut.match(/(\d+)x(\d+)/);
    if (sizeMatch) {
      width = parseInt(sizeMatch[1], 10);
      height = parseInt(sizeMatch[2], 10);
    }

    // Refresh rate analysis
    let maxFps = 60;
    const supportedModes: { fps: number; width: number; height: number }[] = [];

    try {
      const displayDump = execSync(`"${adb}" -s ${target} shell dumpsys display`, { encoding: 'utf-8' });
      // Match patterns like fps=120.00001 or refreshRate 120
      const fpsMatches = displayDump.matchAll(/fps=([0-9.]+)/g);
      for (const m of fpsMatches) {
        const fps = Math.round(parseFloat(m[1]));
        if (fps >= 30 && fps <= 240) {
          if (!supportedModes.some(sm => sm.fps === fps)) {
            supportedModes.push({ fps, width, height });
          }
          if (fps > maxFps) maxFps = fps;
        }
      }
    } catch {
      // Fallback
    }

    if (supportedModes.length === 0) {
      supportedModes.push({ fps: 60, width, height });
    }

    // Detect Wi-Fi IP address
    let ipAddress: string | undefined;
    try {
      const ipOut = execSync(`"${adb}" -s ${target} shell ip route`, { encoding: 'utf-8' });
      const ipMatch = ipOut.match(/src\s+([0-9.]+)/);
      if (ipMatch) {
        ipAddress = ipMatch[1];
      }
    } catch {
      // Ignore
    }

    return {
      serial: target,
      model,
      manufacturer,
      androidVersion,
      width,
      height,
      maxFps,
      supportedModes,
      ipAddress,
    };
  } catch (err) {
    console.error('Failed to inspect device', err);
    return null;
  }
}
