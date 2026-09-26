[CmdletBinding()]
param(
    [Parameter(Mandatory=$true)][string]$UnrealRoot,
    [Parameter(Mandatory=$true)][string]$ExpectedCommit,
    [Parameter(Mandatory=$true)][string]$EvidencePath,
    [Parameter(Mandatory=$true)][string]$UnrealLogPath,
    [int]$SampleWindowSeconds = 60
)

# Bounded unattended startup/memory check for the opt-in Low-Spec Development
# profile. It reuses the -LowSpec/-MeasureMemory launch contract of
# run_phase2_first_playtest.ps1 (same resolution, FPS cap, scalability commands,
# and WorkingSet64 sampling) and adds only unattended-safety flags. It records
# machine-measurable process facts; it never judges visuals, controls, or gameplay.

$ErrorActionPreference = "Stop"

$SchemaVersion = 1
$RepoRoot = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
$ProjectPath = Join-Path $RepoRoot "unreal\Everward.uproject"
$DedicatedSentinel = Join-Path $RepoRoot ".git\everward-unattended-worker"
$EditorExe = Join-Path $UnrealRoot "Engine\Binaries\Win64\UnrealEditor.exe"
$SampleWindowSeconds = [Math]::Min([Math]::Max($SampleWindowSeconds, 30), 300)
$ShutdownGraceSeconds = 30

# Keep identical to the -LowSpec branch of run_phase2_first_playtest.ps1.
# tools/test_unattended_low_spec_startup.py enforces parity.
$LowSpecCommands = @("t.MaxFPS 30", "r.ScreenPercentage 65", "sg.ViewDistanceQuality 0", "sg.AntiAliasingQuality 0", "sg.ShadowQuality 0", "sg.GlobalIlluminationQuality 0", "sg.ReflectionQuality 0", "sg.PostProcessQuality 0", "sg.TextureQuality 0", "sg.EffectsQuality 0", "sg.FoliageQuality 0", "r.Streaming.PoolSize 384")
$LowSpecLaunchArguments = @("-WINDOWED", "-ResX=1280", "-ResY=720")
$UnattendedArguments = @("-Unattended", "-NoSplash", "-NoSound")

$Evidence = [ordered]@{
    schema_version = $SchemaVersion
    profile = "low_spec_development"
    git_commit = $null
    expected_commit = $ExpectedCommit.ToLowerInvariant()
    launch_contract = [ordered]@{
        source = "tools/run_phase2_first_playtest.ps1 -LowSpec -MeasureMemory"
        resolution = "1280x720"
        windowed = $true
        exec_cmds = $LowSpecCommands
        unattended_flags = $UnattendedArguments
    }
    startup = [ordered]@{
        status = "not_run"
        launched = $false
        alive_through_sample_window = $false
        exited_during_sample_window = $false
        exit_code = $null
        main_window_observed = $false
        seconds_to_main_window = $null
    }
    memory = [ordered]@{
        metric = "editor_peak_working_set_mib"
        source = "UnrealEditor process WorkingSet64"
        interpretation = "process working set only; not total system RAM or shared-GPU memory"
        sample_window_seconds = $SampleWindowSeconds
        sample_count = 0
        peak_working_set_mib = $null
        final_working_set_mib = $null
    }
    cleanup = [ordered]@{
        status = "not_needed"
        method = $null
    }
    product_reality_claimed = $false
    failure_reason = $null
}

function Write-Evidence {
    $Parent = Split-Path -Parent $EvidencePath
    if ($Parent) { New-Item -ItemType Directory -Force -Path $Parent | Out-Null }
    $Evidence | ConvertTo-Json -Depth 8 | Set-Content -Path $EvidencePath -Encoding UTF8
}

function Stop-EditorTree {
    param([System.Diagnostics.Process]$Process)
    if ($Process.HasExited) { $Evidence.cleanup.status = "already_exited"; return }
    try { [void]$Process.CloseMainWindow() } catch {}
    if ($Process.WaitForExit($ShutdownGraceSeconds * 1000)) {
        $Evidence.cleanup.status = "stopped"; $Evidence.cleanup.method = "close_main_window"; return
    }
    & taskkill.exe /PID $Process.Id /T /F | Out-Null
    if ($Process.WaitForExit(15000)) {
        $Evidence.cleanup.status = "stopped"; $Evidence.cleanup.method = "taskkill_tree"; return
    }
    $Evidence.cleanup.status = "failed"; $Evidence.cleanup.method = "taskkill_tree"
}

$EditorProcess = $null
try {
    if (-not (Test-Path $DedicatedSentinel -PathType Leaf)) { throw "dedicated_checkout_sentinel_missing" }
    if ($ExpectedCommit -notmatch '^[0-9a-fA-F]{40}$') { throw "invalid_expected_commit" }
    $GitCommit = (& git -C $RepoRoot rev-parse HEAD).Trim().ToLowerInvariant()
    if ($LASTEXITCODE -ne 0 -or -not $GitCommit) { throw "git_commit_unavailable" }
    $Evidence.git_commit = $GitCommit
    if ($GitCommit -ne $Evidence.expected_commit) { throw "commit_identity_mismatch" }
    if (-not (Test-Path $ProjectPath -PathType Leaf)) { throw "project_missing" }
    if (-not (Test-Path $EditorExe -PathType Leaf)) { throw "unreal_editor_missing" }
    if (Get-Process -Name "UnrealEditor", "UnrealEditor-Cmd" -ErrorAction SilentlyContinue) { throw "unreal_editor_already_running" }

    $LogParent = Split-Path -Parent $UnrealLogPath
    if ($LogParent) { New-Item -ItemType Directory -Force -Path $LogParent | Out-Null }
    $EditorArguments = @("`"$ProjectPath`"") + $LowSpecLaunchArguments + @("-ExecCmds=`"$($LowSpecCommands -join ",")`"") + $UnattendedArguments + @("-abslog=`"$UnrealLogPath`"")

    $Launched = Get-Date
    $EditorProcess = Start-Process -FilePath $EditorExe -ArgumentList $EditorArguments -PassThru
    # Caching the handle keeps ExitCode readable if the editor exits early.
    $null = $EditorProcess.Handle
    $Evidence.startup.launched = $true

    $Deadline = $Launched.AddSeconds($SampleWindowSeconds)
    $PeakWorkingSetBytes = 0L
    $LastWorkingSetBytes = 0L
    while ((Get-Date) -lt $Deadline -and -not $EditorProcess.HasExited) {
        $EditorProcess.Refresh()
        $LastWorkingSetBytes = $EditorProcess.WorkingSet64
        if ($LastWorkingSetBytes -gt 0) {
            $Evidence.memory.sample_count += 1
            $PeakWorkingSetBytes = [Math]::Max($PeakWorkingSetBytes, $LastWorkingSetBytes)
        }
        if (-not $Evidence.startup.main_window_observed -and $EditorProcess.MainWindowHandle -ne [IntPtr]::Zero) {
            $Evidence.startup.main_window_observed = $true
            $Evidence.startup.seconds_to_main_window = [Math]::Round(((Get-Date) - $Launched).TotalSeconds, 1)
        }
        Start-Sleep -Milliseconds 500
    }
    if ($PeakWorkingSetBytes -gt 0) {
        $Evidence.memory.peak_working_set_mib = [Math]::Round($PeakWorkingSetBytes / 1MB, 1)
        $Evidence.memory.final_working_set_mib = [Math]::Round($LastWorkingSetBytes / 1MB, 1)
    }

    if ($EditorProcess.HasExited) {
        $Evidence.startup.exited_during_sample_window = $true
        $Evidence.startup.exit_code = $EditorProcess.ExitCode
        $Evidence.startup.status = "fail"
        $Evidence.failure_reason = "editor_exited_during_sample_window"
    }
    else {
        $Evidence.startup.alive_through_sample_window = $true
        $Evidence.startup.status = "pass"
    }
}
catch {
    if ($Evidence.startup.status -ne "fail") { $Evidence.startup.status = "unavailable" }
    if (-not $Evidence.failure_reason) {
        $Message = [string]$_.Exception.Message
        $Evidence.failure_reason = if ($Message -match '^[a-z_]+$') { $Message } else { "launcher_exception" }
    }
}
finally {
    if ($null -ne $EditorProcess) {
        try { Stop-EditorTree -Process $EditorProcess } catch { $Evidence.cleanup.status = "failed" }
    }
    Write-Evidence
}

Write-Host ("Low-spec startup status={0} peak_working_set_mib={1} samples={2} cleanup={3}" -f $Evidence.startup.status, $Evidence.memory.peak_working_set_mib, $Evidence.memory.sample_count, $Evidence.cleanup.status)
if ($Evidence.startup.status -eq "pass" -and $Evidence.cleanup.status -ne "failed") { exit 0 }
if ($Evidence.startup.status -eq "fail") { exit 1 }
exit 2
