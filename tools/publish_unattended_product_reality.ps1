[CmdletBinding()]
param(
    [string]$StateDir = "",
    [string]$Repository = "jweter/Project-Everward",
    [int]$IssueNumber = 243
)

$ErrorActionPreference = "Stop"

if (-not $StateDir) {
    if (-not $env:LOCALAPPDATA) {
        throw "LOCALAPPDATA is unavailable."
    }
    $StateDir = Join-Path $env:LOCALAPPDATA "Everward\unattended-worker"
}

$Latest = Join-Path $StateDir "latest.json"
$PublishState = Join-Path $StateDir "github-publication.json"
$BodyFile = Join-Path $StateDir "github-status-body.md"

function Write-PublishState {
    param([string]$Status, [string]$Detail, [string]$Key)

    [ordered]@{
        schema_version = 1
        status = $Status
        detail = $Detail
        publication_key = $Key
        updated_at_utc = (Get-Date).ToUniversalTime().ToString("o")
    } | ConvertTo-Json -Depth 5 | Set-Content -Path $PublishState -Encoding UTF8
}

function Get-FailureFingerprint {
    param([object]$Check)

    if ($null -eq $Check) { return "" }
    $Status = [string]$Check.status
    if ($Status -eq "REVIEW_REQUIRED") {
        $Reason = [string]$Check.reason
        if ($Reason -match "^([A-Za-z]+Error):") { return "worker_exception|type=$($Matches[1])" }
        return "review_required"
    }
    if ($Status -ne "FAIL") { return "" }
    $ExitCode = [string]$Check.exit_code
    $Tail = [string]$Check.failure_tail
    if ($Tail -match "(?i)timeout") { return "timeout|exit=$ExitCode" }
    if ($Tail -match "(?i)cmake") { return "cmake|exit=$ExitCode" }
    if ($Tail -match "(?i)ctest|test failed|tests failed") { return "test|exit=$ExitCode" }
    if ($Tail -match "(?i)build\.bat|unrealbuildtool|ubt") { return "unreal_build|exit=$ExitCode" }
    if ($Tail -match "(?i)traceback|error|failed") { return "command_failure|exit=$ExitCode" }
    return "exit=$ExitCode"
}

if (-not (Test-Path $Latest -PathType Leaf)) {
    Write-PublishState "LOCAL_ONLY" "No unattended result exists yet." ""
    exit 0
}

$Report = Get-Content -Raw -Path $Latest | ConvertFrom-Json
$Commit = [string]$Report.tested_commit
if (-not $Commit) {
    $Commit = [string]$Report.target_commit
}
$Result = [string]$Report.result
$Completed = [string]$Report.completed_at_utc
$Key = "$Commit|$Result|$Completed"

$Gh = Get-Command gh.exe -ErrorAction SilentlyContinue
if (-not $Gh) {
    $Gh = Get-Command gh -ErrorAction SilentlyContinue
}
if (-not $Gh) {
    Write-PublishState "LOCAL_ONLY" "GitHub CLI is not installed; evidence remains local." $Key
    exit 0
}

& $Gh.Source auth status *> $null
if ($LASTEXITCODE -ne 0) {
    Write-PublishState "LOCAL_ONLY" "GitHub CLI is not authenticated; evidence remains local." $Key
    exit 0
}

$ShortCommit = if ($Commit.Length -ge 12) { $Commit.Substring(0, 12) } else { $Commit }
$Lines = New-Object System.Collections.Generic.List[string]
$Lines.Add("# Everward unattended Windows worker - latest status")
$Lines.Add("")
$Lines.Add("This body is updated by the private Windows test worker. Detailed logs stay on the laptop; this issue receives only a compact sanitized summary.")
$Lines.Add("")
$Lines.Add("- **Result:** **$Result**")
$Lines.Add("- **Tested commit:** ``$ShortCommit``")
$Lines.Add("- **Completed:** $Completed")
$Lines.Add("- **Scope:** deterministic engineering/build checks available to the worker")
$Lines.Add("- **Gameplay/visual Product Reality claimed:** **No**")
$Lines.Add("")
$Lines.Add("## Checks")
$Lines.Add("")

$CheckNames = @(
    "dedicated_checkout",
    "editor_idle",
    "green_main_discovery",
    "cached_exact_commit",
    "checkout_sync",
    "full_preflight",
    "unreal_5_8",
    "unreal_editor_build",
    "unreal_headless_smoke",
    "worker_exception"
)

foreach ($Name in $CheckNames) {
    $Property = $Report.checks.PSObject.Properties[$Name]
    if ($null -eq $Property) { continue }
    $Check = $Property.Value
    $Status = [string]$Check.status
    $Lines.Add("- **${Name}:** $Status")
    $Fingerprint = Get-FailureFingerprint $Check
    if ($Fingerprint) {
        $Lines.Add("  - sanitized failure fingerprint: ``$Fingerprint``")
    }
}

$Lines.Add("")
$Lines.Add("## Human-only debt")
$Lines.Add("")
foreach ($Debt in @($Report.human_only_debt)) {
    if ($Debt) { $Lines.Add("- $Debt") }
}
$Lines.Add("")
$Lines.Add("Related implementation: #240. Local detailed evidence remains under `%LOCALAPPDATA%\\Everward\\unattended-worker\\`. Failure fingerprints contain only allow-listed categories and exit codes; raw local log text and paths are not published.")

$Lines -join "`r`n" | Set-Content -Path $BodyFile -Encoding UTF8

& $Gh.Source issue edit $IssueNumber --repo $Repository --body-file $BodyFile | Out-Null
if ($LASTEXITCODE -ne 0) {
    Write-PublishState "LOCAL_ONLY" "GitHub issue update failed; local evidence was preserved." $Key
    exit 0
}

Write-PublishState "PUBLISHED" "Updated $Repository issue #$IssueNumber with the latest sanitized status." $Key
exit 0
