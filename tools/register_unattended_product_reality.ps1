[CmdletBinding()]
param(
    [string]$RepoRoot = "",
    [switch]$ConfirmDedicatedCheckout
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
$Runner = Join-Path $RepoRoot "tools\run_unattended_product_reality.ps1"
$GitDir = Join-Path $RepoRoot ".git"
$Sentinel = Join-Path $GitDir "everward-unattended-worker"

if (-not (Test-Path $GitDir -PathType Container)) {
    throw "Everward unattended registration requires a Git checkout: $RepoRoot"
}
if (-not (Test-Path $Worker -PathType Leaf)) {
    throw "Everward unattended worker is missing: $Worker"
}
if (-not (Test-Path $Runner -PathType Leaf)) {
    throw "Everward unattended runner is missing: $Runner"
}

$Disabled = [Environment]::GetEnvironmentVariable($DisableVariable)
if ($Disabled -and $Disabled.Trim().ToLowerInvariant() -in @("1", "true", "yes", "on")) {
    & schtasks.exe /Delete /TN $TaskName /F 2>$null | Out-Null
    Write-Host "Everward unattended Product Reality task is disabled. Any existing task was removed."
    exit 0
}

# Never turn an arbitrary development clone into a destructive unattended
# checkout as a side effect of a normal playtest. The sentinel may only be
# created by an explicit one-time setup that passes -ConfirmDedicatedCheckout.
if (-not (Test-Path $Sentinel -PathType Leaf)) {
    if (-not $ConfirmDedicatedCheckout) {
        throw "This checkout is not authorized for unattended destructive sync. Run the dedicated one-time setup or re-run with -ConfirmDedicatedCheckout only for the disposable Everward Playtest checkout."
    }
    "registered_utc=$((Get-Date).ToUniversalTime().ToString('o'))" | Set-Content -Path $Sentinel -Encoding ASCII
}

# Revalidate origin before scheduling any destructive-capable worker.
$Origin = (& git -C $RepoRoot remote get-url origin).Trim()
if ($LASTEXITCODE -ne 0 -or -not $Origin) {
    throw "Could not resolve the checkout origin."
}
$NormalizedOrigin = $Origin.Replace("\", "/").TrimEnd("/").ToLowerInvariant()
if ($NormalizedOrigin.StartsWith("git@github.com:")) {
    $NormalizedOrigin = "https://github.com/" + $NormalizedOrigin.Substring("git@github.com:".Length)
}
$NormalizedOrigin = $NormalizedOrigin.TrimEnd(".git")
if ($NormalizedOrigin -ne "https://github.com/jweter/project-everward") {
    throw "Unexpected origin remote; refusing unattended registration: $Origin"
}

$PythonCommand = Get-Command python -ErrorAction Stop
$Python = $PythonCommand.Source
if (-not $Python) {
    throw "Python was not found in PATH."
}

# Run through the PowerShell wrapper so deterministic local evidence can be
# published to the dedicated GitHub status issue when gh is authenticated.
# Reporting is best-effort and never changes test truth.
$Action = "powershell.exe -NoProfile -ExecutionPolicy Bypass -File `"$Runner`" -RepoRoot `"$RepoRoot`""
& schtasks.exe /Create /TN $TaskName /TR $Action /SC ONIDLE /I 10 /RL LIMITED /F | Out-Null
if ($LASTEXITCODE -ne 0) {
    throw "Windows Task Scheduler could not register the Everward unattended worker (exit $LASTEXITCODE)."
}

Write-Host "Registered/refreshed Everward unattended Product Reality worker."
Write-Host "It will run after approximately 10 minutes of Windows idle time without launching Unreal Editor."
Write-Host "If GitHub CLI is authenticated, the latest sanitized status is also written to issue #243."
