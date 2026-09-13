[CmdletBinding()]
param(
    [string]$RepoRoot = "",
    [string]$UnrealRoot = "",
    [int]$StaleLockHours = 6
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

function Resolve-RepoRoot {
    param([string]$ExplicitRoot)
    if ($ExplicitRoot) {
        return (Resolve-Path $ExplicitRoot).Path
    }
    return (Resolve-Path (Join-Path $PSScriptRoot "..\..")).Path
}

function Resolve-Unreal58Root {
    param([string]$ExplicitRoot)

    $Candidates = New-Object System.Collections.Generic.List[string]
    if ($ExplicitRoot) { $Candidates.Add($ExplicitRoot) }

    foreach ($Name in @("UE58_ROOT", "UE_5_8_ROOT", "UE_5_8")) {
        $Value = [Environment]::GetEnvironmentVariable($Name)
        if ($Value) { $Candidates.Add($Value) }
    }

    foreach ($RegistryPath in @(
        "HKLM:\SOFTWARE\EpicGames\Unreal Engine\5.8",
        "HKCU:\SOFTWARE\EpicGames\Unreal Engine\5.8",
        "HKLM:\SOFTWARE\WOW6432Node\EpicGames\Unreal Engine\5.8"
    )) {
        try {
            $InstalledDirectory = (Get-ItemProperty -Path $RegistryPath -Name InstalledDirectory -ErrorAction Stop).InstalledDirectory
            if ($InstalledDirectory) { $Candidates.Add($InstalledDirectory) }
        }
        catch {
            # Optional registry entry; continue to the next discovery method.
        }
    }

    foreach ($CommonPath in @(
        "C:\Program Files\Epic Games\UE_5.8",
        "C:\Epic Games\UE_5.8"
    )) {
        $Candidates.Add($CommonPath)
    }

    foreach ($Candidate in $Candidates | Select-Object -Unique) {
        $BuildBat = Join-Path $Candidate "Engine\Build\BatchFiles\Build.bat"
        if (Test-Path $BuildBat) {
            return (Resolve-Path $Candidate).Path
        }
    }

    return $null
}

function Resolve-Python {
    foreach ($Name in @("python.exe", "python")) {
        $Command = Get-Command $Name -ErrorAction SilentlyContinue
        if ($Command) { return $Command.Source }
    }
    return $null
}

function Get-SanitizedTail {
    param(
        [string]$Path,
        [string]$RepoPath,
        [string]$StatePath,
        [int]$Lines = 80
    )

    if (-not (Test-Path $Path)) { return @() }

    $Tail = Get-Content -Path $Path -Tail $Lines -ErrorAction SilentlyContinue
    $UserProfile = [Environment]::GetFolderPath("UserProfile")
    $Result = @()
    foreach ($Line in $Tail) {
        $Sanitized = [string]$Line
        if ($RepoPath) { $Sanitized = $Sanitized.Replace($RepoPath, "<REPO>") }
        if ($StatePath) { $Sanitized = $Sanitized.Replace($StatePath, "<WORKER_STATE>") }
        if ($UserProfile) { $Sanitized = $Sanitized.Replace($UserProfile, "<USERPROFILE>") }
        $Result += $Sanitized
    }
    return $Result
}

function Write-Evidence {
    param(
        [hashtable]$Evidence,
        [string]$RunEvidencePath,
        [string]$LatestEvidencePath
    )

    $Json = $Evidence | ConvertTo-Json -Depth 12
    $Json | Set-Content -Path $RunEvidencePath -Encoding UTF8
    $Json | Set-Content -Path $LatestEvidencePath -Encoding UTF8
}

$ResolvedRepoRoot = Resolve-RepoRoot -ExplicitRoot $RepoRoot
$ProjectPath = Join-Path $ResolvedRepoRoot "unreal\Everward.uproject"
$PreflightPath = Join-Path $ResolvedRepoRoot "tools\quality_preflight.py"

$LocalAppData = [Environment]::GetFolderPath("LocalApplicationData")
if (-not $LocalAppData) {
    throw "LOCALAPPDATA is unavailable; cannot create private unattended-worker state."
}

$StateRoot = Join-Path $LocalAppData "Everward\unattended-worker"
$RunsRoot = Join-Path $StateRoot "runs"
$LogsRoot = Join-Path $StateRoot "logs"
New-Item -ItemType Directory -Force -Path $RunsRoot, $LogsRoot | Out-Null

$LockPath = Join-Path $StateRoot "worker.lock"
if (Test-Path $LockPath) {
    $LockAge = (Get-Date) - (Get-Item $LockPath).LastWriteTime
    if ($LockAge.TotalHours -ge $StaleLockHours) {
        Remove-Item -Force $LockPath
    }
    else {
        Write-Host "Everward unattended worker skipped: another run is active (lock age $([math]::Round($LockAge.TotalMinutes, 1)) minutes)."
        exit 0
    }
}

$LockStream = $null
try {
    $LockStream = [System.IO.File]::Open(
        $LockPath,
        [System.IO.FileMode]::CreateNew,
        [System.IO.FileAccess]::Write,
        [System.IO.FileShare]::None)
    $LockWriter = New-Object System.IO.StreamWriter($LockStream)
    $LockWriter.WriteLine("pid=$PID")
    $LockWriter.WriteLine("started_utc=$((Get-Date).ToUniversalTime().ToString('o'))")
    $LockWriter.Flush()

    $StartedUtc = (Get-Date).ToUniversalTime()
    $RunId = $StartedUtc.ToString("yyyyMMdd-HHmmss")
    $RunEvidencePath = Join-Path $RunsRoot "$RunId.json"
    $LatestEvidencePath = Join-Path $StateRoot "latest.json"
    $PreflightLog = Join-Path $LogsRoot "$RunId-preflight.log"
    $BuildLog = Join-Path $LogsRoot "$RunId-unreal-build.log"

    $Evidence = [ordered]@{
        schema_version = 1
        worker = "everward-windows-unattended"
        run_id = $RunId
        started_utc = $StartedUtc.ToString("o")
        ended_utc = $null
        result = "REVIEW_REQUIRED"
        scope = "engineering_build_only"
        product_reality_claimed = $false
        repo = [ordered]@{
            path = "<REPO>"
            commit = $null
            clean = $false
        }
        host = [ordered]@{
            cpu = ""
            gpu = ""
            computer_name = $env:COMPUTERNAME
        }
        checks = [ordered]@{
            repository = [ordered]@{ status = "NOT_RUN"; exit_code = $null; duration_seconds = 0.0; detail = "" }
            full_preflight = [ordered]@{ status = "NOT_RUN"; exit_code = $null; duration_seconds = 0.0; detail = ""; log_tail = @() }
            unreal_58 = [ordered]@{ status = "NOT_RUN"; exit_code = $null; duration_seconds = 0.0; detail = "" }
            unreal_cpp_build = [ordered]@{ status = "NOT_RUN"; exit_code = $null; duration_seconds = 0.0; detail = ""; log_tail = @() }
        }
    }

    try { $Evidence.host.cpu = (Get-CimInstance Win32_Processor | Select-Object -First 1 -ExpandProperty Name).Trim() } catch {}
    try { $Evidence.host.gpu = (Get-CimInstance Win32_VideoController | Select-Object -First 1 -ExpandProperty Name).Trim() } catch {}

    if (-not (Test-Path $ProjectPath)) {
        $Evidence.checks.repository.status = "REVIEW_REQUIRED"
        $Evidence.checks.repository.detail = "Everward.uproject not found."
        $Evidence.ended_utc = (Get-Date).ToUniversalTime().ToString("o")
        Write-Evidence $Evidence $RunEvidencePath $LatestEvidencePath
        exit 2
    }

    $Git = Get-Command git.exe -ErrorAction SilentlyContinue
    if (-not $Git) { $Git = Get-Command git -ErrorAction SilentlyContinue }
    if (-not $Git) {
        $Evidence.checks.repository.status = "REVIEW_REQUIRED"
        $Evidence.checks.repository.detail = "Git executable not found."
        $Evidence.ended_utc = (Get-Date).ToUniversalTime().ToString("o")
        Write-Evidence $Evidence $RunEvidencePath $LatestEvidencePath
        exit 2
    }

    $Commit = (& $Git.Source -C $ResolvedRepoRoot rev-parse HEAD).Trim()
    $GitStatus = @(& $Git.Source -C $ResolvedRepoRoot status --porcelain)
    $Evidence.repo.commit = $Commit
    $Evidence.repo.clean = ($GitStatus.Count -eq 0)

    if (-not $Commit -or $LASTEXITCODE -ne 0) {
        $Evidence.checks.repository.status = "REVIEW_REQUIRED"
        $Evidence.checks.repository.detail = "Unable to resolve exact Git commit."
        $Evidence.ended_utc = (Get-Date).ToUniversalTime().ToString("o")
        Write-Evidence $Evidence $RunEvidencePath $LatestEvidencePath
        exit 2
    }

    if (-not $Evidence.repo.clean) {
        $Evidence.checks.repository.status = "REVIEW_REQUIRED"
        $Evidence.checks.repository.detail = "Working tree is dirty; exact-commit evidence would be ambiguous."
        $Evidence.ended_utc = (Get-Date).ToUniversalTime().ToString("o")
        Write-Evidence $Evidence $RunEvidencePath $LatestEvidencePath
        exit 2
    }

    $Evidence.checks.repository.status = "PASS"
    $Evidence.checks.repository.detail = "Exact clean Git commit recorded."

    $Python = Resolve-Python
    if (-not $Python -or -not (Test-Path $PreflightPath)) {
        $Evidence.checks.full_preflight.status = "REVIEW_REQUIRED"
        $Evidence.checks.full_preflight.detail = "Python or tools/quality_preflight.py is unavailable."
        $Evidence.ended_utc = (Get-Date).ToUniversalTime().ToString("o")
        Write-Evidence $Evidence $RunEvidencePath $LatestEvidencePath
        exit 2
    }

    $PreflightStart = Get-Date
    & $Python $PreflightPath --full *> $PreflightLog
    $PreflightExitCode = $LASTEXITCODE
    $Evidence.checks.full_preflight.duration_seconds = [math]::Round(((Get-Date) - $PreflightStart).TotalSeconds, 3)
    $Evidence.checks.full_preflight.exit_code = $PreflightExitCode
    $Evidence.checks.full_preflight.log_tail = @(Get-SanitizedTail -Path $PreflightLog -RepoPath $ResolvedRepoRoot -StatePath $StateRoot)

    if ($PreflightExitCode -ne 0) {
        $Evidence.checks.full_preflight.status = "FAIL"
        $Evidence.checks.full_preflight.detail = "Canonical full preflight failed."
        $Evidence.result = "FAIL"
        $Evidence.ended_utc = (Get-Date).ToUniversalTime().ToString("o")
        Write-Evidence $Evidence $RunEvidencePath $LatestEvidencePath
        exit 1
    }
    $Evidence.checks.full_preflight.status = "PASS"
    $Evidence.checks.full_preflight.detail = "python tools/quality_preflight.py --full passed."

    $ResolvedUnrealRoot = Resolve-Unreal58Root -ExplicitRoot $UnrealRoot
    if (-not $ResolvedUnrealRoot) {
        $Evidence.checks.unreal_58.status = "REVIEW_REQUIRED"
        $Evidence.checks.unreal_58.detail = "Unreal Engine 5.8 was not found."
        $Evidence.ended_utc = (Get-Date).ToUniversalTime().ToString("o")
        Write-Evidence $Evidence $RunEvidencePath $LatestEvidencePath
        exit 2
    }

    $Evidence.checks.unreal_58.status = "PASS"
    $Evidence.checks.unreal_58.detail = "Unreal Engine 5.8 discovered."
    $BuildBat = Join-Path $ResolvedUnrealRoot "Engine\Build\BatchFiles\Build.bat"

    $BuildStart = Get-Date
    & $BuildBat EverwardEditor Win64 Development $ProjectPath -WaitMutex -NoHotReloadFromIDE *> $BuildLog
    $BuildExitCode = $LASTEXITCODE
    $Evidence.checks.unreal_cpp_build.duration_seconds = [math]::Round(((Get-Date) - $BuildStart).TotalSeconds, 3)
    $Evidence.checks.unreal_cpp_build.exit_code = $BuildExitCode
    $Evidence.checks.unreal_cpp_build.log_tail = @(Get-SanitizedTail -Path $BuildLog -RepoPath $ResolvedRepoRoot -StatePath $StateRoot)

    if ($BuildExitCode -ne 0) {
        $Evidence.checks.unreal_cpp_build.status = "FAIL"
        $Evidence.checks.unreal_cpp_build.detail = "EverwardEditor Win64 Development failed."
        $Evidence.result = "FAIL"
        $Evidence.ended_utc = (Get-Date).ToUniversalTime().ToString("o")
        Write-Evidence $Evidence $RunEvidencePath $LatestEvidencePath
        exit 1
    }

    $Evidence.checks.unreal_cpp_build.status = "PASS"
    $Evidence.checks.unreal_cpp_build.detail = "EverwardEditor Win64 Development built successfully."
    $Evidence.result = "PASS"
    $Evidence.ended_utc = (Get-Date).ToUniversalTime().ToString("o")
    Write-Evidence $Evidence $RunEvidencePath $LatestEvidencePath

    Write-Host "Everward unattended verification PASS for commit $Commit"
    Write-Host "Evidence: $RunEvidencePath"
    exit 0
}
catch {
    try {
        if ($null -ne $Evidence -and $null -ne $RunEvidencePath) {
            $Evidence.result = "REVIEW_REQUIRED"
            $Evidence.ended_utc = (Get-Date).ToUniversalTime().ToString("o")
            $Evidence.unhandled_error = $_.Exception.Message
            Write-Evidence $Evidence $RunEvidencePath $LatestEvidencePath
        }
    }
    catch {}
    Write-Error $_
    exit 2
}
finally {
    if ($null -ne $LockStream) {
        $LockStream.Dispose()
    }
    if (Test-Path $LockPath) {
        Remove-Item -Force $LockPath -ErrorAction SilentlyContinue
    }
}
