# Botfield 技术架构文档

## 架构概览

### 系统拓扑

```
┌──────────────────┐
│  浏览器 (前端)    │  Vue 3 + Vite
│  localhost:5173  │  实时展示排行榜
└────────┬─────────┘
         │ WebSocket
         ▼
┌──────────────────┐
│  后端服务器       │  Node.js + Express
│  localhost:3126  │  • WebSocket Server
│                  │  • HTTP API
│                  │  • 消息转发中心
└────────┬─────────┘
         │ WebSocket
         ▼
┌──────────────────┐
│  C++ Bridge      │  Node.js 客户端
│                  │  • 启动 C++ 进程
│                  │  • 解析 JSON 输出
│  └─> C++ 引擎   │  • 数据转发
│      battlefield │  C++ + OpenMP
└──────────────────┘
```

## 核心组件

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

### 4. Vite 配置 (`vite.config.js`)
- **开发服务器**: 5173 端口
- **API 代理**: `/api/*` → `http://localhost:3126`
- **WebSocket**: 自动处理升级请求

## 数据流详解

### 上行流 (C++ → 前端)

```
[C++ 进程] 
    ↓ stdout 输出
    JSON_DATA:{"type":"rank_update",...}
    ↓
[Bridge Client]
    ↓ 解析 JSON
    ↓ WebSocket.send()
    ↓
[后端服务器]
    ↓ 广播 broadcast()
    ↓
[前端 Vue] ← ← ← 所有连接的浏览器
    ↓ 更新 reactive state
    ↓
[UI 更新] 排行榜实时刷新
```

**关键消息类型:**
- `init`: 初始化配置 (玩家列表、总局数)
- `rank_update`: 每局结束后的排名更新
- `cpp_log`: C++ 进程的日志输出
- `sys_error`: 系统错误信息

### 下行流 (前端 → C++)

```
[前端 Vue]
    ↓ ws.send()
    ↓
[后端服务器]
    ↓ 路由到 C++ 客户端
    ↓
[Bridge Client]
    ↓ 转发到 C++ stdin
    ↓
[C++ 进程] 接收控制命令
```

*当前版本主要为单向数据流 (C++ → 前端),双向控制功能预留*

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

## 端口配置

修改端口需要同步更新以下文件:

| 文件 | 配置项 | 默认值 |
|------|--------|--------|
| `backend/server.js` | `PORT` | 3126 |
| `client/src/bridge-client.js` | `BACKEND_WS_URL` | ws://localhost:3126 |
| `frontend/src/App.vue` | `port` | 3126 |
| `vite.config.js` | `server.proxy['/api'].target` | http://localhost:3126 |

## 故障排查指南

### 问题: 前端显示 "DISCONNECTED"

**排查步骤:**
1. 检查后端服务
   ```powershell
   curl http://localhost:3126/api/health
   ```
   应返回: `{"status":"ok",...}`

2. 查看浏览器控制台
   - 打开 DevTools (F12) → Console
   - 查找 WebSocket 连接错误

3. 验证端口占用
   ```powershell
   netstat -ano | findstr :3126
   ```

**常见原因:**
- 后端未启动
- 端口被占用
- 防火墙拦截

### 问题: C++ Bridge 无法连接

**排查步骤:**
1. 确认后端已启动
2. 检查 `client/src/bridge-client.js` 中的 URL
3. 查看 Bridge 终端输出错误信息

### 问题: 没有游戏数据

**排查步骤:**
1. 确认 C++ 可执行文件存在
   ```powershell
   Test-Path ./client/build/main.exe
   ```

2. 检查 C++ 输出格式
   - 必须包含 `JSON_DATA:` 前缀
   - JSON 格式必须正确

3. 验证 config.yaml 配置
   - bot_dir 路径是否正确
   - default_bot 文件是否存在
