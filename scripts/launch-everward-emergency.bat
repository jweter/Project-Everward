@echo off
setlocal

set "PROJECT=%~dp0..\unreal\Everward.uproject"
if "%UE_EDITOR%"=="" set "UE_EDITOR=C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe"

if not exist "%PROJECT%" (
  echo ERROR: Everward.uproject not found at "%PROJECT%"
  exit /b 1
)
if not exist "%UE_EDITOR%" (
  echo ERROR: UnrealEditor.exe not found. Set UE_EDITOR to its exact path.
  exit /b 1
)

echo Starting Everward Emergency / Minimum Mode...
"%UE_EDITOR%" "%PROJECT%" -game -windowed -ResX=960 -ResY=540 -ExecCmds="t.MaxFPS 30,r.ScreenPercentage 50,sg.ViewDistanceQuality 0,sg.AntiAliasingQuality 0,sg.ShadowQuality 0,sg.GlobalIlluminationQuality 0,sg.ReflectionQuality 0,sg.PostProcessQuality 0,sg.TextureQuality 0,sg.EffectsQuality 0,sg.FoliageQuality 0,sg.ShadingQuality 0,r.Streaming.PoolSize 256,r.Streaming.LimitPoolSizeToVRAM 1,r.VolumetricFog 0,r.MotionBlurQuality 0,r.BloomQuality 0,r.DepthOfFieldQuality 0,r.LensFlareQuality 0"

endlocal
