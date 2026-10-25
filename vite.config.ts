import react from '@vitejs/plugin-react'
import { defineConfig } from 'vite'
import { adbBridgePlugin } from './server/adbBridge.ts'

// https://vite.dev/config/
export default defineConfig({
  plugins: [react(), adbBridgePlugin()],
  server: {
    host: true,
    port: 5173,
    strictPort: true,
  },
})
