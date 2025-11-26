@echo off
echo ==========================================
echo Starting Battlefield AI System...
echo ==========================================

:: 1. Start Backend (bridge_server.js)
:: Key: Use 'start' command to run the backend in a new window
:: This ensures it doesn't block the current script execution
:: /min runs the window minimized
echo Step 1/2: Starting Backend Server...
start "Battlefield Backend" /min node bridge_server.js

:: 2. Start Frontend (Vue)
:: Run frontend in the current window to see the output URLs:
:: "- Local: http://localhost:8080/"
:: "- Network: http://10.x.x.x:8080/"
echo Step 2/2: Starting Frontend Interface...
cd vision

:: Start service and expose IP for LAN access
npm run serve -- --host 0.0.0.0