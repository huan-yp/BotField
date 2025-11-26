# Botfield - 斗地主 Bot 对战平台

基于 WebSocket 的实时斗地主 Bot 对战平台,支持多 Bot 竞技、实时排行榜展示。

## 技术栈

- **前端**: Vue 3 + Vite + Tailwind CSS
- **后端**: Node.js + Express + WebSocket
- **游戏引擎**: C++ (OpenMP 并行)
- **通信协议**: WebSocket + JSON

## 快速开始

[你车校内快速使用](./docs/SEU.md)

### 1. 下载本项目并安装依赖

```powershell
git clone https://github.com/huan-yp/botfield.git
cd botfield && npm install
```

### 2. 编译 C++ 游戏引擎

```powershell
cd client
make
```

### 3. 填写配置

编辑 `config.yaml` 自定义对战参数:

```yaml
backend_listen: localhost:3126      # 后端监听地址和端口
# backend_listen: 0.0.0.0:3126 # 允许外网连接
backend_url: ws://localhost:3126?type=cpp # client 连接地址（写后端的 ip 和端口）
# backend_url: ws://botzone.m5d431.cn?type=cpp # SEU 校内的服务器
total_games: 20           # 对局总数
player_number: 12         # 玩家数量
bot_dir: bots            # Bot 目录
default_bot: demo    # 默认 Bot（不要写后缀名）
```

**Bot 加载规则:**
- 自动扫描 `bot_dir` 目录下的 `.exe` 文件（忽视 default_bot）
- Bot 不足时用 `default_bot` 补全
- 未找到任何 Bot 时全部使用 `default_bot`

### 4. 启动服务器

```powershell
# 方式 A: Windows 一键启动 (推荐)
./start.ps1

# 方式 B: 分别启动
npm run dev:backend & # 终端 1: 后端服务
npm run dev:fe &       # 终端 2: 前端界面
```

### 5. 启动客户端

游戏客户端可以和服务器放在一台机器上，也可以在不同机器上。

如果在不同机器上，需要正确填写 `config.yaml` 里的 `backend_url`。

```powershell
npm run dev:bridge
```

### 6. 访问结果

浏览器打开: **http://localhost:5173**

## 项目结构

```
botfield/
├── frontend/          # Vue 前端界面
├── backend/           # Node.js WebSocket 服务器
├── client/            # C++ 游戏引擎 + Bridge 客户端
├── bots/              # Bot 可执行文件目录
├── config.yaml        # 游戏配置
└── docs/              # 文档
    ├── START_GUIDE.md      # 详细使用指南
    └── ARCHITECTURE.md     # 技术架构文档
```

## 文档

- 🏗️ [架构文档](docs/ARCHITECTURE.md) - 技术架构和数据流说明
- 🚀 [服务器部署](docs/SERVER_SIDE.md) - 公网内容服务器部署
- 🖥️ [客户端部署](docs/CLIENT_SIDE.md) - 内网机器游戏引擎部署指南

## API 端点

- **健康检查**: `GET http://localhost:3126/api/health`
- **连接状态**: `GET http://localhost:3126/api/status`

## License

MIT

