@echo off
setlocal EnableExtensions
title Everward Unattended Testing Setup
color 0A

echo.
echo ============================================================
echo   EVERWARD - UNATTENDED WINDOWS TEST WORKER SETUP
echo ============================================================
echo.
echo This is a one-time setup. It will register Everward to test
echo the latest merged main branch after this laptop is idle.
echo.

set "SETUPROOT=%TEMP%\EverwardUnattendedSetup"
set "TOOLDIR=%SETUPROOT%\tools\unattended"
if exist "%SETUPROOT%" rmdir /s /q "%SETUPROOT%"
mkdir "%TOOLDIR%" >nul 2>&1

echo Downloading the current merged worker setup from GitHub...
powershell.exe -NoProfile -ExecutionPolicy Bypass -Command "$ErrorActionPreference='Stop'; $base='https://raw.githubusercontent.com/jweter/Project-Everward/main/tools/unattended'; Invoke-WebRequest -UseBasicParsing ($base + '/register_everward_windows_worker.ps1') -OutFile '%TOOLDIR%\register_everward_windows_worker.ps1'; Invoke-WebRequest -UseBasicParsing ($base + '/bootstrap_everward_windows_worker.ps1') -OutFile '%TOOLDIR%\bootstrap_everward_windows_worker.ps1'"
if errorlevel 1 goto :failed

echo Registering the 10-minute-idle worker...
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%TOOLDIR%\register_everward_windows_worker.ps1" -IdleMinutes 10
if errorlevel 1 goto :failed

echo.
echo ============================================================
echo   SETUP COMPLETE
echo ============================================================
echo.
echo You can close this window. Everward will now use its own clean
echo checkout of the latest merged main branch when the laptop is idle.
echo.
echo Test evidence is stored under:
echo   %%LOCALAPPDATA%%\Everward\unattended-worker\
echo.
echo If GitHub CLI is already authenticated, changed results are also
echo posted automatically to Project-Everward issue #240.
echo.
echo You do NOT need to launch Unreal or repeat routine test sequences.
echo This window will close automatically.
timeout /t 12 /nobreak >nul
exit /b 0

:failed
color 0C
echo.
echo ============================================================
echo   SETUP NEEDS ATTENTION
echo ============================================================
echo.
echo The unattended worker was not registered. Nothing has been marked
echo as passing. Please keep this window open or take a screenshot of it.
echo.
pause
exit /b 1
