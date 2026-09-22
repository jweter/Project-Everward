[CmdletBinding()]
param(
    [string]$UnrealRoot = "",
    [int]$TimeoutSeconds = 900
)

$ErrorActionPreference = "Stop"
# Preserve the public parameter while enforcing a bounded floor for the low-spec
# worker's observed UE 5.8 cold-start path. The process was still responsive and
# consuming CPU when the previous five-minute limit killed it before engine startup.
$EffectiveTimeoutSeconds = [Math]::Max($TimeoutSeconds, 900)
$RepoRoot = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
$ProjectPath = Join-Path $RepoRoot "unreal\Everward.uproject"

if (-not (Test-Path $ProjectPath -PathType Leaf)) { throw "Everward project not found at $ProjectPath" }
$Candidates = New-Object System.Collections.Generic.List[string]
if ($UnrealRoot) { $Candidates.Add($UnrealRoot) }
foreach ($Name in @("UE58_ROOT", "UE_5_8_ROOT", "UE_5_8")) { $Value = [Environment]::GetEnvironmentVariable($Name); if ($Value) { $Candidates.Add($Value) } }
foreach ($CommonPath in @("C:\Program Files\Epic Games\UE_5.8", "C:\Epic Games\UE_5.8")) { $Candidates.Add($CommonPath) }
$EditorCmd = $null
foreach ($Candidate in $Candidates | Select-Object -Unique) { $CandidateCmd = Join-Path $Candidate "Engine\Binaries\Win64\UnrealEditor-Cmd.exe"; if (Test-Path $CandidateCmd -PathType Leaf) { $EditorCmd = (Resolve-Path $CandidateCmd).Path; break } }
if (-not $EditorCmd) { throw "Unreal Engine 5.8 command-line editor was not found." }
$GitCommit = (& git -C $RepoRoot rev-parse HEAD).Trim()
if ($LASTEXITCODE -ne 0 -or -not $GitCommit) { throw "Unable to resolve the current Git commit." }
$StateDir = Join-Path $env:LOCALAPPDATA "Everward\unattended-worker"
$LogDir = Join-Path $StateDir "logs"
New-Item -ItemType Directory -Force -Path $LogDir | Out-Null
$LogPath = Join-Path $LogDir "headless-smoke-$GitCommit.log"
$StdoutPath = Join-Path $LogDir "headless-smoke-$GitCommit.stdout.log"
$StderrPath = Join-Path $LogDir "headless-smoke-$GitCommit.stderr.log"
$Arguments = @("`"$ProjectPath`"", "-game", "-NullRHI", "-Unattended", "-NoSplash", "-NoSound", "-NoP4", "-NoAutoSDK", "-TestExit=`"Automation Test Queue Empty`"", "-log=`"$LogPath`"")
$Process = Start-Process -FilePath $EditorCmd -ArgumentList $Arguments -PassThru -WindowStyle Hidden -RedirectStandardOutput $StdoutPath -RedirectStandardError $StderrPath
if (-not $Process.WaitForExit($EffectiveTimeoutSeconds * 1000)) {
    try { & taskkill.exe /PID $Process.Id /T /F | Out-Null } catch { Stop-Process -Id $Process.Id -Force -ErrorAction SilentlyContinue }
    throw "Headless Unreal smoke timed out after $EffectiveTimeoutSeconds seconds. Unreal log: $LogPath; stdout: $StdoutPath; stderr: $StderrPath"
}
if ($Process.ExitCode -ne 0) { throw "Headless Unreal smoke failed with exit code $($Process.ExitCode). Log: $LogPath" }
Write-Host "PASS: Everward headless Unreal smoke completed for exact commit $GitCommit."
Write-Host "Log: $LogPath"
