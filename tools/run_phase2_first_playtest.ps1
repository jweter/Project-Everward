[CmdletBinding()]
param(
    [string]$UnrealRoot = "",
    [switch]$SkipBuild,
    [switch]$NoLaunch,
    [switch]$LowSpec
)

$ErrorActionPreference = "Stop"

$RepoRoot = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
$ProjectPath = Join-Path $RepoRoot "unreal\Everward.uproject"
$TemplateHelper = Join-Path $PSScriptRoot "prepare_phase2_first_run_observation.py"
$WorkerRegistration = Join-Path $PSScriptRoot "register_unattended_product_reality.ps1"
$DedicatedSentinel = Join-Path $RepoRoot ".git\everward-unattended-worker"
$ManualPlaytestLock = Join-Path $RepoRoot ".git\everward-manual-playtest.lock"

# A normal playtest must never authorize an arbitrary development clone for
# destructive unattended sync. It may only refresh registration after the
# explicit one-time dedicated-checkout setup has already created the sentinel.
if ((Test-Path $DedicatedSentinel -PathType Leaf) -and (Test-Path $WorkerRegistration -PathType Leaf)) {
    try {
        & powershell -NoProfile -ExecutionPolicy Bypass -File $WorkerRegistration -RepoRoot $RepoRoot
        if ($LASTEXITCODE -ne 0) {
            Write-Warning "Everward unattended worker registration returned exit code $LASTEXITCODE. Manual playtest will continue."
        }
    }
    catch {
        Write-Warning "Everward unattended worker registration failed: $($_.Exception.Message). Manual playtest will continue."
    }
}

function Resolve-Unreal58Root {
    param([string]$ExplicitRoot)

    $Candidates = New-Object System.Collections.Generic.List[string]
    if ($ExplicitRoot) { $Candidates.Add($ExplicitRoot) }

    foreach ($Name in @("UE58_ROOT", "UE_5_8_ROOT", "UE_5_8")) {
        $Value = [Environment]::GetEnvironmentVariable($Name)
        if ($Value) { $Candidates.Add($Value) }
    }

    foreach ($RegistryPath in @(
        "HKLM:\SOFTWARE\EpicGames\Unreal Engine\5.8",
        "HKCU:\SOFTWARE\EpicGames\Unreal Engine\5.8",
        "HKLM:\SOFTWARE\WOW6432Node\EpicGames\Unreal Engine\5.8"
    )) {
        try {
            $InstalledDirectory = (Get-ItemProperty -Path $RegistryPath -Name InstalledDirectory -ErrorAction Stop).InstalledDirectory
            if ($InstalledDirectory) { $Candidates.Add($InstalledDirectory) }
        }
        catch {
            # Registry entry is optional; continue to other discovery methods.
        }
    }

    foreach ($CommonPath in @(
        "C:\Program Files\Epic Games\UE_5.8",
        "C:\Epic Games\UE_5.8"
    )) {
        $Candidates.Add($CommonPath)
    }

    foreach ($Candidate in $Candidates | Select-Object -Unique) {
        $BuildBat = Join-Path $Candidate "Engine\Build\BatchFiles\Build.bat"
        $EditorExe = Join-Path $Candidate "Engine\Binaries\Win64\UnrealEditor.exe"
        if ((Test-Path $BuildBat) -and (Test-Path $EditorExe)) {
            return (Resolve-Path $Candidate).Path
        }
    }

    throw "Unreal Engine 5.8 was not found. Re-run with -UnrealRoot 'C:\path\to\UE_5.8' or set UE58_ROOT."
}

function Set-ObservationCheck {
    param(
        [string]$ObservationPath,
        [string]$CheckId,
        [ValidateSet("pass", "fail", "not_tested")][string]$Status,
        [string]$Notes
    )

    $Data = Get-Content -Raw -Path $ObservationPath | ConvertFrom-Json
    $Data.checks.$CheckId.status = $Status
    $Data.checks.$CheckId.notes = $Notes

    if ($Status -eq "fail") {
        $Data.overall_result = "fail"
    }
    elseif ($Data.overall_result -eq "not_tested") {
        $Data.overall_result = "partial"
    }

    $Data | ConvertTo-Json -Depth 12 | Set-Content -Path $ObservationPath -Encoding UTF8
}

function Add-ObservationBlocker {
    param([string]$ObservationPath, [string]$Blocker)
    $Data = Get-Content -Raw -Path $ObservationPath | ConvertFrom-Json
    $Data.blockers += $Blocker
    $Data | ConvertTo-Json -Depth 12 | Set-Content -Path $ObservationPath -Encoding UTF8
}

if (-not (Test-Path $ProjectPath)) {
    throw "Everward project not found at $ProjectPath"
}

$ResolvedUnrealRoot = Resolve-Unreal58Root -ExplicitRoot $UnrealRoot
$BuildBat = Join-Path $ResolvedUnrealRoot "Engine\Build\BatchFiles\Build.bat"
$EditorExe = Join-Path $ResolvedUnrealRoot "Engine\Binaries\Win64\UnrealEditor.exe"

$Python = (Get-Command python -ErrorAction Stop).Source
$GitCommit = (& git -C $RepoRoot rev-parse HEAD).Trim()
if ($LASTEXITCODE -ne 0 -or -not $GitCommit) {
    throw "Unable to resolve the current Git commit."
}

$Cpu = ""
$Gpu = ""
try {
    $Cpu = (Get-CimInstance Win32_Processor | Select-Object -First 1 -ExpandProperty Name).Trim()
}
catch {}
try {
    $Gpu = (Get-CimInstance Win32_VideoController | Select-Object -First 1 -ExpandProperty Name).Trim()
}
catch {}

$Timestamp = (Get-Date).ToUniversalTime().ToString("yyyyMMdd-HHmmss")
$ObservationPath = Join-Path $RepoRoot "playtests\phase2\observations\phase2-first-run-$Timestamp.json"

$NeedsManualLock = (-not $SkipBuild) -or (-not $NoLaunch)
if ($NeedsManualLock) {
    $PID.ToString() | Set-Content -Path $ManualPlaytestLock -Encoding ASCII
}

try {
    & $Python $TemplateHelper --output $ObservationPath --git-commit $GitCommit --cpu $Cpu --gpu $Gpu
    if ($LASTEXITCODE -ne 0) {
        throw "Failed to prepare the Phase 2 first-run observation file."
    }

    Write-Host ""
    Write-Host "Everward Phase 2 first-run harness"
    Write-Host "  Repo:       $RepoRoot"
    Write-Host "  Commit:     $GitCommit"
    Write-Host "  Unreal 5.8: $ResolvedUnrealRoot"
    Write-Host "  Evidence:   $ObservationPath"
    if ($LowSpec) {
        Write-Host "  Profile:    Low-Spec Development (720p / 30 FPS / reduced presentation cost)"
    }
    Write-Host ""

    if (-not $SkipBuild) {
        Write-Host "Building EverwardEditor (Win64 Development)..."
        $BuildArguments = @("EverwardEditor", "Win64", "Development", $ProjectPath, "-WaitMutex", "-NoHotReloadFromIDE")
        if ($LowSpec) {
            $BuildArguments += "-MaxParallelActions=2"
        }
        & $BuildBat @BuildArguments
        $BuildExitCode = $LASTEXITCODE

        if ($BuildExitCode -ne 0) {
            Set-ObservationCheck -ObservationPath $ObservationPath -CheckId "unreal_cpp_build" -Status "fail" -Notes "UnrealBuildTool exited with code $BuildExitCode."
            Add-ObservationBlocker -ObservationPath $ObservationPath -Blocker "Unreal C++ build failed with exit code $BuildExitCode."
            throw "EverwardEditor build failed with exit code $BuildExitCode. Observation recorded at $ObservationPath"
        }

        $BuildNotes = "EverwardEditor Win64 Development build completed successfully via Unreal Engine 5.8 Build.bat."
        if ($LowSpec) {
            $BuildNotes += " Low-Spec Development mode constrained UnrealBuildTool to at most 2 parallel actions."
        }
        Set-ObservationCheck -ObservationPath $ObservationPath -CheckId "unreal_cpp_build" -Status "pass" -Notes $BuildNotes
        Write-Host "Build passed and was recorded in the observation file."
    }
    else {
        Write-Host "Build skipped by request; unreal_cpp_build remains not_tested."
    }

    if (-not $NoLaunch) {
        $EditorArguments = @("`"$ProjectPath`"", "-log")
        if ($LowSpec) {
            $LowSpecCommands = @(
                "t.MaxFPS 30",
                "r.ScreenPercentage 65",
                "sg.ViewDistanceQuality 0",
                "sg.AntiAliasingQuality 0",
                "sg.ShadowQuality 0",
                "sg.GlobalIlluminationQuality 0",
                "sg.ReflectionQuality 0",
                "sg.PostProcessQuality 0",
                "sg.TextureQuality 0",
                "sg.EffectsQuality 0",
                "sg.FoliageQuality 0",
                "r.Streaming.PoolSize 384"
            ) -join ","
            $EditorArguments += @(
                "-WINDOWED",
                "-ResX=1280",
                "-ResY=720",
                "-ExecCmds=`"$LowSpecCommands`""
            )
            Write-Host "Launching Unreal Editor in opt-in Low-Spec Development mode..."
        }
        else {
            Write-Host "Launching Unreal Editor with log window..."
        }
        Start-Process -FilePath $EditorExe -ArgumentList $EditorArguments

        # Keep the shared manual lock until the editor process has had a chance
        # to become visible. After that, the unattended worker also detects
        # UnrealEditor.exe directly and will continue to defer safely.
        $LaunchDeadline = (Get-Date).AddSeconds(10)
        while ((Get-Date) -lt $LaunchDeadline) {
            if (Get-Process -Name "UnrealEditor" -ErrorAction SilentlyContinue) {
                break
            }
            Start-Sleep -Milliseconds 200
        }
        Write-Host "Unreal Editor launched. Follow docs/PHASE2_FIRST_RUN_PLAYTEST.md and update the observation file as you test."
    }
    else {
        Write-Host "Editor launch skipped by request."
    }

    Write-Host ""
    Write-Host "Validate the completed observation with:"
    Write-Host "  python tools\validate_phase2_first_run_observation.py `"$ObservationPath`""
}
finally {
    if ($NeedsManualLock -and (Test-Path $ManualPlaytestLock -PathType Leaf)) {
        Remove-Item -Force $ManualPlaytestLock -ErrorAction SilentlyContinue
    }
}
