[CmdletBinding()]
param(
    [string]$UnrealRoot = "",
    [int]$StartupTimeoutSeconds = 900
)

$ErrorActionPreference = "Stop"
# Preserve the public parameter while enforcing the same bounded cold-start floor
# already required by tools/run_unreal_headless_smoke.ps1 for UE 5.8 on this hardware.
$EffectiveStartupTimeoutSeconds = [Math]::Max($StartupTimeoutSeconds, 900)
$SampleWindowSeconds = 60
$RepoRoot = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
$ProjectPath = Join-Path $RepoRoot "unreal\Everward.uproject"

if (-not (Test-Path $ProjectPath -PathType Leaf)) { throw "Everward project not found at $ProjectPath" }

$Candidates = New-Object System.Collections.Generic.List[string]
if ($UnrealRoot) { $Candidates.Add($UnrealRoot) }
foreach ($Name in @("UE58_ROOT", "UE_5_8_ROOT", "UE_5_8")) { $Value = [Environment]::GetEnvironmentVariable($Name); if ($Value) { $Candidates.Add($Value) } }
foreach ($CommonPath in @("C:\Program Files\Epic Games\UE_5.8", "C:\Epic Games\UE_5.8")) { $Candidates.Add($CommonPath) }
$EditorExe = $null
foreach ($Candidate in $Candidates | Select-Object -Unique) {
    $CandidateExe = Join-Path $Candidate "Engine\Binaries\Win64\UnrealEditor.exe"
    if (Test-Path $CandidateExe -PathType Leaf) { $EditorExe = (Resolve-Path $CandidateExe).Path; break }
}
if (-not $EditorExe) { throw "Unreal Engine 5.8 editor executable was not found." }

$GitCommit = (& git -C $RepoRoot rev-parse HEAD).Trim()
if ($LASTEXITCODE -ne 0 -or -not $GitCommit) { throw "Unable to resolve the current Git commit." }

$StateDir = Join-Path $env:LOCALAPPDATA "Everward\unattended-worker"
$LogDir = Join-Path $StateDir "logs"
$EvidenceDir = Join-Path $StateDir "low-spec-startup"
New-Item -ItemType Directory -Force -Path $LogDir | Out-Null
New-Item -ItemType Directory -Force -Path $EvidenceDir | Out-Null
$LogPath = Join-Path $LogDir "low-spec-startup-$GitCommit.log"
$EvidencePath = Join-Path $EvidenceDir "$GitCommit.json"

# Same opt-in Low-Spec Development exec-command contract as
# tools/run_phase2_first_playtest.ps1 -LowSpec -MeasureMemory. Kept identical by
# tools/test_low_spec_startup_check_contract.py so the two launch paths cannot drift.
$LowSpecCommands = @("t.MaxFPS 30", "r.ScreenPercentage 65", "sg.ViewDistanceQuality 0", "sg.AntiAliasingQuality 0", "sg.ShadowQuality 0", "sg.GlobalIlluminationQuality 0", "sg.ReflectionQuality 0", "sg.PostProcessQuality 0", "sg.TextureQuality 0", "sg.EffectsQuality 0", "sg.FoliageQuality 0", "r.Streaming.PoolSize 384") -join ","
$Arguments = @("`"$ProjectPath`"", "-Unattended", "-NoSplash", "-NoSound", "-NoP4", "-NoAutoSDK", "-WINDOWED", "-ResX=1280", "-ResY=720", "-ExecCmds=`"$LowSpecCommands`"", "-log=`"$LogPath`"")

function Write-StartupEvidence {
    param(
        [ValidateSet("pass", "fail")][string]$Result,
        [string]$Reason,
        [double]$PeakWorkingSetMiB,
        [int]$WindowSeconds
    )
    $Evidence = [ordered]@{
        schema_version = 1
        commit = $GitCommit
        profile = "low_spec_v1"
        startup_result = $Result
        reason = $Reason
        peak_working_set_mib = $PeakWorkingSetMiB
        sample_window_seconds = $WindowSeconds
        source = "UnrealEditor process WorkingSet64"
        interpretation = "process working set and process-liveness only; not total system RAM, shared-GPU memory, frame time, or a visual/control-feel/gameplay Product Reality signal"
        recorded_at_utc = (Get-Date).ToUniversalTime().ToString("o")
    }
    $Evidence | ConvertTo-Json -Depth 6 | Set-Content -Path $EvidencePath -Encoding UTF8
}

$Process = $null
try {
    $Process = Start-Process -FilePath $EditorExe -ArgumentList $Arguments -PassThru -WindowStyle Hidden
    $LaunchDeadline = (Get-Date).AddSeconds($EffectiveStartupTimeoutSeconds)
    $Reached = $false
    while ((Get-Date) -lt $LaunchDeadline) {
        if ($Process.HasExited) { break }
        if (Get-Process -Id $Process.Id -ErrorAction SilentlyContinue) { $Reached = $true; break }
        Start-Sleep -Milliseconds 200
    }
    if ($Process.HasExited) {
        Write-StartupEvidence -Result "fail" -Reason "UnrealEditor exited during startup with exit code $($Process.ExitCode)." -PeakWorkingSetMiB 0 -WindowSeconds 0
        throw "Low-spec Unreal Editor startup failed: process exited during startup. Log: $LogPath"
    }
    if (-not $Reached) {
        Write-StartupEvidence -Result "fail" -Reason "UnrealEditor did not report a running process within $EffectiveStartupTimeoutSeconds seconds." -PeakWorkingSetMiB 0 -WindowSeconds 0
        throw "Low-spec Unreal Editor startup timed out after $EffectiveStartupTimeoutSeconds seconds. Log: $LogPath"
    }

    $PeakWorkingSetBytes = 0L
    $MemoryDeadline = (Get-Date).AddSeconds($SampleWindowSeconds)
    $CrashedDuringSample = $false
    while ((Get-Date) -lt $MemoryDeadline) {
        if ($Process.HasExited) { $CrashedDuringSample = $true; break }
        $Process.Refresh()
        $PeakWorkingSetBytes = [Math]::Max($PeakWorkingSetBytes, $Process.WorkingSet64)
        Start-Sleep -Milliseconds 500
    }
    if ($CrashedDuringSample) {
        Write-StartupEvidence -Result "fail" -Reason "UnrealEditor exited during the memory sample window with exit code $($Process.ExitCode)." -PeakWorkingSetMiB 0 -WindowSeconds $SampleWindowSeconds
        throw "Low-spec Unreal Editor crashed during the memory sample window. Log: $LogPath"
    }
    if ($PeakWorkingSetBytes -le 0) {
        Write-StartupEvidence -Result "fail" -Reason "No working-set memory sample was captured." -PeakWorkingSetMiB 0 -WindowSeconds $SampleWindowSeconds
        throw "Low-spec Unreal Editor startup produced no memory evidence. Log: $LogPath"
    }

    $PeakWorkingSetMiB = [Math]::Round($PeakWorkingSetBytes / 1MB, 1)
    Write-StartupEvidence -Result "pass" -Reason "" -PeakWorkingSetMiB $PeakWorkingSetMiB -WindowSeconds $SampleWindowSeconds
    Write-Host "PASS: Everward low-spec startup check completed for exact commit $GitCommit."
    Write-Host ("Peak working set during first {0}s: {1} MiB" -f $SampleWindowSeconds, $PeakWorkingSetMiB)
}
finally {
    if ($Process -and -not $Process.HasExited) {
        try { & taskkill.exe /PID $Process.Id /T /F | Out-Null } catch { Stop-Process -Id $Process.Id -Force -ErrorAction SilentlyContinue }
    }
}
