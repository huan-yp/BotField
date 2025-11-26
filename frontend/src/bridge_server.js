/**
 * 增强版后端转发服务器 (修复版)
 * 修复了下标从 0 开始导致显示少一局的问题
 */

const { spawn } = require('child_process');
const WebSocket = require('ws');
const path = require('path');
const fs = require('fs');

// 配置：根据你的系统调整
const EXE_NAME = './backend/build/main.exe'; // Windows
// const EXE_NAME = './battlefield'; // Mac/Linux

const EXE_PATH = EXE_NAME;
const WS_PORT = 3000;

// 1. 启动 WebSocket 服务器
const wss = new WebSocket.Server({ port: WS_PORT });
console.log(`WebSocket server started on port ${WS_PORT}`);

// 广播工具函数
function broadcast(data) {
    wss.clients.forEach(client => {
        if (client.readyState === WebSocket.OPEN) {
            client.send(JSON.stringify(data));
        }
    });
}

wss.on('connection', (ws) => {
    console.log('Frontend connected');
    ws.send(JSON.stringify({ type: 'sys_log', message: 'Connected to Bridge Server' }));
    
    if (!fs.existsSync(EXE_PATH)) {
        ws.send(JSON.stringify({ type: 'sys_error', message: `❌ 找不到文件: ${EXE_NAME}。请确保已编译 C++ 代码并放在同级目录。` }));
    }
});

// 2. 启动 C++ 进程
if (fs.existsSync(EXE_PATH)) {
    console.log(`Launching ${EXE_NAME}...`);
    const child = spawn(EXE_PATH);

    // 处理 C++ 标准输出
    child.stdout.on('data', (data) => {
        const text = data.toString();
        const lines = text.split('\n');

        lines.forEach(line => {
            const trimmed = line.trim();
            if (!trimmed) return;

            // 情况 A: 是我们定义的 JSON 数据
            if (trimmed.startsWith('JSON_DATA:')) {
                try {
                    const jsonStr = trimmed.replace('JSON_DATA:', '');
                    const jsonData = JSON.parse(jsonStr);
                    broadcast(jsonData); // 转发数据
                    
                    if (jsonData.game_num !== undefined) {
                        // 核心修复：C++ 是 0-based，这里 +1 转换为人类可读的 1-based
                        const currentRound = jsonData.game_num + 1;
                        process.stdout.write(`\r[Game ${currentRound}/${jsonData.total_games}] Updating...   `);
                    }
                } catch (e) {
                    console.error('JSON Parse Error:', e);
                }
            } 
            // 情况 B: 是普通日志 (如 "game 0 is over...")
            else {
                // 如果是 C++ 的结果总结，打印个换行，避免被进度条覆盖
                if (trimmed.startsWith('result:')) console.log('');
                
                console.log(`[CPP] ${trimmed}`);
                broadcast({ type: 'cpp_log', message: trimmed });
            }
        });
    });

    child.stderr.on('data', (data) => {
        console.error(`[CPP ERROR] ${data}`);
        broadcast({ type: 'sys_error', message: data.toString() });
    });

    child.on('close', (code) => {
        console.log(`\nProcess exited with code ${code}`);
        broadcast({ type: 'sys_log', message: `C++ Process finished (Exit Code: ${code})` });
    });
} else {
    console.error("Executable not found.");
}