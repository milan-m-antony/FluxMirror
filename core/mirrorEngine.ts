import { spawn, type ChildProcess } from 'node:child_process';
import path from 'node:path';
import { findScrcpy, inspectDeviceCapabilities, type DeviceCapabilities } from './deviceManager.ts';

export interface MirrorConfig {
  serial?: string;
  fps?: number | 'max';
  bitrate?: string;
  alwaysOnTop?: boolean;
  tcpip?: boolean;
  printFps?: boolean;
  stayAwake?: boolean;
  noWindow?: boolean;
  videoCodec?: 'h264' | 'h265';
}

export function buildScrcpyArgs(caps: DeviceCapabilities | null, config: MirrorConfig = {}): string[] {
  const args: string[] = [];

  // Target device
  if (config.serial || caps?.serial) {
    args.push('-s', config.serial || caps!.serial);
  }

  // Window title with live telemetry
  const model = caps?.model || 'Android Device';
  const targetFps = config.fps === 'max' ? (caps?.maxFps || 120) : (config.fps || caps?.maxFps || 60);
  args.push('--window-title', `FluxMirror — ${model} (${targetFps} FPS)`);

  // Frame rate optimization
  args.push(`--max-fps=${targetFps}`);

  // Latency optimizations (zero buffer)
  args.push('--video-buffer=0');

  // Video codec and bitrate
  const codec = config.videoCodec || 'h264';
  args.push(`--video-codec=${codec}`);
  args.push(`--video-bit-rate=${config.bitrate || '16M'}`);

  // Always-On-Top
  if (config.alwaysOnTop !== false) {
    args.push('--always-on-top');
  }

  // Stay awake
  if (config.stayAwake !== false) {
    args.push('--stay-awake');
  }

  // Live FPS telemetry validation
  if (config.printFps !== false) {
    args.push('--print-fps');
  }

  // Wi-Fi TCP/IP
  if (config.tcpip) {
    args.push('--tcpip');
  }

  return args;
}

export function startNativeMirror(config: MirrorConfig = {}): { process: ChildProcess; caps: DeviceCapabilities | null } {
  const scrcpyPath = findScrcpy();
  if (!scrcpyPath) {
    throw new Error('scrcpy binary not found. Please install scrcpy on your system.');
  }

  const caps = inspectDeviceCapabilities(config.serial);
  const args = buildScrcpyArgs(caps, config);

  console.log(`\n========================================================`);
  console.log(`                 FLUXMIRROR NATIVE ENGINE               `);
  console.log(`   60–120 FPS Ultra-Low Latency Direct GPU Pipeline     `);
  console.log(`========================================================\n`);

  if (caps) {
    console.log(`[+] Device:          ${caps.manufacturer} ${caps.model} (Android ${caps.androidVersion})`);
    console.log(`[+] Resolution:      ${caps.width}x${caps.height}`);
    console.log(`[+] Max Display FPS: ${caps.maxFps} Hz`);
    console.log(`[+] Target Stream:   ${config.fps === 'max' ? caps.maxFps : (config.fps || caps.maxFps)} FPS`);
    console.log(`[+] Video Pipeline:  Hardware ${config.videoCodec || 'H.264'} @ ${config.bitrate || '16M'}`);
    console.log(`[+] Video Buffer:    0 ms (Ultra-Low Latency Mode)`);
    console.log(`[+] Always-On-Top:   ${config.alwaysOnTop !== false ? 'Enabled' : 'Disabled'}`);
  }

  console.log(`\n[+] Launching scrcpy with direct GPU rendering...\n`);

  const proc = spawn(scrcpyPath, args, {
    cwd: path.dirname(scrcpyPath),
    stdio: 'inherit',
  });

  return { process: proc, caps };
}
