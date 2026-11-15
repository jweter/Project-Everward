$ErrorActionPreference = 'Stop'
$project = Split-Path -Parent $MyInvocation.MyCommand.Path
$root = Split-Path -Parent $project
$godot = Join-Path $root 'tools\godot-4.7.2\Godot_v4.7.2-stable_win64_console.exe'
$evidence = Join-Path $project 'evidence'
if (!(Test-Path $godot)) { throw 'GODOT_NOT_FOUND' }
if (!(Test-Path (Join-Path $project 'project.godot'))) { throw 'PROJECT_NOT_FOUND' }

$rows = @()
foreach ($trial in 2,3) {
    $stdout = Join-Path $evidence ('trial_' + $trial + '_stdout.log')
    $stderr = Join-Path $evidence ('trial_' + $trial + '_stderr.log')
    $arguments = '--path "' + $project + '" --quit-after 400 -- --benchmark'
    $startFreeMb = [math]::Round((Get-CimInstance Win32_OperatingSystem).FreePhysicalMemory / 1024,1)
    $sw = [System.Diagnostics.Stopwatch]::StartNew()
    $proc = Start-Process -FilePath $godot -ArgumentList $arguments -PassThru -RedirectStandardOutput $stdout -RedirectStandardError $stderr
    $peakMb = 0.0
    while (!$proc.HasExited) {
        foreach ($running in @(Get-Process -Name 'Godot_v4.7.2-stable_win64*' -ErrorAction SilentlyContinue)) {
            $mb = [math]::Round($running.WorkingSet64 / 1MB,1)
            if ($mb -gt $peakMb) { $peakMb = $mb }
        }
        Start-Sleep -Milliseconds 400
    }
    $sw.Stop()
    $proc.Refresh()
    $metricsPath = Join-Path $evidence 'godot_render_smoke.json'
    if (!(Test-Path $metricsPath)) { throw ('MISSING_METRICS_TRIAL_' + $trial) }
    $metrics = Get-Content $metricsPath -Raw | ConvertFrom-Json
    Copy-Item $metricsPath (Join-Path $evidence ('trial_' + $trial + '_render_smoke.json')) -Force
    $row = [pscustomobject]@{
        trial = $trial
        exit_code = $proc.ExitCode
        wall_seconds = [math]::Round($sw.Elapsed.TotalSeconds,2)
        mean_fps_est = [math]::Round($metrics.mean_frame_based_fps,2)
        p95_frame_ms = [math]::Round($metrics.p95_frame_ms,2)
        p99_frame_ms = [math]::Round($metrics.p99_frame_ms,2)
        observed_peak_godot_process_ws_mb = $peakMb
        free_system_ram_before_mb = $startFreeMb
        sample_count = $metrics.frames
        output_log = $stdout
    }
    $rows += $row
    Write-Output ($row | ConvertTo-Json -Compress)
}
$rows | ConvertTo-Json | Set-Content (Join-Path $evidence 'repeat_trials.json') -Encoding UTF8
