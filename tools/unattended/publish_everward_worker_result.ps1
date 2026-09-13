[CmdletBinding()]
param(
    [string]$EvidencePath = "",
    [string]$Repository = "jweter/Project-Everward",
    [int]$IssueNumber = 240
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

$LocalAppData = [Environment]::GetFolderPath("LocalApplicationData")
if (-not $LocalAppData) { throw "LOCALAPPDATA is unavailable." }
$StateRoot = Join-Path $LocalAppData "Everward\unattended-worker"
if (-not $EvidencePath) { $EvidencePath = Join-Path $StateRoot "latest.json" }
$PublishStatePath = Join-Path $StateRoot "publication_status.json"
$LastKeyPath = Join-Path $StateRoot "last_published_key.txt"
$BodyPath = Join-Path $StateRoot "github-comment.md"

function Write-PublishState {
    param([string]$Status, [string]$Detail, [string]$Key)
    [ordered]@{
        schema_version = 1
        status = $Status
        detail = $Detail
        publication_key = $Key
        updated_utc = (Get-Date).ToUniversalTime().ToString("o")
    } | ConvertTo-Json -Depth 5 | Set-Content -Path $PublishStatePath -Encoding UTF8
}

if (-not (Test-Path $EvidencePath)) {
    Write-PublishState "LOCAL_ONLY" "No worker evidence exists yet." ""
    exit 0
}

$Evidence = Get-Content -Raw -Path $EvidencePath | ConvertFrom-Json
$Commit = [string]$Evidence.repo.commit
$Result = [string]$Evidence.result
$Preflight = [string]$Evidence.checks.full_preflight.status
$Build = [string]$Evidence.checks.unreal_cpp_build.status
$PublicationKey = "$Commit|$Result|$Preflight|$Build"

if (Test-Path $LastKeyPath) {
    $PreviousKey = (Get-Content -Raw -Path $LastKeyPath).Trim()
    if ($PreviousKey -eq $PublicationKey) {
        Write-PublishState "UNCHANGED" "This exact commit/result combination was already published." $PublicationKey
        exit 0
    }
}

$Gh = Get-Command gh.exe -ErrorAction SilentlyContinue
if (-not $Gh) { $Gh = Get-Command gh -ErrorAction SilentlyContinue }
if (-not $Gh) {
    Write-PublishState "LOCAL_ONLY" "GitHub CLI is not installed; evidence remains local." $PublicationKey
    exit 0
}

& $Gh.Source auth status *> $null
if ($LASTEXITCODE -ne 0) {
    Write-PublishState "LOCAL_ONLY" "GitHub CLI is present but not authenticated; evidence remains local." $PublicationKey
    exit 0
}

$ShortCommit = if ($Commit.Length -ge 12) { $Commit.Substring(0, 12) } else { $Commit }
$DurationPreflight = $Evidence.checks.full_preflight.duration_seconds
$DurationBuild = $Evidence.checks.unreal_cpp_build.duration_seconds

$Lines = @(
    "### Everward unattended Windows verification",
    "",
    "- **Commit:** ``$ShortCommit``",
    "- **Result:** **$Result**",
    "- **Canonical full preflight:** $Preflight ($DurationPreflight s)",
    "- **Unreal 5.8 C++ build:** $Build ($DurationBuild s)",
    "- **Scope:** engineering/build verification only; no visual/gameplay Product Reality claim",
    "- **Run:** $($Evidence.started_utc)",
    ""
)

if ($Result -ne "PASS") {
    $FailureDetails = @()
    if ($Preflight -ne "PASS") { $FailureDetails += "Preflight: $($Evidence.checks.full_preflight.detail)" }
    if ($Build -ne "PASS") { $FailureDetails += "Unreal build: $($Evidence.checks.unreal_cpp_build.detail)" }
    if ($FailureDetails.Count -gt 0) {
        $Lines += "**Machine-verifiable failure/review details:**"
        foreach ($Detail in $FailureDetails) { $Lines += "- $Detail" }
        $Lines += ""
    }
}

$Lines += "Local detailed evidence/logs remain on the test machine under `%LOCALAPPDATA%\\Everward\\unattended-worker\\`."
$Lines -join "`r`n" | Set-Content -Path $BodyPath -Encoding UTF8

& $Gh.Source issue comment $IssueNumber --repo $Repository --body-file $BodyPath | Out-Null
if ($LASTEXITCODE -ne 0) {
    Write-PublishState "LOCAL_ONLY" "GitHub publication failed; local evidence was preserved." $PublicationKey
    exit 0
}

$PublicationKey | Set-Content -Path $LastKeyPath -Encoding ASCII
Write-PublishState "PUBLISHED" "Published compact sanitized result to GitHub issue #$IssueNumber." $PublicationKey
Write-Host "Everward unattended result published to $Repository issue #$IssueNumber."
