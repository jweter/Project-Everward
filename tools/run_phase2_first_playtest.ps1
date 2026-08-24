[CmdletBinding()]
param(
    [string]$UnrealRoot = "",
    [switch]$SkipBuild,
    [switch]$NoLaunch
)

$ErrorActionPreference = "Stop"

$RepoRoot = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
$ProjectPath = Join-Path $RepoRoot "unreal\Everward.uproject"
$TemplateHelper = Join-Path $PSScriptRoot "prepare_phase2_first_run_observation.py"

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
Write-Host ""

if (-not $SkipBuild) {
    Write-Host "Building EverwardEditor (Win64 Development)..."
    & $BuildBat EverwardEditor Win64 Development $ProjectPath -WaitMutex -NoHotReloadFromIDE
    $BuildExitCode = $LASTEXITCODE

    if ($BuildExitCode -ne 0) {
        Set-ObservationCheck -ObservationPath $ObservationPath -CheckId "unreal_cpp_build" -Status "fail" -Notes "UnrealBuildTool exited with code $BuildExitCode."
        Add-ObservationBlocker -ObservationPath $ObservationPath -Blocker "Unreal C++ build failed with exit code $BuildExitCode."
        Write-Error "EverwardEditor build failed. Observation recorded at $ObservationPath"
        exit $BuildExitCode
    }

    Set-ObservationCheck -ObservationPath $ObservationPath -CheckId "unreal_cpp_build" -Status "pass" -Notes "EverwardEditor Win64 Development build completed successfully via Unreal Engine 5.8 Build.bat."
    Write-Host "Build passed and was recorded in the observation file."
}
else {
    Write-Host "Build skipped by request; unreal_cpp_build remains not_tested."
}

if (-not $NoLaunch) {
    Write-Host "Launching Unreal Editor with log window..."
    Start-Process -FilePath $EditorExe -ArgumentList @("`"$ProjectPath`"", "-log")
    Write-Host "Unreal Editor launched. Follow docs/PHASE2_FIRST_RUN_PLAYTEST.md and update the observation file as you test."
}
else {
    Write-Host "Editor launch skipped by request."
}

Write-Host ""
Write-Host "Validate the completed observation with:"
Write-Host "  python tools\validate_phase2_first_run_observation.py `"$ObservationPath`""
