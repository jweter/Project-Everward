[CmdletBinding()]
param(
    [Parameter(Mandatory=$true)][string]$UnrealRoot,
    [Parameter(Mandatory=$true)][string]$ResultPath,
    [Parameter(Mandatory=$true)][string]$LogPath,
    [int]$StartupTimeoutSeconds = 900,
    [int]$SampleWindowSeconds = 60
)

# Bounded unattended startup/memory probe for the opt-in Low-Spec Development
# launch contract owned by tools/run_phase2_first_playtest.ps1 (-LowSpec
# -MeasureMemory). The editor arguments below must stay identical to that
# harness; tools/test_unattended_low_spec_startup.py locks the two together.
#
# This script only records raw machine facts (exact commit, whether the engine
# initialization marker appeared, elapsed time, process working set). It never
# classifies visual quality, control feel, frame time, or gameplay, and it
# never decides PASS: tools/everward_unattended_worker.py interprets the result
# fail-closed. No file paths are written into the result JSON.

$ErrorActionPreference = "Stop"

$StartupMarkers = @("Engine is initialized.", "(Engine Initialization) Total time:")
$RepoRoot = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
$ProjectPath = Join-Path $RepoRoot "unreal\Everward.uproject"
$EditorExe = Join-Path $UnrealRoot "Engine\Binaries\Win64\UnrealEditor.exe"

$Result = [ordered]@{
    schema_version = 1
    profile = "low-spec-development"
    git_commit = $null
    editor_launched = $false
    startup_marker_observed = $false
    startup_seconds = $null
    engine_reported_init_seconds = $null
    exited_before_window_end = $false
    exit_code = $null
    startup_timeout_seconds = $StartupTimeoutSeconds
    sample_window_seconds = $SampleWindowSeconds
    sampled_after_startup_seconds = 0.0
    peak_working_set_mib = $null
    memory_sample_count = 0
    telemetry_errors = 0
    cleanup = "not_started"
    error = $null
}

$EditorProcess = $null
$LogOffset = 0L
$LogCarry = ""

function Read-NewLogText {
    if (-not (Test-Path $LogPath -PathType Leaf)) { return "" }
    $Stream = $null
    try {
        $Share = [System.IO.FileShare]::ReadWrite -bor [System.IO.FileShare]::Delete
        $Stream = [System.IO.File]::Open($LogPath, [System.IO.FileMode]::Open, [System.IO.FileAccess]::Read, $Share)
        if ($Stream.Length -lt $script:LogOffset) { $script:LogOffset = 0L }
        [void]$Stream.Seek($script:LogOffset, [System.IO.SeekOrigin]::Begin)
        $Reader = New-Object System.IO.StreamReader($Stream)
        $Text = $Reader.ReadToEnd()
        $script:LogOffset = $Stream.Position
        return $Text
    }
    catch { return "" }
    finally { if ($Stream) { $Stream.Dispose() } }
}

function Update-PeakWorkingSet {
    try {
        $EditorProcess.Refresh()
        if ($EditorProcess.HasExited) { return }
        $Bytes = $EditorProcess.WorkingSet64
        $Result.memory_sample_count += 1
        $MiB = [Math]::Round($Bytes / 1MB, 1)
        if ($null -eq $Result.peak_working_set_mib -or $MiB -gt $Result.peak_working_set_mib) { $Result.peak_working_set_mib = $MiB }
    }
    catch { $Result.telemetry_errors += 1 }
}

try {
    if (-not (Test-Path $ProjectPath -PathType Leaf)) { throw "Everward project was not found." }
    if (-not (Test-Path $EditorExe -PathType Leaf)) { throw "Unreal Engine 5.8 UnrealEditor.exe was not found." }
    $GitCommit = (& git -C $RepoRoot rev-parse HEAD).Trim()
    if ($LASTEXITCODE -ne 0 -or -not $GitCommit) { throw "Unable to resolve the current Git commit." }
    $Result.git_commit = $GitCommit.ToLowerInvariant()

    $LogDirectory = Split-Path -Parent $LogPath
    if ($LogDirectory) { New-Item -ItemType Directory -Force -Path $LogDirectory | Out-Null }
    if (Test-Path $LogPath -PathType Leaf) { Remove-Item -Force $LogPath }

    # Low-Spec Development launch contract (keep identical to run_phase2_first_playtest.ps1).
    $LowSpecCommands = @("t.MaxFPS 30", "r.ScreenPercentage 65", "sg.ViewDistanceQuality 0", "sg.AntiAliasingQuality 0", "sg.ShadowQuality 0", "sg.GlobalIlluminationQuality 0", "sg.ReflectionQuality 0", "sg.PostProcessQuality 0", "sg.TextureQuality 0", "sg.EffectsQuality 0", "sg.FoliageQuality 0", "r.Streaming.PoolSize 384") -join ","
    $EditorArguments = @("`"$ProjectPath`"", "-WINDOWED", "-ResX=1280", "-ResY=720", "-ExecCmds=`"$LowSpecCommands`"")
    # Worker-only safety flags: suppress modal dialogs and route the log to the worker evidence directory.
    $EditorArguments += @("-Unattended", "-log=`"$LogPath`"")

    $Stopwatch = [System.Diagnostics.Stopwatch]::StartNew()
    $EditorProcess = Start-Process -FilePath $EditorExe -ArgumentList $EditorArguments -PassThru
    $Result.editor_launched = $true
    $Result.cleanup = "pending"

    $StartupDeadline = $StartupTimeoutSeconds
    $SampleEndSeconds = $null
    while ($true) {
        $Elapsed = $Stopwatch.Elapsed.TotalSeconds
        if ($EditorProcess.HasExited) { break }
        Update-PeakWorkingSet

        if (-not $Result.startup_marker_observed) {
            $Chunk = $LogCarry + (Read-NewLogText)
            foreach ($Marker in $StartupMarkers) {
                if ($Chunk.Contains($Marker)) {
                    $Result.startup_marker_observed = $true
                    $Result.startup_seconds = [Math]::Round($Elapsed, 1)
                    $SampleEndSeconds = $Elapsed + $SampleWindowSeconds
                    break
                }
            }
            $Match = [regex]::Match($Chunk, "\(Engine Initialization\) Total time:\s*([0-9]+(?:\.[0-9]+)?)\s*seconds")
            if ($Match.Success) { $Result.engine_reported_init_seconds = [double]$Match.Groups[1].Value }
            $LogCarry = if ($Chunk.Length -gt 512) { $Chunk.Substring($Chunk.Length - 512) } else { $Chunk }
            if (-not $Result.startup_marker_observed -and $Elapsed -ge $StartupDeadline) { break }
        }
        else {
            if ($null -eq $Result.engine_reported_init_seconds) {
                $Chunk = $LogCarry + (Read-NewLogText)
                $Match = [regex]::Match($Chunk, "\(Engine Initialization\) Total time:\s*([0-9]+(?:\.[0-9]+)?)\s*seconds")
                if ($Match.Success) { $Result.engine_reported_init_seconds = [double]$Match.Groups[1].Value }
                $LogCarry = if ($Chunk.Length -gt 512) { $Chunk.Substring($Chunk.Length - 512) } else { $Chunk }
            }
            $Result.sampled_after_startup_seconds = [Math]::Round($Elapsed - $Result.startup_seconds, 1)
            if ($Elapsed -ge $SampleEndSeconds) { break }
        }
        Start-Sleep -Milliseconds 500
    }

    $EditorProcess.Refresh()
    if ($EditorProcess.HasExited) {
        $Result.exited_before_window_end = $true
        try { $Result.exit_code = [int]$EditorProcess.ExitCode } catch { $Result.exit_code = $null }
    }
}
catch {
    $Result.error = $_.Exception.Message
}
finally {
    if ($null -ne $EditorProcess) {
        try {
            if ($EditorProcess.HasExited) { $Result.cleanup = "already_exited" }
            else {
                & taskkill.exe /PID $EditorProcess.Id /T /F | Out-Null
                if ($EditorProcess.WaitForExit(30000)) { $Result.cleanup = "terminated" }
                else {
                    Stop-Process -Id $EditorProcess.Id -Force -ErrorAction SilentlyContinue
                    $Result.cleanup = if ($EditorProcess.WaitForExit(10000)) { "terminated" } else { "failed" }
                }
            }
        }
        catch { $Result.cleanup = "failed" }
    }
    $ResultDirectory = Split-Path -Parent $ResultPath
    if ($ResultDirectory) { New-Item -ItemType Directory -Force -Path $ResultDirectory | Out-Null }
    $Result | ConvertTo-Json -Depth 5 | Set-Content -Path $ResultPath -Encoding UTF8
}

Write-Host "Low-spec startup probe finished: marker_observed=$($Result.startup_marker_observed) startup_seconds=$($Result.startup_seconds) peak_working_set_mib=$($Result.peak_working_set_mib) cleanup=$($Result.cleanup)"
exit 0
