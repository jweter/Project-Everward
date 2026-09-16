[CmdletBinding()]
param(
    [string]$RepoRoot = "",
    [switch]$ConfirmDedicatedCheckout,
    [switch]$RunOnceNow
)

$ErrorActionPreference = "Stop"
$TaskName = "Everward Unattended Product Reality"
$DisableVariable = "EVERWARD_DISABLE_UNATTENDED_WORKER"

if (-not $RepoRoot) {
    $RepoRoot = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
}
else {
    $RepoRoot = (Resolve-Path $RepoRoot).Path
}

$Worker = Join-Path $RepoRoot "tools\everward_unattended_worker.py"
$Runner = Join-Path $RepoRoot "tools\run_unattended_product_reality.ps1"
$GitDir = Join-Path $RepoRoot ".git"
$Sentinel = Join-Path $GitDir "everward-unattended-worker"

if (-not (Test-Path $GitDir -PathType Container)) {
    throw "Everward unattended registration requires a Git checkout: $RepoRoot"
}
if (-not (Test-Path $Worker -PathType Leaf)) {
    throw "Everward unattended worker is missing: $Worker"
}
if (-not (Test-Path $Runner -PathType Leaf)) {
    throw "Everward unattended runner is missing: $Runner"
}

$Disabled = [Environment]::GetEnvironmentVariable($DisableVariable)
if ($Disabled -and $Disabled.Trim().ToLowerInvariant() -in @("1", "true", "yes", "on")) {
    & schtasks.exe /Delete /TN $TaskName /F 2>$null | Out-Null
    Write-Host "Everward unattended Product Reality task is disabled. Any existing task was removed."
    exit 0
}

# Never turn an arbitrary development clone into a destructive unattended
# checkout as a side effect of a normal playtest. The sentinel may only be
# created by an explicit one-time setup that passes -ConfirmDedicatedCheckout.
if (-not (Test-Path $Sentinel -PathType Leaf)) {
    if (-not $ConfirmDedicatedCheckout) {
        throw "This checkout is not authorized for unattended destructive sync. Run the dedicated one-time setup or re-run with -ConfirmDedicatedCheckout only for the disposable Everward Playtest checkout."
    }
    "registered_utc=$((Get-Date).ToUniversalTime().ToString('o'))" | Set-Content -Path $Sentinel -Encoding ASCII
}

# Revalidate origin before scheduling any destructive-capable worker.
$Origin = (& git -C $RepoRoot remote get-url origin).Trim()
if ($LASTEXITCODE -ne 0 -or -not $Origin) {
    throw "Could not resolve the checkout origin."
}
$NormalizedOrigin = $Origin.Replace("\", "/").TrimEnd("/").ToLowerInvariant()
if ($NormalizedOrigin.StartsWith("git@github.com:")) {
    $NormalizedOrigin = "https://github.com/" + $NormalizedOrigin.Substring("git@github.com:".Length)
}
$NormalizedOrigin = $NormalizedOrigin.TrimEnd(".git")
if ($NormalizedOrigin -ne "https://github.com/jweter/project-everward") {
    throw "Unexpected origin remote; refusing unattended registration: $Origin"
}

$PythonCommand = Get-Command python -ErrorAction Stop
$Python = $PythonCommand.Source
if (-not $Python) {
    throw "Python was not found in PATH."
}

function ConvertTo-XmlText {
    param([string]$Value)
    return [System.Security.SecurityElement]::Escape($Value)
}

# Do not use schtasks /TR for the worker command. /TR reparses embedded quotes
# and broke the first real install because both the runner and repo live under
# "Everward Playtest", which contains a space. Register from XML instead so
# Command, Arguments, and WorkingDirectory remain separate Task Scheduler fields.
$CurrentUser = [System.Security.Principal.WindowsIdentity]::GetCurrent().Name
$ActionArguments = "-NoProfile -ExecutionPolicy Bypass -File `"$Runner`" -RepoRoot `"$RepoRoot`""
$EscapedUser = ConvertTo-XmlText $CurrentUser
$EscapedArguments = ConvertTo-XmlText $ActionArguments
$EscapedRepoRoot = ConvertTo-XmlText $RepoRoot
$TaskXmlPath = Join-Path ([System.IO.Path]::GetTempPath()) "everward-unattended-task-$PID.xml"

$TaskXml = @"
<?xml version="1.0" encoding="UTF-16"?>
<Task version="1.4" xmlns="http://schemas.microsoft.com/windows/2004/02/mit/task">
  <RegistrationInfo>
    <Description>Everward unattended deterministic Windows and Unreal verification worker.</Description>
  </RegistrationInfo>
  <Triggers>
    <IdleTrigger>
      <Enabled>true</Enabled>
    </IdleTrigger>
  </Triggers>
  <Principals>
    <Principal id="Author">
      <UserId>$EscapedUser</UserId>
      <LogonType>InteractiveToken</LogonType>
      <RunLevel>LeastPrivilege</RunLevel>
    </Principal>
  </Principals>
  <Settings>
    <MultipleInstancesPolicy>IgnoreNew</MultipleInstancesPolicy>
    <DisallowStartIfOnBatteries>false</DisallowStartIfOnBatteries>
    <StopIfGoingOnBatteries>false</StopIfGoingOnBatteries>
    <AllowHardTerminate>true</AllowHardTerminate>
    <StartWhenAvailable>true</StartWhenAvailable>
    <RunOnlyIfNetworkAvailable>false</RunOnlyIfNetworkAvailable>
    <IdleSettings>
      <Duration>PT10M</Duration>
      <WaitTimeout>PT1H</WaitTimeout>
      <StopOnIdleEnd>false</StopOnIdleEnd>
      <RestartOnIdle>false</RestartOnIdle>
    </IdleSettings>
    <AllowStartOnDemand>true</AllowStartOnDemand>
    <Enabled>true</Enabled>
    <Hidden>false</Hidden>
    <WakeToRun>false</WakeToRun>
    <ExecutionTimeLimit>PT0S</ExecutionTimeLimit>
    <Priority>7</Priority>
  </Settings>
  <Actions Context="Author">
    <Exec>
      <Command>powershell.exe</Command>
      <Arguments>$EscapedArguments</Arguments>
      <WorkingDirectory>$EscapedRepoRoot</WorkingDirectory>
    </Exec>
  </Actions>
</Task>
"@

try {
    $TaskXml | Set-Content -Path $TaskXmlPath -Encoding Unicode
    & schtasks.exe /Create /TN $TaskName /XML $TaskXmlPath /F | Out-Null
    if ($LASTEXITCODE -ne 0) {
        throw "Windows Task Scheduler could not register the Everward unattended worker from XML (exit $LASTEXITCODE)."
    }

    # Fail closed if Windows claimed success but the task cannot be queried back.
    & schtasks.exe /Query /TN $TaskName /FO LIST 2>$null | Out-Null
    if ($LASTEXITCODE -ne 0) {
        throw "Windows Task Scheduler registration could not be verified after creation."
    }
}
finally {
    Remove-Item -Force $TaskXmlPath -ErrorAction SilentlyContinue
}

# First-time explicit setup should produce evidence immediately instead of leaving the
# repository status issue empty until Windows happens to become idle. This switch is NOT
# used by normal playtest/desktop refresh paths, so ordinary launches do not trigger a
# heavy Unreal build. Task Scheduler starts the already-registered worker asynchronously.
if ($RunOnceNow) {
    & schtasks.exe /Run /TN $TaskName | Out-Null
    if ($LASTEXITCODE -ne 0) {
        throw "Windows Task Scheduler registered the worker but could not start the requested first run."
    }
    Write-Host "Started the first unattended verification run in the background."
}

Write-Host "Registered/refreshed Everward unattended Product Reality worker."
Write-Host "It will run after approximately 10 minutes of Windows idle time without launching Unreal Editor."
Write-Host "If GitHub CLI is authenticated, the latest sanitized status is also written to issue #243."
