[CmdletBinding()]
param(
    [string]$RepoRoot = "",
    [int]$IdleMinutes = 10,
    [string]$TaskName = "Everward Unattended Worker"
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

if ($IdleMinutes -lt 1) {
    throw "IdleMinutes must be at least 1."
}

if ($RepoRoot) {
    $ResolvedRepoRoot = (Resolve-Path $RepoRoot).Path
}
else {
    $ResolvedRepoRoot = (Resolve-Path (Join-Path $PSScriptRoot "..\..")).Path
}

$WorkerPath = Join-Path $ResolvedRepoRoot "tools\unattended\run_everward_windows_worker.ps1"
if (-not (Test-Path $WorkerPath)) {
    throw "Everward unattended worker not found at $WorkerPath"
}

$LocalAppData = [Environment]::GetFolderPath("LocalApplicationData")
if (-not $LocalAppData) {
    throw "LOCALAPPDATA is unavailable; cannot record unattended-worker registration."
}

$StateRoot = Join-Path $LocalAppData "Everward\unattended-worker"
New-Item -ItemType Directory -Force -Path $StateRoot | Out-Null
$RegistrationPath = Join-Path $StateRoot "registration.json"

$TaskCommand = "powershell.exe -NoProfile -ExecutionPolicy Bypass -File `"$WorkerPath`" -RepoRoot `"$ResolvedRepoRoot`""

# ONIDLE is intentionally current-user and non-interactive. Do not add /RU SYSTEM:
# the worker must retain the same repository/Unreal access as the signed-in user
# and must not require elevation merely to refresh its registration.
& schtasks.exe /Create /TN $TaskName /TR $TaskCommand /SC ONIDLE /I $IdleMinutes /F | Out-Host
$TaskExitCode = $LASTEXITCODE
if ($TaskExitCode -ne 0) {
    throw "schtasks.exe failed to register '$TaskName' (exit code $TaskExitCode)."
}

$Registration = [ordered]@{
    schema_version = 1
    task_name = $TaskName
    trigger = "ONIDLE"
    idle_minutes = $IdleMinutes
    repo_root = "<REPO>"
    worker_script = "tools/unattended/run_everward_windows_worker.ps1"
    registered_utc = (Get-Date).ToUniversalTime().ToString("o")
    elevation_required = $false
}
$Registration | ConvertTo-Json -Depth 6 | Set-Content -Path $RegistrationPath -Encoding UTF8

Write-Host "Everward unattended worker registered."
Write-Host "  Task:     $TaskName"
Write-Host "  Trigger:  after $IdleMinutes minutes idle"
Write-Host "  Evidence: $StateRoot"
