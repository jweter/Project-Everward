$ErrorActionPreference = "Stop"

$PinnedCommit = "5914a8fd280b79d00fc6b0783c7e7d7b6affd654"
$Upstream = "https://github.com/arielshad/3d-asset-server.git"
$RepoRoot = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
$ToolsRoot = Join-Path $RepoRoot ".tools"
$InstallDir = Join-Path $ToolsRoot "3d-asset-server"

if (-not (Get-Command git -ErrorAction SilentlyContinue)) {
    throw "Git is required."
}
if (-not (Get-Command node -ErrorAction SilentlyContinue)) {
    throw "Node.js 20+ is required."
}
if (-not (Get-Command npm -ErrorAction SilentlyContinue)) {
    throw "npm is required."
}

$NodeMajor = [int]((node --version).TrimStart("v").Split(".")[0])
if ($NodeMajor -lt 20) {
    throw "Node.js 20+ is required; found $(node --version)."
}

New-Item -ItemType Directory -Force -Path $ToolsRoot | Out-Null

if (-not (Test-Path (Join-Path $InstallDir ".git"))) {
    git clone $Upstream $InstallDir
}

Push-Location $InstallDir
try {
    git fetch origin main
    git checkout --detach $PinnedCommit
    npm ci
    npm run build
}
finally {
    Pop-Location
}

Write-Host ""
Write-Host "Everward 3D asset server is pinned and built at:"
Write-Host "  $InstallDir"
Write-Host ""
Write-Host "Start it in a separate terminal with:"
Write-Host "  cd `"$InstallDir`""
Write-Host "  npm start"
Write-Host ""
Write-Host "Everward Asset Scout will then use http://127.0.0.1:8787 by default."
