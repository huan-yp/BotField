# SEU 校内快速使用

## 安装

### 安装 nodejs 和 git

[安装 nodejs](https://nodejs.org/zh-cn)
[安装 git](https://github.com/git-for-windows/git/releases/download/v2.52.0.windows.1/Git-2.52.0-64-bit.exe)

### 下载本项目

```bash
git clone https://github.com/huan-yp/botfield.git
```
### 安装本项目依赖

进入本项目目录后（README.md 所在目录），终端执行：

```bash
cd botfield
npm install
```

## 配置

照抄

```yaml
backend_listen: localhost:3126  # 后端监听地址和端口 (只配置 Client 不需要管这个)
backend_url: ws://botzone.m5d431.cn?type=cpp # SEU 校内的服务器
total_games: 20           # 对局总数
player_number: 12         # 玩家数量
bot_dir: bots            # Bot 目录
default_bot: demo    # 默认 Bot（不要写后缀名）
```

## 跑

```powershell
npm run dev:bridge
```