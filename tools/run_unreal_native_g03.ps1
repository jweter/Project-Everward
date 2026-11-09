# Free native Unreal G03 Automation test runner; NEVER launches Aura.
# Build the exact candidate SHA via UnrealBuildTool before executing tests.
# Fail closed when native compilation or automation evidence is absent.
[CmdletBinding()]
param(
    [string]$UnrealRoot = "",
    [int]$TimeoutSeconds = 900,
    [int]$BuildTimeoutSeconds = 1800,
    [double]$MinimumFreeMemoryGiB = 10.0,
    [double]$OngoingMemoryFloorGiB = 7.0
)

$ErrorActionPreference = 'Stop'
$RepoRoot = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$Project = Join-Path $RepoRoot 'unreal\Everward.uproject'
if (-not (Test-Path $Project -PathType Leaf)) { throw 'Everward .uproject unavailable' }
$Origin = (& git -C $RepoRoot remote get-url origin).Trim()
if ($LASTEXITCODE -ne 0 -or $Origin -notmatch 'github\.com[:/]jweter/Project-Everward(\.git)?$') {
    throw 'G03 refuses an unrecognized repository origin'
}
$Head = (& git -C $RepoRoot rev-parse HEAD).Trim()
if ($LASTEXITCODE -ne 0 -or $Head -notmatch '^[a-f0-9]{40}$') { throw 'G03 cannot identify exact source commit' }
if ((& git -C $RepoRoot status --porcelain)) { throw 'G03 requires a clean exact-HEAD checkout' }

$Candidates = @($UnrealRoot, $env:UE58_ROOT, $env:UE_5_8_ROOT, 'C:\Program Files\Epic Games\UE_5.8') |
    Where-Object { $_ } | Select-Object -Unique
$EditorCmd = $null
$BuildBat = $null
foreach ($Base in $Candidates) {
    $Candidate = Join-Path $Base 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
    $CandidateBuild = Join-Path $Base 'Engine\Build\BatchFiles\Build.bat'
    if ((Test-Path $Candidate -PathType Leaf) -and (Test-Path $CandidateBuild -PathType Leaf)) {
        $EditorCmd = $Candidate
        $BuildBat = $CandidateBuild
        break
    }
}
if (-not $EditorCmd -or -not $BuildBat) { throw 'Unreal Editor 5.8 and UnrealBuildTool launcher not installed' }
if (Get-Process -Name UnrealEditor,UnrealEditor-Cmd,UnrealBuildTool -ErrorAction SilentlyContinue) {
    throw 'G03 native tests deferred: another Unreal/editor/build process is active'
}
$MemoryGiB = (Get-CimInstance Win32_OperatingSystem).FreePhysicalMemory / 1MB
if ($MemoryGiB -lt $MinimumFreeMemoryGiB) {
    throw "G03 deferred: insufficient free RAM ($([math]::Round($MemoryGiB, 2)) GiB); owner/supervisor memory floor preserved"
}
if ($TimeoutSeconds -lt 30 -or $TimeoutSeconds -gt 1800) { throw 'G03 timeout must be 30..1800 s' }
if ($BuildTimeoutSeconds -lt 60 -or $BuildTimeoutSeconds -gt 3600) { throw 'G03 build timeout must be 60..3600 s' }
if ($OngoingMemoryFloorGiB -lt 7.0 -or $MinimumFreeMemoryGiB -lt ($OngoingMemoryFloorGiB + 2.0)) {
    throw 'G03 requires at least 2 GiB preflight headroom above the protected 7 GiB floor'
}

$Stamp = (Get-Date).ToUniversalTime().ToString('yyyyMMdd-HHmmss-fff')
$Evidence = Join-Path $env:LOCALAPPDATA "Everward\native-g03\$Head\$Stamp"
$Report = Join-Path $Evidence 'report'
New-Item -ItemType Directory -Force -Path $Report | Out-Null
$Log = Join-Path $Evidence 'editor.log'
$Out = Join-Path $Evidence 'stdout.log'
$Err = Join-Path $Evidence 'stderr.log'

function Wait-Guarded([System.Diagnostics.Process]$Running, [int]$Seconds, [string]$Phase) {
    $Deadline = (Get-Date).AddSeconds($Seconds)
    while (-not $Running.WaitForExit(3000)) {
        $AvailableGiB = (Get-CimInstance Win32_OperatingSystem).FreePhysicalMemory / 1MB
        if ($AvailableGiB -lt $OngoingMemoryFloorGiB -or (Get-Date) -gt $Deadline) {
            & taskkill.exe /PID $Running.Id /T /F | Out-Null
            throw "G03 $Phase aborted (time or owner/supervisor memory floor). Evidence: $Evidence"
        }
    }
    if ($Running.ExitCode -ne 0) {
        throw "G03 $Phase exit code $($Running.ExitCode). Evidence: $Evidence"
    }
}

# A previous editor binary is never accepted as evidence for the current
# checkout. Rebuild this HEAD first, with one compiler action at a time.
$BuildOut = Join-Path $Evidence 'ubt-stdout.log'
$BuildErr = Join-Path $Evidence 'ubt-stderr.log'
$BuildLine = "`"`"$BuildBat`" EverwardEditor Win64 Development -Project=`"$Project`" -WaitMutex -NoHotReload -MaxParallelActions=1`""
$BuildProc = Start-Process -FilePath $env:COMSPEC -ArgumentList @('/d', '/c', $BuildLine) -PassThru -WindowStyle Hidden -RedirectStandardOutput $BuildOut -RedirectStandardError $BuildErr
Wait-Guarded $BuildProc $BuildTimeoutSeconds 'native UBT build'

$Arguments = @(
    "`"$Project`"", '-Unattended', '-NoSplash', '-NoSound', '-NullRHI', '-NoP4',
    '-NoAutoSDK', '-NoAssetRegistryCache', '-log', "-abslog=`"$Log`"",
    '-ExecCmds="Automation RunTests Everward.G03.Native"',
    '-TestExit="Automation Test Queue Empty"',
    "-ReportExportPath=`"$Report`""
)
$Process = Start-Process -FilePath $EditorCmd -ArgumentList $Arguments -PassThru -WindowStyle Hidden -RedirectStandardOutput $Out -RedirectStandardError $Err
Wait-Guarded $Process $TimeoutSeconds 'native Automation execution'
$Index = Join-Path $Report 'index.json'
if (-not (Test-Path $Index -PathType Leaf)) {
    throw "G03 missing real Unreal Automation report; no PASS can be inferred. Evidence: $Evidence"
}
$Raw = Get-Content $Index -Raw
$Parsed = $Raw | ConvertFrom-Json
if ($null -eq $Parsed.failed -or $null -eq $Parsed.succeeded) {
    throw "G03 Automation report missing numeric pass/fail totals. Evidence: $Evidence"
}
if ([int]$Parsed.failed -ne 0 -or [int]$Parsed.succeeded -lt 3 -or
    $Raw -notmatch 'Everward.G03.Native.ScanSurveyPersistsAfterCancel' -or
    $Raw -notmatch 'Everward.G03.Native.MiningRejectsWithoutSurveyAndTool' -or
    $Raw -notmatch 'Everward.G03.Native.ArmToggleReversesAndJointControlsStayHonest') {
    throw "G03 expected native tests did not both pass. Evidence: $Evidence"
}
Write-Output "UNREAL_VERIFIED (native G03 subset only): 3 Everward game-world automation tests passed on exact source $Head"
Write-Output "Evidence: $Evidence"
Write-Output 'This does NOT certify G01/G02 mining success, real key-input path, HUD, Fix_It, or paid Aura milestone acceptance.'
