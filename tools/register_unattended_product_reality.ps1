[CmdletBinding()]
param(
    [string]$RepoRoot = ""
)

$ErrorActionPreference = "Stop"
$TaskName = "Everward Unattended Product Reality"
$DisableVariable = "EVERWARD_DISABLE_UNATTENDED_WORKER"

if (-not $RepoRoot) {
    $RepoRoot = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
}
else {
    $RepoRoot = (Resolve-Path $RepoRoot).Path
}

$Worker = Join-Path $RepoRoot "tools\everward_unattended_worker.py"
$GitDir = Join-Path $RepoRoot ".git"
$Sentinel = Join-Path $GitDir "everward-unattended-worker"

if (-not (Test-Path $GitDir -PathType Container)) {
    throw "Everward unattended registration requires a Git checkout: $RepoRoot"
}
if (-not (Test-Path $Worker -PathType Leaf)) {
    throw "Everward unattended worker is missing: $Worker"
}

$Disabled = [Environment]::GetEnvironmentVariable($DisableVariable)
if ($Disabled -and $Disabled.Trim().ToLowerInvariant() -in @("1", "true", "yes", "on")) {
    & schtasks.exe /Delete /TN $TaskName /F 2>$null | Out-Null
    Write-Host "Everward unattended Product Reality task is disabled. Any existing task was removed."
    exit 0
}

$PythonCommand = Get-Command python -ErrorAction Stop
$Python = $PythonCommand.Source
if (-not $Python) {
    throw "Python was not found in PATH."
}

# This sentinel is deliberately stored inside .git so the worker can prove that
# destructive reset/clean operations are restricted to the dedicated playtest checkout.
"registered_utc=$((Get-Date).ToUniversalTime().ToString('o'))" | Set-Content -Path $Sentinel -Encoding ASCII

$Action = "`"$Python`" `"$Worker`" --repo-root `"$RepoRoot`""
& schtasks.exe /Create /TN $TaskName /TR $Action /SC ONIDLE /I 10 /RL LIMITED /F | Out-Null
if ($LASTEXITCODE -ne 0) {
    throw "Windows Task Scheduler could not register the Everward unattended worker (exit $LASTEXITCODE)."
}

Write-Host "Registered/refreshed Everward unattended Product Reality worker."
Write-Host "It will run after approximately 10 minutes of Windows idle time without launching Unreal Editor."
