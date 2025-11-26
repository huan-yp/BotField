# 分布式部署指南

公网机器跑前后端,内网机器跑 C++ 客户端

## 架构

```
公网服务器 (1.2.3.4)
  ├─ 后端: node backend/server.js  (localhost:3126)
  └─ 前端: npm run dev:fe          (localhost:5173)
        ↑ WebSocket
内网机器 (任意地址)
  └─ C++ Bridge: node client/src/bridge-client.js → ws://1.2.3.4:3126
```

## 公网服务器部署

### 安装

```bash
# 1. 安装 Node.js
curl -fsSL https://deb.nodesource.com/setup_18.x | sudo -E bash -
sudo apt-get install -y nodejs git

# 2. 克隆并安装
git clone https://github.com/huan-yp/botfield.git
cd botfield && npm install
```

### 配置

填写 `config.yaml`:

```yaml
backend_listen: localhost:3126  # 后端监听地址和端口 (只配置 Client 不需要管这个)
# backend_listen: 0.0.0.0:3126 # 允许外网连接
backend_url: ws://localhost:3126?type=cpp # client 连接地址（写后端的 ip 和端口）
# backend_url: ws://botzone.m5d431.cn?type=cpp # SEU 校内的服务器
total_games: 20           # 对局总数
player_number: 12         # 玩家数量
bot_dir: bots            # Bot 目录
default_bot: demo    # 默认 Bot（不要写后缀名）
```

开防火墙

```bash
sudo ufw allow 5173/tcp
sudo ufw allow 3126/tcp
```

### 启动

```bash
screen -S backend
node backend/server.js

screen -S frontend
npm run dev:fe
```

