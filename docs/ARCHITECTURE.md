# Botfield WebSocket 架构说明

## 架构概览

```
┌─────────────┐         ┌─────────────┐         ┌─────────────┐
│   前端 Vue  │◄────────┤  后端服务器  ├────────►│ C++ Bridge  │
│  (浏览器)   │  WebSocket  (Node.js)  │  WebSocket │  + C++ 进程 │
└─────────────┘         └─────────────┘         └─────────────┘
  localhost:5173         localhost:3126          连接到后端
```

## 组件说明

### 1. 后端服务器 (`backend/server.js`)
- **端口**: 3126
- **功能**:
  - WebSocket 服务器,管理所有连接
  - HTTP API 端点 (`/api/health`, `/api/status`)
  - 转发 C++ 数据到前端
  - 转发前端命令到 C++

### 2. C++ Bridge (`client/src/bridge-client.js`)
- **功能**:
  - 启动 C++ 可执行文件
  - 连接到后端 WebSocket (`ws://localhost:3126?type=cpp`)
  - 解析 C++ 输出中的 JSON 数据
  - 转发给后端服务器

### 3. 前端 Vue (`frontend/src/App.vue`)
- **端口**: 5173 (Vite dev server)
- **功能**:
  - 连接到后端 WebSocket (`ws://localhost:3126`)
  - 实时显示游戏数据和排行榜
  - 支持自动重连

### 4. Vite 代理配置
- 将 `/api/*` 请求代理到 `http://localhost:3126`

## 启动步骤

### 方法 1: 使用启动脚本 (推荐)
```powershell
./start.ps1
```
这会自动启动后端和前端,然后手动运行:
```powershell
npm run dev:bridge
```

### 方法 2: 手动启动
1. 启动后端服务器:
   ```powershell
   npm run dev:backend
   ```

2. 启动前端:
   ```powershell
   npm run dev:fe
   ```

3. 启动 C++ Bridge:
   ```powershell
   npm run dev:bridge
   ```

## 数据流

### C++ → 前端
1. C++ 输出 `JSON_DATA:{...}`
2. Bridge 解析并发送到后端
3. 后端广播给所有前端
4. 前端更新 UI

### 前端 → C++
1. 前端发送 WebSocket 消息到后端
2. 后端转发给 C++ Bridge
3. Bridge 可以控制 C++ 进程

## API 端点

- `GET /api/health` - 健康检查
  ```json
  {
    "status": "ok",
    "connections": {
      "cpp": "connected",
      "frontends": 1
    }
  }
  ```

- `GET /api/status` - 连接状态
  ```json
  {
    "cppConnected": true,
    "frontendCount": 1
  }
  ```

## 配置修改

### 修改端口
在 `backend/server.js` 中修改 `PORT` 常量,同时更新:
- `client/src/bridge-client.js` 中的 `BACKEND_WS_URL`
- `frontend/src/App.vue` 中的 `port` 变量
- `vite.config.js` 中的代理 target

### C++ 可执行文件路径
在 `client/src/bridge-client.js` 中修改 `EXE_PATH`

## 故障排查

1. **前端无法连接**
   - 检查后端是否运行: `http://localhost:3126/api/health`
   - 查看浏览器控制台 WebSocket 错误

2. **C++ Bridge 无法连接**
   - 确保后端先启动
   - 检查 WebSocket URL 是否正确

3. **没有数据显示**
   - 确认 C++ 进程是否正常运行
   - 检查 C++ 是否输出 `JSON_DATA:` 格式的数据
