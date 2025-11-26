/**
 * WebSocket 后端服务器
 * 负责管理 C++ 客户端连接、前端连接和数据转发
 */

const WebSocket = require('ws');
const http = require('http');
const express = require('express');
const fs = require('fs');
const yaml = require('js-yaml');
const path = require('path');

// 读取配置文件
const configPath = path.join(__dirname, '../config.yaml');
let config;
try {
  const configFile = fs.readFileSync(configPath, 'utf8');
  config = yaml.load(configFile);
} catch (e) {
  console.error('❌ 无法读取配置文件:', e.message);
  process.exit(1);
}

// 解析 backend_listen 配置 (格式: host:port)
const [HOST, PORT] = (config.backend_listen || 'localhost:3126').split(':');
const portNumber = parseInt(PORT, 10);

const app = express();

// 创建 HTTP 服务器
const server = http.createServer(app);

// 创建 WebSocket 服务器
// 在 /ws 路径上监听 WebSocket 连接
const wss = new WebSocket.Server({ server, path: '/ws' });

// 存储连接的客户端
const clients = {
  cpp: null,      // C++ 客户端 (bridge)
  frontends: new Set()  // 前端连接集合
};

// 中间件
app.use(express.json());

// 健康检查 API
app.get('/api/health', (req, res) => {
  res.json({
    status: 'ok',
    connections: {
      cpp: clients.cpp ? 'connected' : 'disconnected',
      frontends: clients.frontends.size
    },
    timestamp: new Date().toISOString()
  });
});

// 获取当前游戏状态 API
app.get('/api/status', (req, res) => {
  res.json({
    cppConnected: !!clients.cpp,
    frontendCount: clients.frontends.size
  });
});

// WebSocket 连接处理
wss.on('connection', (ws, req) => {
  const clientType = req.url.includes('type=cpp') ? 'cpp' : 'frontend';
  
  console.log(`[${new Date().toLocaleTimeString()}] 新连接: ${clientType}`);

  if (clientType === 'cpp') {
    // C++ 客户端连接
    if (clients.cpp) {
      console.log('⚠️  已有 C++ 客户端连接,关闭旧连接');
      clients.cpp.close();
    }
    
    clients.cpp = ws;
    console.log('✅ C++ 客户端已连接');
    
    // 通知所有前端
    broadcastToFrontends({
      type: 'sys_log',
      message: 'C++ 客户端已连接'
    });

    ws.on('message', (data) => {
      try {
        const message = JSON.parse(data.toString());
        console.log(`[CPP → Server] 收到数据类型: ${message.type}`);
        
        // 转发给所有前端
        broadcastToFrontends(message);
      } catch (e) {
        console.error('解析 C++ 消息失败:', e);
      }
    });

    ws.on('close', () => {
      console.log('❌ C++ 客户端断开连接');
      clients.cpp = null;
      broadcastToFrontends({
        type: 'sys_log',
        message: 'C++ 客户端已断开'
      });
    });

  } else {
    // 前端连接
    clients.frontends.add(ws);
    console.log(`✅ 前端已连接 (总数: ${clients.frontends.size})`);
    
    // 发送欢迎消息
    ws.send(JSON.stringify({
      type: 'sys_log',
      message: '已连接到后端服务器'
    }));

    // 如果 C++ 已连接,通知前端
    if (clients.cpp) {
      ws.send(JSON.stringify({
        type: 'sys_log',
        message: 'C++ 客户端在线'
      }));
    }

    ws.on('message', (data) => {
      try {
        const message = JSON.parse(data.toString());
        console.log(`[Frontend → Server] 收到消息:`, message);
        
        // 可以添加前端到 C++ 的通信逻辑
        if (clients.cpp && clients.cpp.readyState === WebSocket.OPEN) {
          clients.cpp.send(JSON.stringify(message));
        }
      } catch (e) {
        console.error('解析前端消息失败:', e);
      }
    });

    ws.on('close', () => {
      clients.frontends.delete(ws);
      console.log(`❌ 前端断开连接 (剩余: ${clients.frontends.size})`);
    });
  }

  ws.on('error', (error) => {
    console.error(`WebSocket 错误 (${clientType}):`, error.message);
  });
});

// 广播给所有前端
function broadcastToFrontends(data) {
  const message = JSON.stringify(data);
  let successCount = 0;
  
  clients.frontends.forEach(ws => {
    if (ws.readyState === WebSocket.OPEN) {
      ws.send(message);
      successCount++;
    }
  });
  
  if (successCount > 0) {
    console.log(`📤 已广播给 ${successCount} 个前端`);
  }
}

// 启动服务器
server.listen(portNumber, HOST, () => {
  console.log('═══════════════════════════════════════');
  console.log(`🚀 后端服务器已启动`);
  console.log(`📡 HTTP API: http://${HOST}:${portNumber}`);
  console.log(`🔌 WebSocket: ws://${HOST}:${portNumber}`);
  console.log('═══════════════════════════════════════');
  console.log('');
  console.log('连接说明:');
  console.log(`  - C++ 客户端: ws://${HOST}:${portNumber}?type=cpp`);
  console.log(`  - 前端客户端: ws://${HOST}:${portNumber}`);
  console.log('');
  console.log('API 端点:');
  console.log(`  - GET /api/health - 服务健康检查`);
  console.log(`  - GET /api/status - 连接状态`);
  console.log('═══════════════════════════════════════');
});

// 优雅关闭
process.on('SIGINT', () => {
  console.log('\n正在关闭服务器...');
  wss.close(() => {
    server.close(() => {
      console.log('服务器已关闭');
      process.exit(0);
    });
  });
});
