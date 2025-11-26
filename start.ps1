# 启动脚本 - 依次启动后端和前端

Write-Host "========================================" -ForegroundColor Cyan
Write-Host "   启动 Botfield 完整系统" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""

# 检查依赖
if (-not (Test-Path "node_modules")) {
    Write-Host "⚠️  未检测到 node_modules，正在安装依赖..." -ForegroundColor Yellow
    npm install
}

# 检查 express 是否安装
$packageJson = Get-Content "package.json" | ConvertFrom-Json
if (-not $packageJson.dependencies.express) {
    Write-Host "📦 安装 express..." -ForegroundColor Yellow
    npm install express
}

Write-Host ""
Write-Host "🚀 启动后端服务器..." -ForegroundColor Green
Start-Process pwsh -ArgumentList "-NoExit", "-Command", "npm run dev:backend"

Start-Sleep -Seconds 2

Write-Host "🌐 启动前端开发服务器..." -ForegroundColor Green
Start-Process pwsh -ArgumentList "-NoExit", "-Command", "npm run dev:fe"

Start-Sleep -Seconds 2

Write-Host ""
Write-Host "========================================" -ForegroundColor Cyan
Write-Host "✅ 系统已启动！" -ForegroundColor Green
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""
Write-Host "访问地址:" -ForegroundColor White
Write-Host "  前端: http://localhost:5173" -ForegroundColor Cyan
Write-Host "  后端 API: http://localhost:3126/api/health" -ForegroundColor Cyan
Write-Host ""
Write-Host "启动 C++ Bridge:" -ForegroundColor Yellow
Write-Host "  npm run dev:bridge" -ForegroundColor Cyan
Write-Host ""
Write-Host "按 Ctrl+C 停止各个进程窗口" -ForegroundColor Gray
