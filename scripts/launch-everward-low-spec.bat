@echo off
setlocal EnableExtensions EnableDelayedExpansion

set "PROJECT=%~dp0..\unreal\Everward.uproject"
set "PROFILE=%~dp0..\unreal\Config\LowSpecPlay.ini"

if not exist "%PROJECT%" (
  echo ERROR: Everward.uproject not found at "%PROJECT%"
  exit /b 1
)

if not exist "%PROFILE%" (
  echo ERROR: Low-spec profile not found at "%PROFILE%"
  exit /b 1
)

if "%UE_EDITOR%"=="" (
  set "UE_EDITOR=C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe"
)

if not exist "%UE_EDITOR%" (
  echo ERROR: UnrealEditor.exe not found.
  echo Set UE_EDITOR to the exact UnrealEditor.exe path and run again.
  exit /b 1
)

set "EXEC_CMDS="
for /f "usebackq tokens=1,* delims==" %%A in ("%PROFILE%") do (
  set "KEY=%%A"
  if defined KEY if not "!KEY:~0,1!"==";" if not "!KEY:~0,1!"=="[" (
    if defined EXEC_CMDS (set "EXEC_CMDS=!EXEC_CMDS!,%%A=%%B") else set "EXEC_CMDS=%%A=%%B"
  )
)

if not defined EXEC_CMDS (
  echo ERROR: No console variables were loaded from "%PROFILE%".
  exit /b 1
)

echo Starting Everward Low-Spec Play Mode...
echo Project: %PROJECT%
echo Profile: %PROFILE%

"%UE_EDITOR%" "%PROJECT%" -game -windowed -ResX=1280 -ResY=720 -ExecCmds="%EXEC_CMDS%"

endlocal
