import { execSync, execFileSync } from 'node:child_process';
import fs from 'node:fs';
import path from 'node:path';
import os from 'node:os';
import { findAdb, getConnectedDevices } from './deviceManager.ts';

export interface ScreenshotOptions {
  serial?: string;
  savePath?: string;
  copyToClipboard?: boolean;
}

export function captureDeviceScreenshot(options: ScreenshotOptions = {}): { success: boolean; buffer?: Buffer; savedFile?: string; error?: string } {
  const adb = findAdb();
  const devices = getConnectedDevices();
  const target = options.serial || devices[0];

  if (!target) {
    return { success: false, error: 'No Android device connected via ADB' };
  }

  try {
    const pngBuffer = execFileSync(adb, ['-s', target, 'exec-out', 'screencap', '-p'], {
      maxBuffer: 30 * 1024 * 1024,
    });

    if (!pngBuffer || pngBuffer.length < 1000) {
      return { success: false, error: 'Captured frame is invalid or empty' };
    }

    // 1. Save to file if requested
    let savedFile: string | undefined;
    if (options.savePath) {
      const resolved = path.resolve(options.savePath);
      fs.mkdirSync(path.dirname(resolved), { recursive: true });
      fs.writeFileSync(resolved, pngBuffer);
      savedFile = resolved;
    }

    // 2. Copy to system clipboard if requested (default: true)
    if (options.copyToClipboard !== false) {
      copyBufferToSystemClipboard(pngBuffer);
    }

    return {
      success: true,
      buffer: pngBuffer,
      savedFile,
    };
  } catch (err: any) {
    return { success: false, error: err.message || 'Screenshot failed' };
  }
}

export function copyBufferToSystemClipboard(pngBuffer: Buffer): boolean {
  const platform = os.platform();
  const tempFile = path.join(os.tmpdir(), `flux_screen_${Date.now()}.png`);
  fs.writeFileSync(tempFile, pngBuffer);

  try {
    if (platform === 'win32') {
      const psCommand = `Add-Type -AssemblyName System.Windows.Forms; [Windows.Forms.Clipboard]::SetImage([System.Drawing.Image]::FromFile('${tempFile.replace(/\\/g, '\\\\')}'))`;
      execSync(`powershell.exe -NoProfile -Command "${psCommand}"`, { stdio: 'ignore' });
    } else if (platform === 'darwin') {
      execSync(`osascript -e 'set the clipboard to (read (POSIX file "${tempFile}") as «class PNGf»)'`, { stdio: 'ignore' });
    } else {
      // Linux (X11 / Wayland)
      try {
        execSync(`xclip -selection clipboard -t image/png -i "${tempFile}"`, { stdio: 'ignore' });
      } catch {
        execSync(`wl-copy -t image/png < "${tempFile}"`, { stdio: 'ignore' });
      }
    }
    return true;
  } catch (err) {
    console.error('Failed to copy screenshot to system clipboard', err);
    return false;
  } finally {
    // Keep temp file slightly or delete after delay
    setTimeout(() => {
      try { fs.unlinkSync(tempFile); } catch {}
    }, 2000);
  }
}
