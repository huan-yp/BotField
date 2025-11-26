<script setup>
import { ref, computed, onMounted, onUnmounted } from 'vue';
import { Trophy, Activity, Crown, TrendingDown, TrendingUp, RefreshCw, WifiOff } from 'lucide-vue-next';

// 状态定义
const gameInfo = ref({ current: 0, total: 16 }); // total 默认为 16，会被后端的 init 覆盖
const players = ref([]);
const isConnected = ref(false);
const connectionError = ref('');
// 动态颜色映射表，key 是玩家 name，value 是 tailwind class
const playerColorMap = ref({});

let ws = null;
let reconnectTimer = null;

// 预定义颜色调色板 (Tailwind 类名)，用于循环分配给动态加入的玩家
const palette = [
  'bg-red-500',
  'bg-orange-500',
  'bg-yellow-500',
  'bg-green-500',
  'bg-emerald-500',
  'bg-teal-500',
  'bg-cyan-500',
  'bg-blue-500',
  'bg-indigo-500',
  'bg-violet-500',
  'bg-purple-500',
  'bg-fuchsia-500',
  'bg-pink-500',
  'bg-rose-500'
];

// 获取颜色 (如果没分配过则默认灰色)
const getColor = (name) => playerColorMap.value[name] || 'bg-gray-500';

// 计算分数区间 (Min-Max Normalization) - 适配负分
const scoreStats = computed(() => {
  if (players.value.length === 0) return { min: 0, max: 100, spread: 100 };
  
  const scores = players.value.map(p => p.score);
  const min = Math.min(...scores, 0); 
  const max = Math.max(...scores, 0);
  const spread = max - min || 1; 
  
  return { min, max, spread };
});

// 计算进度条宽度
const getBarWidth = (score) => {
  const { min, spread } = scoreStats.value;
  // 公式：(当前分 - 最低分) / 总跨度
  const percentage = ((score - min) / spread) * 100;
  return `${Math.max(percentage, 2)}%`; 
};

// 处理接收到的数据
const handleData = (payload) => {
  if (!payload) return;

  // 1. 处理初始化信息 (C++ 启动时发送)
  if (payload.type === 'init') {
    console.log('接收到初始化配置:', payload);
    
    // 更新总局数
    if (payload.total_games) {
      gameInfo.value.total = payload.total_games;
    }
    
    // 初始化玩家列表和颜色
    if (payload.players && Array.isArray(payload.players)) {
      const initPlayers = [];
      payload.players.forEach((p, index) => {
        // 动态分配颜色
        if (!playerColorMap.value[p.name]) {
          playerColorMap.value[p.name] = palette[index % palette.length];
        }
        
        initPlayers.push({
          name: p.name,
          exe: p.exe,
          score: 0,
          wins: 0
        });
      });
      // 只有当当前没有玩家数据时才初始化，避免覆盖已有的进度
      if (players.value.length === 0) {
        players.value = initPlayers;
      }
    }
  }
  // 2. 处理排名更新数据 (每一局结束发送)
  else if (payload.type === 'rank_update') {
    // 防御性更新：如果后端在update里也带了total，则同步更新
    if (payload.total_games) gameInfo.value.total = payload.total_games;

    // 核心修复：C++ 传过来的是 0-15，前端显示修复为 1-16
    const displayRound = payload.game_num + 1;
    gameInfo.value.current = Math.min(displayRound, gameInfo.value.total);
    
    // 确保颜色映射存在 (防止中途加入或重连错过 init)
    payload.data.forEach((p, index) => {
        if (!playerColorMap.value[p.name]) {
            playerColorMap.value[p.name] = palette[index % palette.length];
        }
    });

    players.value = [...payload.data].sort((a, b) => b.score - a.score);
  }
};

// WebSocket 连接 (带重连机制)
const connectWebSocket = () => {
  if (ws) {
    ws.close();
  }

  // 连接到后端 WebSocket 服务器
  const protocol = window.location.protocol === 'https:' ? 'wss:' : 'ws:';
  const host = window.location.hostname;
  const port = '3126'; // 后端服务器端口
  const wsUrl = `${protocol}//${host}:${port}/ws`;

  console.log(`正在尝试连接到后端: ${wsUrl}`);
  ws = new WebSocket(wsUrl);

  ws.onopen = () => {
    isConnected.value = true;
    connectionError.value = '';
    console.log('已连接到后端服务器');
    if (reconnectTimer) {
      clearTimeout(reconnectTimer);
      reconnectTimer = null;
    }
  };

  ws.onmessage = (event) => {
    try {
      const data = JSON.parse(event.data);
      handleData(data);
    } catch (e) {
      console.error('数据解析错误:', e);
    }
  };

  ws.onclose = () => {
    isConnected.value = false;
    console.log('连接断开，3秒后尝试重连...');
    scheduleReconnect();
  };
  
  ws.onerror = (err) => {
    isConnected.value = false;
    connectionError.value = '连接失败，请检查 bridge_server.js 是否运行';
    console.error('WebSocket Error:', err);
  };
};

const scheduleReconnect = () => {
  if (!reconnectTimer) {
    reconnectTimer = setTimeout(() => {
      console.log('尝试重连...');
      connectWebSocket();
      reconnectTimer = null;
    }, 3000);
  }
};

onMounted(() => {
  connectWebSocket();
});

onUnmounted(() => {
  if (ws) ws.close();
  if (reconnectTimer) clearTimeout(reconnectTimer);
});
</script>

<template>
  <div class="min-h-screen bg-slate-900 text-white p-4 font-sans selection:bg-blue-500 selection:text-white overflow-hidden">
    <!-- 头部区域 -->
    <header class="max-w-4xl mx-auto mb-8 text-center pt-8">
      <h1 class="text-4xl font-extrabold mb-2 tracking-tight text-transparent bg-clip-text bg-gradient-to-r from-blue-400 to-emerald-400">
        斗地主 Bot 实时对战
      </h1>
      
      <!-- 状态指示器 -->
      <div class="flex items-center justify-center gap-2 text-sm mb-6 transition-colors duration-300"
           :class="isConnected ? 'text-emerald-400' : 'text-rose-400'">
        <div class="w-2 h-2 rounded-full" :class="isConnected ? 'bg-emerald-500 animate-pulse' : 'bg-rose-500'"></div>
        <span v-if="isConnected">LIVE DATA CONNECTION</span>
        <span v-else class="flex items-center gap-1">
          <WifiOff :size="14" />
          DISCONNECTED - RETRYING...
        </span>
      </div>

      <!-- 全局进度条 -->
      <div class="relative w-full h-4 bg-slate-800 rounded-full overflow-hidden shadow-inner border border-slate-700">
        <div 
          class="absolute top-0 left-0 h-full bg-gradient-to-r from-blue-600 to-cyan-500 transition-all duration-700 ease-out"
          :style="{ width: `${Math.min((gameInfo.current / (gameInfo.total || 1)) * 100, 100)}%` }"
        ></div>
        <div class="absolute inset-0 flex items-center justify-center text-xs font-bold text-white drop-shadow-md">
          GAME {{ gameInfo.current }} / {{ gameInfo.total }}
        </div>
      </div>
    </header>

    <!-- 排行榜列表区域 -->
    <main class="max-w-4xl mx-auto relative min-h-[400px]">
      <!-- 表头 -->
      <div class="grid grid-cols-12 gap-4 text-slate-400 text-sm font-semibold uppercase tracking-wider mb-2 px-4">
        <div class="col-span-1 text-center">Rank</div>
        <div class="col-span-3">Player</div>
        <div class="col-span-6">Performance (Relative)</div>
        <div class="col-span-2 text-right">Score</div>
      </div>

      <!-- 列表容器 -->
      <TransitionGroup name="list" tag="div" class="flex flex-col gap-3 relative">
        <div 
          v-for="(player, index) in players" 
          :key="player.name"
          class="relative bg-slate-800/80 backdrop-blur-sm border border-slate-700 rounded-xl p-3 shadow-lg hover:scale-[1.01] hover:bg-slate-700 transition-colors duration-300"
          :style="{ zIndex: players.length - index }"
        >
          <div class="grid grid-cols-12 gap-4 items-center relative z-10">
            
            <!-- 排名 -->
            <div class="col-span-1 flex justify-center">
              <Crown v-if="index === 0" class="w-8 h-8 text-yellow-400 fill-yellow-400/20 drop-shadow-glow" />
              <Trophy v-else-if="index === 1" class="w-7 h-7 text-slate-300" />
              <Trophy v-else-if="index === 2" class="w-6 h-6 text-amber-600" />
              <span v-else class="text-xl font-bold text-slate-500">#{{ index + 1 }}</span>
            </div>

            <!-- 玩家信息 -->
            <div class="col-span-3 flex items-center gap-3">
              <div class="w-10 h-10 rounded-lg flex items-center justify-center shadow-lg text-white font-bold" :class="getColor(player.name)">
                {{ player.name.substring(0, 1).toUpperCase() }}
              </div>
              <div class="flex flex-col">
                <span class="font-bold text-lg leading-tight">{{ player.name }}</span>
                <span class="text-xs text-slate-500 font-mono">{{ player.exe }}</span>
              </div>
            </div>

            <!-- 进度条可视化 (支持负数) -->
            <div class="col-span-6 flex flex-col justify-center gap-1">
              <div class="flex justify-between text-xs text-slate-400 mb-0.5">
                <span class="opacity-50 text-[10px]">Low: {{ scoreStats.min }}</span>
                <span class="opacity-50 text-[10px]">High: {{ scoreStats.max }}</span>
              </div>
              <div class="w-full h-3 bg-slate-900 rounded-full overflow-hidden relative border border-slate-700/50">
                <!-- 0分参考线 (当分数跨越正负时显示) -->
                <div v-if="scoreStats.min < 0 && scoreStats.max > 0" 
                     class="absolute top-0 bottom-0 w-[1px] bg-slate-500 z-10 opacity-50"
                     :style="{ left: `${((0 - scoreStats.min) / scoreStats.spread) * 100}%` }">
                </div>

                <div 
                  class="absolute top-0 left-0 h-full transition-all duration-1000 ease-out rounded-full shadow-[0_0_10px_rgba(255,255,255,0.3)]"
                  :class="getColor(player.name)"
                  :style="{ width: getBarWidth(player.score) }"
                >
                  <div class="absolute top-0 right-0 bottom-0 w-2 bg-white/50 blur-[2px]"></div>
                </div>
              </div>
            </div>

            <!-- 具体数值 -->
            <div class="col-span-2 flex flex-col items-end justify-center">
              <div class="text-2xl font-bold font-mono tracking-tight" 
                   :class="player.score >= 0 ? 'text-white' : 'text-red-400'">
                {{ player.score > 0 ? '+' : '' }}{{ player.score }}
              </div>
              <div class="text-xs font-medium flex items-center gap-1"
                   :class="player.score >= 0 ? 'text-emerald-400' : 'text-rose-400'">
                <component :is="player.score >= 0 ? TrendingUp : TrendingDown" :size="12" />
                {{ player.wins }} Wins
              </div>
            </div>

          </div>
        </div>
      </TransitionGroup>
      
      <!-- 无数据/断开连接状态 -->
      <div v-if="players.length === 0" class="absolute inset-0 flex flex-col items-center justify-center text-slate-500 bg-slate-900/50 backdrop-blur-sm z-20">
        <div v-if="!isConnected" class="flex flex-col items-center gap-3 animate-pulse">
            <RefreshCw class="animate-spin text-blue-500" :size="48" />
            <p class="text-lg font-medium text-slate-400">正在寻找对战服务器...</p>
            <p class="text-xs text-slate-600">确保运行: node bridge_server.js</p>
        </div>
        <div v-else class="text-center">
            <Activity class="mx-auto mb-2 text-slate-600" :size="48" />
            <p>已连接，等待初始数据 (Init)...</p>
        </div>
      </div>
    </main>
    
    <!-- 底部装饰 -->
    <div class="fixed bottom-4 right-4 text-slate-600 text-xs font-mono">
      RENDER: Vue 3 + Tailwind • ENGINE: CPP-MCTS
    </div>
  </div>
</template>

<style>
/* 列表动画 */
.list-move,
.list-enter-active,
.list-leave-active {
  transition: all 0.7s cubic-bezier(0.25, 1, 0.5, 1);
}
.list-enter-from,
.list-leave-to {
  opacity: 0;
  transform: translateX(30px);
}
.list-leave-active {
  position: absolute;
  width: 100%;
}
</style>