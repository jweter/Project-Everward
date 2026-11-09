# Free native Unreal G03 Automation test runner; NEVER launches Aura.
# A passing UBT compile is required before this script runs against the exact
# candidate SHA. Fail closed when automation report or expected tests are absent.
[CmdletBinding()]
param(
    [string]$UnrealRoot = "",
    [int]$TimeoutSeconds = 900,
    [double]$MinimumFreeMemoryGiB = 10.0
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
foreach ($Base in $Candidates) {
    $Candidate = Join-Path $Base 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
    if (Test-Path $Candidate -PathType Leaf) { $EditorCmd = $Candidate; break }
}
if (-not $EditorCmd) { throw 'Unreal Editor 5.8 commandlet not installed' }
if (Get-Process -Name UnrealEditor,UnrealEditor-Cmd,UnrealBuildTool -ErrorAction SilentlyContinue) {
    throw 'G03 native tests deferred: another Unreal/editor/build process is active'
}
$MemoryGiB = (Get-CimInstance Win32_OperatingSystem).FreePhysicalMemory / 1MB
if ($MemoryGiB -lt $MinimumFreeMemoryGiB) {
    throw "G03 deferred: insufficient free RAM ($([math]::Round($MemoryGiB, 2)) GiB); owner/supervisor memory floor preserved"
}
if ($TimeoutSeconds -lt 30 -or $TimeoutSeconds -gt 1800) { throw 'G03 timeout must be 30..1800 s' }

$Stamp = (Get-Date).ToUniversalTime().ToString('yyyyMMdd-HHmmss-fff')
$Evidence = Join-Path $env:LOCALAPPDATA "Everward\native-g03\$Head\$Stamp"
$Report = Join-Path $Evidence 'report'
New-Item -ItemType Directory -Force -Path $Report | Out-Null
$Log = Join-Path $Evidence 'editor.log'
$Out = Join-Path $Evidence 'stdout.log'
$Err = Join-Path $Evidence 'stderr.log'
$Arguments = @(
    "`"$Project`"", '-Unattended', '-NoSplash', '-NoSound', '-NullRHI', '-NoP4',
    '-NoAutoSDK', '-NoAssetRegistryCache', '-log', "-abslog=`"$Log`"",
    '-ExecCmds="Automation RunTests Everward.G03.Native"',
    '-TestExit="Automation Test Queue Empty"',
    "-ReportExportPath=`"$Report`""
)
$Process = Start-Process -FilePath $EditorCmd -ArgumentList $Arguments -PassThru -WindowStyle Hidden -RedirectStandardOutput $Out -RedirectStandardError $Err
if (-not $Process.WaitForExit($TimeoutSeconds * 1000)) {
    & taskkill.exe /PID $Process.Id /T /F | Out-Null
    throw "G03 native Unreal automation timed out. Evidence: $Evidence"
}
if ($Process.ExitCode -ne 0) {
    throw "G03 Unreal Editor returned $($Process.ExitCode). Evidence: $Evidence"
}
$Index = Join-Path $Report 'index.json'
if (-not (Test-Path $Index -PathType Leaf)) {
    throw "G03 missing real Unreal Automation report; no PASS can be inferred. Evidence: $Evidence"
}
$Raw = Get-Content $Index -Raw
$Parsed = $Raw | ConvertFrom-Json
if ($null -eq $Parsed.failed -or $null -eq $Parsed.succeeded) {
    throw "G03 Automation report missing numeric pass/fail totals. Evidence: $Evidence"
}
if ([int]$Parsed.failed -ne 0 -or [int]$Parsed.succeeded -lt 2 -or
    $Raw -notmatch 'Everward.G03.Native.ScanSurveyPersistsAfterCancel' -or
    $Raw -notmatch 'Everward.G03.Native.MiningRejectsWithoutSurveyAndTool') {
    throw "G03 expected native tests did not both pass. Evidence: $Evidence"
}
Write-Output "UNREAL_VERIFIED (native G03 subset only): 2 Everward game-world automation tests passed on exact source $Head"
Write-Output "Evidence: $Evidence"
Write-Output 'This does NOT certify G01/G02 mining success, real key-input path, HUD, Fix_It, or paid Aura milestone acceptance.'
