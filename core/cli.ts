import { startNativeMirror } from './mirrorEngine.ts';
import { inspectDeviceCapabilities } from './deviceManager.ts';
import { captureDeviceScreenshot } from './screenshotEngine.ts';

const command = process.argv[2] || 'mirror';

if (command === 'diag' || command === 'diagnostics') {
  console.log(`\n========================================================`);
  console.log(`              FLUXMIRROR PIPELINE DIAGNOSTICS          `);
  console.log(`========================================================\n`);

  const caps = inspectDeviceCapabilities();
  if (!caps) {
    console.error('[x] No connected Android device detected via ADB.');
    process.exit(1);
  }

  console.log(`[+] Device Model:       ${caps.manufacturer} ${caps.model}`);
  console.log(`[+] Serial Number:      ${caps.serial}`);
  console.log(`[+] Android Version:    ${caps.androidVersion}`);
  console.log(`[+] Native Resolution:  ${caps.width}x${caps.height}`);
  console.log(`[+] Max Display Refresh:${caps.maxFps} Hz`);
  console.log(`[+] Wi-Fi IP Address:   ${caps.ipAddress || 'Not connected to local Wi-Fi'}`);
  console.log(`\n[+] Supported Display Refresh Modes:`);
  caps.supportedModes.forEach((mode, idx) => {
    console.log(`    Mode ${idx + 1}: ${mode.width}x${mode.height} @ ${mode.fps} FPS`);
  });

  console.log(`\n[+] Verification Check:`);
  if (caps.maxFps >= 120) {
    console.log(`    ✨ Device supports High-Refresh 120 FPS pipeline.`);
  } else if (caps.maxFps >= 90) {
    console.log(`    ✨ Device supports High-Refresh 90 FPS pipeline.`);
  } else {
    console.log(`    ✨ Device supports Standard 60 FPS pipeline.`);
  }
  process.exit(0);
}

if (command === 'snap' || command === 'screenshot') {
  const saveArg = process.argv.find(a => a.startsWith('--save='));
  const savePath = saveArg ? saveArg.split('=')[1] : undefined;

  console.log(`[+] Capturing screenshot...`);
  const result = captureDeviceScreenshot({ savePath, copyToClipboard: true });

  if (result.success) {
    console.log(`[+] Screenshot copied to system clipboard! Press Ctrl+V (or Cmd+V) to paste.`);
    if (result.savedFile) {
      console.log(`[+] Saved to: ${result.savedFile}`);
    }
  } else {
    console.error(`[x] Failed: ${result.error}`);
    process.exit(1);
  }
  process.exit(0);
}

// Default: Run Mirror
const fpsArg = process.argv.find(a => a.startsWith('--fps='));
const fpsVal = fpsArg ? fpsArg.split('=')[1] : 'max';
const targetFps = fpsVal === 'max' ? 'max' : parseInt(fpsVal, 10);
const isWifi = process.argv.includes('--wifi') || process.argv.includes('--tcpip');

try {
  startNativeMirror({
    fps: targetFps,
    tcpip: isWifi,
  });
} catch (err: any) {
  console.error(`[x] Error starting native mirror:`, err.message);
  process.exit(1);
}
