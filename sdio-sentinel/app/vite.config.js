// SDIO Sentinel Vite configuration. Author: jayis1.
import { defineConfig } from 'vite';
import react from '@vitejs/plugin-react';

export default defineConfig({
  plugins: [react()],
  server: { host: '127.0.0.1', port: 4173 },
  build: { sourcemap: true },
});
