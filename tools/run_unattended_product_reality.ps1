[CmdletBinding()]
param(
    [Parameter(Mandatory=$true)][string]$RepoRoot
)

$ErrorActionPreference = "Stop"

$Worker = Join-Path $RepoRoot "tools\everward_unattended_worker.py"
$Publisher = Join-Path $RepoRoot "tools\publish_unattended_product_reality.ps1"

if (-not (Test-Path $Worker -PathType Leaf)) {
    throw "Everward unattended worker is missing: $Worker"
}

$PythonCommand = Get-Command python -ErrorAction Stop
$Python = $PythonCommand.Source
if (-not $Python) {
    throw "Python was not found in PATH."
}

# The worker is authoritative for the deterministic result and intentionally
# exits zero under Task Scheduler to avoid retry storms. Test truth lives in
# %LOCALAPPDATA%\Everward\unattended-worker\latest.json.
& $Python $Worker --repo-root $RepoRoot
$WorkerExit = $LASTEXITCODE

# Repository reporting is best-effort and may never promote or demote the
# deterministic result. Missing GitHub CLI/auth leaves evidence local.
if (Test-Path $Publisher -PathType Leaf) {
    try {
        & powershell.exe -NoProfile -ExecutionPolicy Bypass -File $Publisher
    }
    catch {
        Write-Warning "Everward unattended result publishing failed: $($_.Exception.Message)"
    }
}

exit $WorkerExit
