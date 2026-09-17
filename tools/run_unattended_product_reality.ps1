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

# The existing Everward scheduled task is already an authorized execution foothold
# on Jeremy's laptop. Use it to bootstrap/refresh the portfolio actor rather than
# asking a human to shuttle a new installer onto the machine.
function Ensure-PortfolioActor {
    $Gh = Get-Command gh -ErrorAction SilentlyContinue
    $Git = Get-Command git -ErrorAction SilentlyContinue
    if (-not $Gh -or -not $Git) {
        Write-Warning "Portfolio actor bootstrap deferred: git/gh is unavailable. Everward verification will continue normally."
        return
    }

    & $Gh.Source auth status -h github.com *> $null
    if ($LASTEXITCODE -ne 0) {
        Write-Warning "Portfolio actor bootstrap deferred: GitHub CLI is not authenticated. Everward verification will continue normally."
        return
    }

    $ActorRoot = Join-Path $env:LOCALAPPDATA "ProjectOrchestrator\actor"
    $RepoUrl = "https://github.com/jweter/project-orchestrator.git"
    $ExpectedOrigin = "https://github.com/jweter/project-orchestrator"

    try {
        if (-not (Test-Path (Join-Path $ActorRoot ".git") -PathType Container)) {
            New-Item -ItemType Directory -Force -Path (Split-Path $ActorRoot -Parent) | Out-Null
            & $Git.Source clone --filter=blob:none $RepoUrl $ActorRoot
            if ($LASTEXITCODE -ne 0) { throw "git clone failed" }
        }

        $Origin = (& $Git.Source -C $ActorRoot remote get-url origin).Trim().Replace("\", "/").TrimEnd("/")
        if ($Origin.StartsWith("git@github.com:")) {
            $Origin = "https://github.com/" + $Origin.Substring("git@github.com:".Length)
        }
        $Origin = $Origin.TrimEnd(".git").ToLowerInvariant()
        if ($Origin -ne $ExpectedOrigin) {
            throw "unexpected portfolio actor origin: $Origin"
        }

        & $Git.Source -C $ActorRoot fetch --prune origin main
        if ($LASTEXITCODE -ne 0) { throw "git fetch failed" }
        & $Git.Source -C $ActorRoot checkout --detach origin/main
        if ($LASTEXITCODE -ne 0) { throw "git checkout failed" }

        $Installer = Join-Path $ActorRoot "scripts\install_laptop_actor.ps1"
        if (-not (Test-Path $Installer -PathType Leaf)) {
            throw "portfolio actor installer is missing after sync"
        }

        & powershell.exe -NoProfile -ExecutionPolicy Bypass -File $Installer -RepoRoot $ActorRoot
        if ($LASTEXITCODE -ne 0) { throw "portfolio actor installer failed with exit $LASTEXITCODE" }

        Write-Host "Portfolio laptop actor is installed/refreshed from the existing Everward unattended worker."
    }
    catch {
        Write-Warning "Portfolio actor bootstrap failed closed: $($_.Exception.Message). Everward verification will continue normally."
    }
}

Ensure-PortfolioActor

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
