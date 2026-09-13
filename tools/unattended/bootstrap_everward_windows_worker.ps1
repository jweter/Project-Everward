[CmdletBinding()]
param(
    [string]$RepositoryUrl = "https://github.com/jweter/Project-Everward.git",
    [string]$Branch = "main"
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

$LocalAppData = [Environment]::GetFolderPath("LocalApplicationData")
if (-not $LocalAppData) { throw "LOCALAPPDATA is unavailable." }

$StateRoot = Join-Path $LocalAppData "Everward\unattended-worker"
$CheckoutRoot = Join-Path $StateRoot "checkout"
$BootstrapLog = Join-Path $StateRoot "bootstrap.log"
New-Item -ItemType Directory -Force -Path $StateRoot | Out-Null

function Write-BootstrapLog([string]$Message) {
    $Line = "[$((Get-Date).ToUniversalTime().ToString('o'))] $Message"
    Add-Content -Path $BootstrapLog -Value $Line -Encoding UTF8
}

$Git = Get-Command git.exe -ErrorAction SilentlyContinue
if (-not $Git) { $Git = Get-Command git -ErrorAction SilentlyContinue }
if (-not $Git) {
    Write-BootstrapLog "Git unavailable; worker cannot sync."
    exit 2
}

try {
    if (-not (Test-Path (Join-Path $CheckoutRoot ".git"))) {
        if (Test-Path $CheckoutRoot) {
            Remove-Item -Recurse -Force $CheckoutRoot
        }
        Write-BootstrapLog "Creating dedicated worker checkout from $RepositoryUrl."
        & $Git.Source clone --no-tags --branch $Branch --single-branch $RepositoryUrl $CheckoutRoot *> $null
        if ($LASTEXITCODE -ne 0) { throw "git clone failed with exit code $LASTEXITCODE" }
    }
    else {
        Write-BootstrapLog "Refreshing dedicated worker checkout."
        & $Git.Source -C $CheckoutRoot fetch --prune origin $Branch *> $null
        if ($LASTEXITCODE -ne 0) { throw "git fetch failed with exit code $LASTEXITCODE" }
        & $Git.Source -C $CheckoutRoot checkout -f $Branch *> $null
        if ($LASTEXITCODE -ne 0) { throw "git checkout failed with exit code $LASTEXITCODE" }
        & $Git.Source -C $CheckoutRoot reset --hard "origin/$Branch" *> $null
        if ($LASTEXITCODE -ne 0) { throw "git reset failed with exit code $LASTEXITCODE" }
        # The checkout is dedicated to this worker. Remove non-ignored untracked
        # files while preserving ignored Unreal build caches for faster repeats.
        & $Git.Source -C $CheckoutRoot clean -fd *> $null
        if ($LASTEXITCODE -ne 0) { throw "git clean failed with exit code $LASTEXITCODE" }
    }

    $WorkerPath = Join-Path $CheckoutRoot "tools\unattended\run_everward_windows_worker.ps1"
    $PublisherPath = Join-Path $CheckoutRoot "tools\unattended\publish_everward_worker_result.ps1"
    if (-not (Test-Path $WorkerPath)) {
        throw "Synced main does not contain tools/unattended/run_everward_windows_worker.ps1."
    }

    Write-BootstrapLog "Running unattended worker against dedicated main checkout."
    & powershell.exe -NoProfile -ExecutionPolicy Bypass -File $WorkerPath -RepoRoot $CheckoutRoot
    $WorkerExitCode = $LASTEXITCODE

    if (Test-Path $PublisherPath) {
        Write-BootstrapLog "Publishing changed compact result when GitHub CLI is authenticated."
        & powershell.exe -NoProfile -ExecutionPolicy Bypass -File $PublisherPath
        # Publication is best-effort. It must never replace the worker's
        # deterministic exit code or turn a failed test into a PASS.
    }

    Write-BootstrapLog "Worker completed with exit code $WorkerExitCode."
    exit $WorkerExitCode
}
catch {
    Write-BootstrapLog "Bootstrap failure: $($_.Exception.Message)"
    exit 2
}
