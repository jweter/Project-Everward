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

$BootstrapSource = Join-Path $ResolvedRepoRoot "tools\unattended\bootstrap_everward_windows_worker.ps1"
if (-not (Test-Path $BootstrapSource)) {
    throw "Everward unattended bootstrap not found at $BootstrapSource"
}

$LocalAppData = [Environment]::GetFolderPath("LocalApplicationData")
if (-not $LocalAppData) {
    throw "LOCALAPPDATA is unavailable; cannot record unattended-worker registration."
}

$StateRoot = Join-Path $LocalAppData "Everward\unattended-worker"
New-Item -ItemType Directory -Force -Path $StateRoot | Out-Null
$RegistrationPath = Join-Path $StateRoot "registration.json"
$BootstrapInstalled = Join-Path $StateRoot "worker-bootstrap.ps1"

# Copy only the small stable bootstrap outside the working repository. The
# bootstrap maintains a dedicated clean checkout of main and invokes the
# current worker implementation from there, so future merged worker changes
# do not require Jeremy to update his normal development checkout.
Copy-Item -Force -Path $BootstrapSource -Destination $BootstrapInstalled

$TaskCommand = "powershell.exe -NoProfile -ExecutionPolicy Bypass -File `"$BootstrapInstalled`""

# ONIDLE is intentionally current-user and non-interactive. Do not add /RU SYSTEM:
# the worker must retain the same Unreal/Git/Python/GitHub CLI access as the
# signed-in user and must not require elevation merely to refresh registration.
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
    source_repo_root = "<REPO>"
    installed_bootstrap = "%LOCALAPPDATA%\\Everward\\unattended-worker\\worker-bootstrap.ps1"
    dedicated_checkout = "%LOCALAPPDATA%\\Everward\\unattended-worker\\checkout"
    branch = "main"
    github_report_issue = "jweter/Project-Everward#240"
    registered_utc = (Get-Date).ToUniversalTime().ToString("o")
    elevation_required = $false
}
$Registration | ConvertTo-Json -Depth 6 | Set-Content -Path $RegistrationPath -Encoding UTF8

Write-Host "Everward unattended worker registered."
Write-Host "  Task:       $TaskName"
Write-Host "  Trigger:    after $IdleMinutes minutes idle"
Write-Host "  Test code:  dedicated synced checkout of main"
Write-Host "  Evidence:   $StateRoot"
Write-Host "  Reporting:  GitHub issue #240 when gh is authenticated; otherwise local evidence is preserved"
