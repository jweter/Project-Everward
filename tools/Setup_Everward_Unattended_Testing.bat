@echo off
setlocal EnableExtensions EnableDelayedExpansion
title Everward - One-Time Unattended Testing Setup
color 0A

echo ============================================================
echo   EVERWARD - ONE-TIME UNATTENDED TESTING SETUP
echo ============================================================
echo.
echo This setup will:
echo   1. prepare the dedicated Everward playtest checkout,
echo   2. register the 10-minute-idle Windows test worker,
echo   3. configure repo status reporting when GitHub CLI can authenticate.
echo.
echo After this, routine Everward build/preflight testing should happen
echo without you launching Unreal or repeating deterministic QA steps.
echo.

set "REPO_URL=https://github.com/jweter/Project-Everward.git"
set "TEST_ROOT=%USERPROFILE%\Documents\Everward Playtest"
set "REPO_DIR=%TEST_ROOT%\Project-Everward"
set "PASSED_SHA="
set "GH_EXE="

where git >nul 2>nul
if errorlevel 1 (
    echo ERROR: Git was not found in PATH.
    goto :failed
)

where powershell >nul 2>nul
if errorlevel 1 (
    echo ERROR: Windows PowerShell was not found.
    goto :failed
)

echo [1/5] Finding the latest successful main-branch Foundation build...
for /f "usebackq delims=" %%S in (`powershell -NoProfile -ExecutionPolicy Bypass -Command ^
    "$ErrorActionPreference='Stop';" ^
    "$headers=@{'User-Agent'='Everward-Unattended-Setup';'Accept'='application/vnd.github+json'};" ^
    "$u='https://api.github.com/repos/jweter/Project-Everward/actions/workflows/foundation.yml/runs?branch=main&status=success&event=push&per_page=20';" ^
    "$runs=(Invoke-RestMethod -Headers $headers -Uri $u).workflow_runs;" ^
    "$run=$runs | Where-Object { $_.head_branch -eq 'main' -and $_.conclusion -eq 'success' -and $_.event -eq 'push' } | Select-Object -First 1;" ^
    "if(-not $run){ throw 'No successful main Foundation run was found.' };" ^
    "Write-Output $run.head_sha"`) do set "PASSED_SHA=%%S"

if not defined PASSED_SHA (
    echo ERROR: Could not determine the latest successful main commit.
    goto :failed
)
echo       Passed commit: !PASSED_SHA!

if not exist "%TEST_ROOT%" mkdir "%TEST_ROOT%"

echo [2/5] Preparing the dedicated playtest checkout...
if not exist "%REPO_DIR%\.git" (
    git clone "%REPO_URL%" "%REPO_DIR%"
    if errorlevel 1 goto :failed
)

pushd "%REPO_DIR%"
git fetch --prune origin
if errorlevel 1 (
    popd
    goto :failed
)
git reset --hard >nul 2>nul
git clean -fd >nul 2>nul
git checkout --detach "!PASSED_SHA!"
if errorlevel 1 (
    popd
    goto :failed
)

if not exist "tools\register_unattended_product_reality.ps1" (
    echo ERROR: The passed build does not contain the unattended worker yet.
    echo Run this setup again after PR #241 is merged and main Foundation is green.
    popd
    goto :failed
)

echo [3/5] Explicitly authorizing this disposable playtest checkout and registering the Windows idle worker...
powershell.exe -NoProfile -ExecutionPolicy Bypass -File ".\tools\register_unattended_product_reality.ps1" -RepoRoot "%REPO_DIR%" -ConfirmDedicatedCheckout
if errorlevel 1 (
    popd
    goto :failed
)
popd

echo [4/5] Checking GitHub status reporting...
for /f "delims=" %%G in ('where gh 2^>nul') do if not defined GH_EXE set "GH_EXE=%%G"

if not defined GH_EXE if exist "%ProgramFiles%\GitHub CLI\gh.exe" set "GH_EXE=%ProgramFiles%\GitHub CLI\gh.exe"

if not defined GH_EXE (
    where winget >nul 2>nul
    if not errorlevel 1 (
        echo       GitHub CLI is not installed. Installing it once for status reporting...
        winget install --id GitHub.cli -e --source winget --silent --accept-package-agreements --accept-source-agreements
        if exist "%ProgramFiles%\GitHub CLI\gh.exe" set "GH_EXE=%ProgramFiles%\GitHub CLI\gh.exe"
    )
)

if defined GH_EXE (
    "!GH_EXE!" auth status >nul 2>nul
    if errorlevel 1 (
        echo.
        echo       ONE-TIME GITHUB AUTHORIZATION
        echo       A browser sign-in may open. This is only so the unattended
        echo       worker can update Everward status issue #243 for us.
        echo.
        "!GH_EXE!" auth login --web --hostname github.com --git-protocol https
    )
    "!GH_EXE!" auth status >nul 2>nul
    if not errorlevel 1 (
        echo       GitHub reporting: READY ^(issue #243^)
    ) else (
        echo       GitHub reporting: LOCAL ONLY until gh authentication succeeds.
    )
) else (
    echo       GitHub reporting: LOCAL ONLY ^(GitHub CLI unavailable^).
)

echo [5/5] Setup complete.
echo.
echo ============================================================
echo   YOU ARE OUT OF THE ROUTINE TEST LOOP
echo ============================================================
echo.
echo Leave the laptop plugged in with the sleep/lid settings you already set.
echo After about 10 minutes of Windows idle time, Everward will automatically:
echo   - find the latest Foundation-green main commit,
echo   - update only the dedicated playtest checkout,
echo   - run the full repository preflight,
echo   - build EverwardEditor with Unreal Engine 5.8,
echo   - store exact local evidence,
echo   - update GitHub issue #243 when GitHub CLI is authenticated.
echo.
echo You do not need to launch Unreal or repeat routine regression testing.
echo Human playtests are reserved for actual game feel and milestone experiences.
echo.
echo This window will close automatically.
timeout /t 15 /nobreak >nul
exit /b 0

:failed
color 0C
echo.
echo ============================================================
echo   SETUP NEEDS ATTENTION
echo ============================================================
echo.
echo The unattended worker was not fully registered. Nothing has been
echo declared passing. Take a screenshot of this window and send it to me.
echo.
pause
exit /b 1
