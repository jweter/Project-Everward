@echo off
setlocal EnableExtensions EnableDelayedExpansion

set "PROJECT=%~dp0..\unreal\Everward.uproject"
set "PROFILE=%~dp0..\unreal\Config\EmergencyMinimum.ini"
if "%UE_EDITOR%"=="" set "UE_EDITOR=C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe"

if not exist "%PROJECT%" (
  echo ERROR: Everward.uproject not found at "%PROJECT%"
  exit /b 1
)
if not exist "%PROFILE%" (
  echo ERROR: Emergency profile not found at "%PROFILE%"
  exit /b 1
)
if not exist "%UE_EDITOR%" (
  echo ERROR: UnrealEditor.exe not found. Set UE_EDITOR to its exact path.
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

echo Starting Everward Emergency / Minimum Mode...
echo Profile: %PROFILE%
"%UE_EDITOR%" "%PROJECT%" -game -windowed -ResX=960 -ResY=540 -ExecCmds="%EXEC_CMDS%"

endlocal
