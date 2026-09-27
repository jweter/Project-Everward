[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)][string]$UnrealRoot,
    [Parameter(Mandatory = $true)][string]$EvidencePath,
    [Parameter(Mandatory = $true)][string]$UnrealLogPath,
    [int]$StartupTimeoutSeconds = 900
)

$ErrorActionPreference = "Stop"

function Get-ProcessTreeIds([int]$RootPid) {
    $all = @(Get-CimInstance Win32_Process -ErrorAction SilentlyContinue)
    $ids = [System.Collections.Generic.HashSet[int]]::new()
    [void]$ids.Add($RootPid)
    $changed = $true
    while ($changed) {
        $changed = $false
        foreach ($item in $all) {
            if ($ids.Contains([int]$item.ParentProcessId) -and -not $ids.Contains([int]$item.ProcessId)) {
                [void]$ids.Add([int]$item.ProcessId)
                $changed = $true
            }
        }
    }
    return @($ids)
}

# Bounded unattended startup/memory probe. The launch contract mirrors the opt-in
# `tools/run_phase2_first_playtest.ps1 -LowSpec -MeasureMemory` path exactly; a
# regression test keeps the two command lists identical. The only unattended
# additions are `-Unattended` (no modal dialogs) and `-abslog` (log outside the
# checkout). No project config, scalability default, or simulation state is changed.
# The evidence file intentionally contains no machine paths.
$SampleWindowSeconds = 60
$SampleIntervalMilliseconds = 500
$EngineInitializedMarker = "Engine is initialized."
$StartupTimeoutSeconds = [Math]::Max($StartupTimeoutSeconds, $SampleWindowSeconds)

$RepoRoot = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
$ProjectPath = Join-Path $RepoRoot "unreal\Everward.uproject"
$LowSpecCommandList = @("t.MaxFPS 30", "r.ScreenPercentage 65", "sg.ViewDistanceQuality 0", "sg.AntiAliasingQuality 0", "sg.ShadowQuality 0", "sg.GlobalIlluminationQuality 0", "sg.ReflectionQuality 0", "sg.PostProcessQuality 0", "sg.TextureQuality 0", "sg.EffectsQuality 0", "sg.FoliageQuality 0", "r.Streaming.PoolSize 384")
$LowSpecCommands = $LowSpecCommandList -join ","

$Evidence = [ordered]@{
    schema_version = 1
    kind = "everward_unattended_low_spec_startup"
    git_commit = ""
    profile = "low_spec_development"
    launch_contract = "tools/run_phase2_first_playtest.ps1 -LowSpec -MeasureMemory"
    resolution = "1280x720"
    exec_cmds = $LowSpecCommandList
    unattended_additions = @("-Unattended", "-abslog")
    launched = $false
    startup = [ordered]@{
        status = "not_started"
        engine_initialized_marker_observed = $false
        seconds_to_engine_initialized = $null
        exit_code = $null
        timeout_seconds = $StartupTimeoutSeconds
    }
    memory = [ordered]@{
        metric = "editor_peak_working_set_mib"
        value = $null
        sample_window_seconds = $SampleWindowSeconds
        sample_interval_ms = $SampleIntervalMilliseconds
        sample_count = 0
        source = "UnrealEditor process WorkingSet64"
        interpretation = "process working set only; not total system RAM or shared-GPU memory"
    }
    cleanup = [ordered]@{
        attempted = $false
        process_tree_terminated = $false
    }
    error = ""
}

$ExitCode = 3
$Process = $null
try {
    if (-not (Test-Path $ProjectPath -PathType Leaf)) { throw "Everward project was not found in the checkout." }
    $EditorExe = Join-Path $UnrealRoot "Engine\Binaries\Win64\UnrealEditor.exe"
    if (-not (Test-Path $EditorExe -PathType Leaf)) { throw "Unreal Engine 5.8 UnrealEditor.exe was not found." }
    $GitCommit = (& git -C $RepoRoot rev-parse HEAD).Trim()
    if ($LASTEXITCODE -ne 0 -or -not $GitCommit) { throw "Unable to resolve the current Git commit." }
    $Evidence.git_commit = $GitCommit.ToLowerInvariant()

    if (Test-Path $UnrealLogPath -PathType Leaf) { Remove-Item -Force $UnrealLogPath }
    $EditorArguments = @("`"$ProjectPath`"", "-log", "-WINDOWED", "-ResX=1280", "-ResY=720", "-ExecCmds=`"$LowSpecCommands`"", "-Unattended", "-abslog=`"$UnrealLogPath`"")
    $Process = Start-Process -FilePath $EditorExe -ArgumentList $EditorArguments -PassThru
    # Cache the process handle so ExitCode remains readable after an early exit.
    $null = $Process.Handle
    $Evidence.launched = $true
    $Stopwatch = [Diagnostics.Stopwatch]::StartNew()

    $PeakWorkingSetBytes = 0L
    $SampleCount = 0
    $MarkerSeconds = $null
    while ($true) {
        $Elapsed = $Stopwatch.Elapsed.TotalSeconds
        if ($Process.HasExited) { break }
        if ($Elapsed -lt $SampleWindowSeconds) {
            try {
                $Process.Refresh()
                $WorkingSet = $Process.WorkingSet64
                if ($WorkingSet -gt 0) {
                    $PeakWorkingSetBytes = [Math]::Max($PeakWorkingSetBytes, $WorkingSet)
                    $SampleCount++
                }
            }
            catch {}
        }
        if ($null -eq $MarkerSeconds -and (Test-Path $UnrealLogPath -PathType Leaf)) {
            try {
                if (Select-String -Path $UnrealLogPath -SimpleMatch -Pattern $EngineInitializedMarker -Quiet) {
                    $MarkerSeconds = [Math]::Round($Elapsed, 1)
                }
            }
            catch {}
        }
        if ($null -ne $MarkerSeconds -and $Elapsed -ge $SampleWindowSeconds) { break }
        if ($Elapsed -ge $StartupTimeoutSeconds) { break }
        Start-Sleep -Milliseconds $SampleIntervalMilliseconds
    }

    $Evidence.memory.sample_count = $SampleCount
    if ($SampleCount -gt 0) { $Evidence.memory.value = [Math]::Round($PeakWorkingSetBytes / 1MB, 1) }
    $Evidence.startup.engine_initialized_marker_observed = ($null -ne $MarkerSeconds)
    $Evidence.startup.seconds_to_engine_initialized = $MarkerSeconds

    if ($Process.HasExited) {
        $Evidence.startup.status = "exited_before_evidence_complete"
        $Evidence.startup.exit_code = $Process.ExitCode
        $Evidence.cleanup.process_tree_terminated = $true
        $ExitCode = 1
    }
    elseif ($null -ne $MarkerSeconds) {
        $Evidence.startup.status = "initialized"
        $ExitCode = 0
    }
    else {
        $Evidence.startup.status = "timeout_before_engine_initialized"
        $ExitCode = 2
    }
}
catch {
    $Evidence.error = $_.Exception.Message
    $ExitCode = 3
}
finally {
    if ($null -ne $Process -and -not $Process.HasExited) {
        $Evidence.cleanup.attempted = $true
        $TreeIds = @(Get-ProcessTreeIds $Process.Id)
        try { & taskkill.exe /PID $Process.Id /T /F | Out-Null } catch {}
        try { $null = $Process.WaitForExit(30000) } catch {}
        if (-not $Process.HasExited) {
            try { Stop-Process -Id $Process.Id -Force -ErrorAction SilentlyContinue } catch {}
            try { $null = $Process.WaitForExit(10000) } catch {}
        }
        $Survivors = @($TreeIds | Where-Object { Get-Process -Id $_ -ErrorAction SilentlyContinue })
        # PASS requires proof that the complete captured launch tree is gone, not only
        # the UnrealEditor root. This is intentionally fail-closed when any descendant
        # survives taskkill/Stop-Process cleanup.
        $Evidence.cleanup.process_tree_terminated = ($Process.HasExited -and $Survivors.Count -eq 0)
    }
    if ($ExitCode -eq 0 -and ($Evidence.memory.sample_count -le 0 -or -not $Evidence.cleanup.process_tree_terminated)) { $ExitCode = 2 }
    $EvidenceDirectory = Split-Path -Parent $EvidencePath
    if ($EvidenceDirectory) { New-Item -ItemType Directory -Force -Path $EvidenceDirectory | Out-Null }
    $Evidence | ConvertTo-Json -Depth 6 | Set-Content -Path $EvidencePath -Encoding UTF8
}

Write-Host ("Low-spec startup status: {0}; peak working set (first {1}s): {2} MiB; samples: {3}" -f $Evidence.startup.status, $SampleWindowSeconds, $Evidence.memory.value, $Evidence.memory.sample_count)
exit $ExitCode
