import { defineConfig } from 'vite'
import vue from '@vitejs/plugin-vue'

// https://vite.dev/config/
export default defineConfig({
  plugins: [vue()],
  base: './',  // Use relative paths for JUCE ResourceProvider
  build: {
    outDir: 'dist',
    assetsInlineLimit: 100000,  // Inline assets < 100KB as data URIs
    rollupOptions: {
      output: {
        // Single JS bundle for easy embedding
        manualChunks: undefined,
      },
    },
  },
  server: {
    port: 3000,  // Match JUCE localDevServerAddress
  },
})
