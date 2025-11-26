import { defineConfig } from 'vite'
import vue from '@vitejs/plugin-vue'
import { resolve } from 'path'

export default defineConfig({
  plugins: [vue()],
  root: resolve('frontend'),   // ← 关键
  base: './',
  server: {
    host: '0.0.0.0',
    port: 5173,
    proxy: {
      // 将 /api 开头的请求转发到后端服务器
      '/api': {
        target: 'http://localhost:3126',
        changeOrigin: true,
      },
      '/ws': {
        target: 'ws://localhost:3126/ws',
        ws: true
      }
  }
}
})