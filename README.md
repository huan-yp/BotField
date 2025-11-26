# Battlefield - 斗地主对战平台

## 配置文件

项目使用 `config.yaml` 进行配置：

```yaml
total_games: 20           # 对局总数
player_number: 12         # 玩家数量
bot_dir: bots            # bot 存放目录
default_bot: demo.exe # 默认 bot（用于补全不足的玩家）
```

### Bot 加载规则

1. 程序会自动扫描 `bot_dir` 目录下的所有 `.exe` 文件
2. 如果找到的 bot 数量少于 `player_number`，会用 `default_bot` 补全
3. 如果 `bot_dir` 中没有找到任何 bot，所有玩家都使用 `default_bot`

## 编译后端

### 使用 make（推荐）

```bash
cd backend
make
```

### 使用 G++ 直接编译

```bash
g++ -o ./backend/build/main -O2 -std=c++17 -fopenmp ./backend/src/battlefield.cpp ./backend/src/third_party/jsoncpp/jsoncpp.cpp -Ibackend/src/third_party
```

