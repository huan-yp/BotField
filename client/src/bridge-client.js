/**
 * C++ Bridge - 连接到后端服务器
 * 负责启动 C++ 进程并将其输出转发到后端 WebSocket 服务器
 */

const { spawn } = require('child_process');
const WebSocket = require('ws');
const path = require('path');
const fs = require('fs');
const yaml = require('js-yaml');

// 配置
const EXE_PATH = './client/build/main.exe'; // Windows
// const EXE_PATH = './client/build/main'; // Mac/Linux
const DEFAULT_WS_URL = 'ws://localhost:3126?type=cpp';

// 读取配置文件
const configPath = path.join(__dirname, '../../config.yaml');
let config;
try {
  const configFile = fs.readFileSync(configPath, 'utf8');
  config = yaml.load(configFile);
} catch (e) {
  console.error('❌ 无法读取配置文件:', e.message);
  process.exit(1);
}


const BACKEND_WS_URL = config.backend_url || DEFAULT_WS_URL;

let ws = null;
let reconnectTimer = null;
let cppProcess = null;

// 连接到后端服务器
function connectToBackend() {
  if (ws) {
    ws.close();
  }

  console.log(`正在连接到后端服务器: ${BACKEND_WS_URL}`);
  ws = new WebSocket(BACKEND_WS_URL);

  ws.on('open', () => {
    console.log('✅ 已连接到后端服务器');
    if (reconnectTimer) {
      clearTimeout(reconnectTimer);
      reconnectTimer = null;
    }
    
    // 连接成功后启动 C++ 进程
    if (!cppProcess) {
      startCppProcess();
    }
  });

  ws.on('message', (data) => {
    try {
      const message = JSON.parse(data.toString());
      console.log('[后端 → Bridge] 收到消息:', message);
      
      // 可以处理后端发来的控制命令
      if (message.type === 'command') {
        console.log('收到命令:', message.command);
      }
    } catch (e) {
      console.error('解析后端消息失败:', e);
    }
  });

  ws.on('close', () => {
    console.log('❌ 与后端服务器断开连接');
    scheduleReconnect();
  });

  ws.on('error', (error) => {
    console.error('WebSocket 错误:', error.message);
  });
}

// 发送数据到后端
function sendToBackend(data) {
  if (ws && ws.readyState === WebSocket.OPEN) {
    ws.send(JSON.stringify(data));
  } else {
    console.warn('⚠️  后端未连接,数据未发送:', data.type);
  }
}

// 启动 C++ 进程
function startCppProcess() {
  if (!fs.existsSync(EXE_PATH)) {
    console.error(`❌ 找不到可执行文件: ${EXE_PATH}`);
    sendToBackend({
      type: 'sys_error',
      message: `找不到可执行文件: ${EXE_PATH}`
    });
    return;
  }

  console.log(`🚀 启动 C++ 进程: ${EXE_PATH}`);
  cppProcess = spawn(EXE_PATH);

  // 处理标准输出
  cppProcess.stdout.on('data', (data) => {
    const text = data.toString();
    const lines = text.split('\n');

    lines.forEach(line => {
      const trimmed = line.trim();
      if (!trimmed) return;

      // 检测 JSON 数据
      if (trimmed.startsWith('JSON_DATA:')) {
        try {
          const jsonStr = trimmed.replace('JSON_DATA:', '');
          const jsonData = JSON.parse(jsonStr);
          
          // 转发到后端
          sendToBackend(jsonData);
          
          // 本地日志
          if (jsonData.game_num !== undefined) {
            const currentRound = jsonData.game_num + 1;
            process.stdout.write(`\r[Game ${currentRound}/${jsonData.total_games}] Running...   `);
          }
        } catch (e) {
          console.error('JSON 解析错误:', e);
          sendToBackend({
            type: 'sys_error',
            message: `JSON 解析错误: ${e.message}`
          });
        }
      } else {
        // 普通日志
        if (trimmed.startsWith('result:')) console.log('');
        console.log(`[C++] ${trimmed}`);
        
        sendToBackend({
          type: 'cpp_log',
          message: trimmed
        });
      }
    });
  });

  // 处理标准错误
  cppProcess.stderr.on('data', (data) => {
    const errorMsg = data.toString();
    console.error(`[C++ ERROR] ${errorMsg}`);
    
    sendToBackend({
      type: 'sys_error',
      message: errorMsg
    });
  });

  // 进程退出
  cppProcess.on('close', (code) => {
    console.log(`\n✅ C++ 进程结束 (退出码: ${code})`);
    
    sendToBackend({
      type: 'sys_log',
      message: `C++ 进程结束 (退出码: ${code})`
    });
    
    cppProcess = null;
    
    // 根据需要决定是否自动重启
    // 这里选择不自动重启,让用户手动控制
  });
}

// 重连调度
function scheduleReconnect() {
  if (!reconnectTimer) {
    reconnectTimer = setTimeout(() => {
      console.log('🔄 尝试重新连接...');
      connectToBackend();
      reconnectTimer = null;
    }, 3000);
  }
}

// 优雅关闭
function cleanup() {
  console.log('\n正在清理资源...');
  
  if (cppProcess) {
    console.log('正在终止 C++ 进程...');
    cppProcess.kill();
  }
  
  if (ws) {
    ws.close();
  }
  
  if (reconnectTimer) {
    clearTimeout(reconnectTimer);
  }
  
  process.exit(0);
}

process.on('SIGINT', cleanup);
process.on('SIGTERM', cleanup);

// 启动
console.log('═══════════════════════════════════════');
console.log('🎮 C++ Bridge Client');
console.log('═══════════════════════════════════════');
connectToBackend();
