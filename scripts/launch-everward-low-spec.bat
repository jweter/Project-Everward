@echo off
setlocal

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

echo Starting Everward Low-Spec Play Mode...
echo Project: %PROJECT%
echo Profile: %PROFILE%

"%UE_EDITOR%" "%PROJECT%" -game -windowed -ResX=1280 -ResY=720 -ExecCmds="t.MaxFPS 30,r.ScreenPercentage 70,sg.ViewDistanceQuality 0,sg.AntiAliasingQuality 1,sg.ShadowQuality 0,sg.GlobalIlluminationQuality 0,sg.ReflectionQuality 0,sg.PostProcessQuality 0,sg.TextureQuality 1,sg.EffectsQuality 0,sg.FoliageQuality 0,sg.ShadingQuality 0,r.Streaming.PoolSize 512,r.Streaming.LimitPoolSizeToVRAM 1,r.VolumetricFog 0,r.MotionBlurQuality 0,r.BloomQuality 1,r.DepthOfFieldQuality 0,r.LensFlareQuality 0"

endlocal
