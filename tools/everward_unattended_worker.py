from __future__ import annotations

import argparse
import json
import os
import re
import shutil
import socket
import subprocess
import sys
import time
import urllib.parse
import urllib.request
from datetime import datetime, timezone
from pathlib import Path
from typing import Any, Literal

Status = Literal["PASS", "FAIL", "REVIEW_REQUIRED", "BUSY"]

REPO_OWNER = "jweter"
REPO_NAME = "Project-Everward"
EXPECTED_ORIGIN = "https://github.com/jweter/Project-Everward.git"
FOUNDATION_WORKFLOW = "foundation.yml"
SENTINEL_NAME = "everward-unattended-worker"
MANUAL_LOCK_NAME = "everward-manual-playtest.lock"
LOCK_NAME = "worker.lock"
LAST_PASS_MARKER = "last_passed_headless_smoke_commit.txt"
LOW_SPEC_PASS_MARKER = "last_passed_low_spec_startup_commit.txt"
REPORT_SCHEMA_VERSION = 1
LOW_SPEC_RESULT_SCHEMA_VERSION = 1
LOW_SPEC_STARTUP_TIMEOUT_SECONDS = 900
LOW_SPEC_SAMPLE_WINDOW_SECONDS = 60
LOW_SPEC_NOT_EVIDENCE_FOR = [
    "visual quality or art direction",
    "control or movement feel",
    "HUD readability",
    "frame time or playability",
    "gameplay loop quality",
]


def utc_now() -> str:
    return datetime.now(timezone.utc).isoformat()


def default_state_dir() -> Path:
    local = os.environ.get("LOCALAPPDATA")
    if local:
        return Path(local) / "Everward" / "unattended-worker"
    return Path.home() / ".everward" / "unattended-worker"


def normalize_origin(value: str) -> str:
    text = value.strip().replace("\\", "/")
    if text.endswith("/"):
        text = text[:-1]
    if text.startswith("git@github.com:"):
        text = "https://github.com/" + text.removeprefix("git@github.com:")
    return text.lower()


def origin_is_expected(value: str) -> bool:
    return normalize_origin(value).removesuffix(".git") == normalize_origin(EXPECTED_ORIGIN).removesuffix(".git")


def select_successful_main_sha(payload: dict[str, Any]) -> str:
    for run in payload.get("workflow_runs", []):
        if not isinstance(run, dict):
            continue
        sha = str(run.get("head_sha") or "")
        if (
            run.get("head_branch") == "main"
            and run.get("conclusion") == "success"
            and run.get("event") == "push"
            and re.fullmatch(r"[0-9a-fA-F]{40}", sha)
        ):
            return sha.lower()
    raise RuntimeError("No successful main-branch Foundation checks run was found.")


def latest_successful_main_sha(timeout_seconds: float = 20.0) -> str:
    query = urllib.parse.urlencode(
        {"branch": "main", "status": "success", "event": "push", "per_page": "20"}
    )
    url = (
        f"https://api.github.com/repos/{REPO_OWNER}/{REPO_NAME}/actions/workflows/"
        f"{FOUNDATION_WORKFLOW}/runs?{query}"
    )
    request = urllib.request.Request(
        url,
        headers={
            "User-Agent": "Everward-Unattended-Product-Reality",
            "Accept": "application/vnd.github+json",
        },
    )
    with urllib.request.urlopen(request, timeout=timeout_seconds) as response:
        payload = json.loads(response.read().decode("utf-8"))
    if not isinstance(payload, dict):
        raise RuntimeError("GitHub Actions response was not an object.")
    return select_successful_main_sha(payload)


def run_capture(
    args: list[str], *, cwd: Path | None = None, timeout: float = 30.0
) -> subprocess.CompletedProcess[str]:
    return subprocess.run(
        args,
        cwd=str(cwd) if cwd else None,
        check=False,
        capture_output=True,
        text=True,
        timeout=timeout,
    )


def git_output(repo_root: Path, *args: str) -> str:
    proc = run_capture(["git", "-C", str(repo_root), *args], timeout=60.0)
    if proc.returncode != 0:
        detail = (proc.stderr or proc.stdout or "git command failed").strip()
        raise RuntimeError(f"git {' '.join(args)} failed: {detail[:500]}")
    return proc.stdout.strip()


def validate_dedicated_checkout(repo_root: Path) -> None:
    git_dir = repo_root / ".git"
    sentinel = git_dir / SENTINEL_NAME
    if not git_dir.is_dir():
        raise RuntimeError(f"Not a Git checkout: {repo_root}")
    if not sentinel.is_file():
        raise RuntimeError(
            "Dedicated-playtest sentinel is missing; refusing to reset or clean this checkout. "
            "Run tools/register_unattended_product_reality.ps1 with explicit dedicated-checkout confirmation first."
        )
    origin = git_output(repo_root, "remote", "get-url", "origin")
    if not origin_is_expected(origin):
        raise RuntimeError(f"Unexpected origin remote; refusing destructive sync: {origin}")


def process_running(image_name: str) -> bool:
    if os.name != "nt":
        return False
    proc = run_capture(["tasklist.exe", "/FI", f"IMAGENAME eq {image_name}", "/NH"], timeout=10.0)
    return proc.returncode == 0 and image_name.lower() in proc.stdout.lower()


def unreal_build_running() -> bool:
    if os.name != "nt":
        return False
    script = (
        "$p=Get-CimInstance Win32_Process | Where-Object { "
        "($_.Name -like 'UnrealBuildTool*') -or "
        "(($_.Name -eq 'dotnet.exe') -and ($_.CommandLine -like '*UnrealBuildTool*')) "
        "} | Select-Object -First 1 -ExpandProperty ProcessId; "
        "if($p){Write-Output $p}"
    )
    proc = run_capture(
        ["powershell.exe", "-NoProfile", "-Command", script], timeout=15.0
    )
    return proc.returncode == 0 and bool(proc.stdout.strip())


def pid_is_alive(pid: int) -> bool:
    if pid <= 0:
        return False
    if os.name == "nt":
        proc = run_capture(["tasklist.exe", "/FI", f"PID eq {pid}", "/NH"], timeout=10.0)
        return proc.returncode == 0 and re.search(rf"\b{pid}\b", proc.stdout) is not None
    try:
        os.kill(pid, 0)
    except OSError:
        return False
    return True


def manual_playtest_active(repo_root: Path) -> bool:
    lock = repo_root / ".git" / MANUAL_LOCK_NAME
    if lock.is_file():
        try:
            pid = int(lock.read_text(encoding="ascii").strip())
        except (OSError, ValueError):
            pid = -1
        if pid_is_alive(pid):
            return True
        try:
            lock.unlink()
        except OSError:
            return True
    return (
        process_running("UnrealEditor.exe")
        or process_running("UnrealEditor-Cmd.exe")
        or unreal_build_running()
    )


def preserve_playtest_evidence(repo_root: Path, state_dir: Path) -> Path | None:
    source = repo_root / "playtests" / "phase2" / "observations"
    if not source.exists():
        return None
    files = [path for path in source.rglob("*") if path.is_file()]
    if not files:
        return None
    stamp = datetime.now(timezone.utc).strftime("%Y%m%dT%H%M%S.%fZ")
    destination = state_dir / "preserved-playtests" / stamp / "phase2-observations"
    destination.mkdir(parents=True, exist_ok=True)
    for path in files:
        relative = path.relative_to(source)
        target = destination / relative
        target.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(path, target)
    return destination


def sync_to_target(repo_root: Path, target_sha: str, state_dir: Path) -> tuple[str, Path | None]:
    validate_dedicated_checkout(repo_root)
    if not re.fullmatch(r"[0-9a-f]{40}", target_sha):
        raise RuntimeError(f"Invalid target commit: {target_sha}")
    if manual_playtest_active(repo_root):
        raise BlockingIOError(
            "Manual Unreal playtest/build is active; unattended checkout mutation is deferred."
        )

    preserved = preserve_playtest_evidence(repo_root, state_dir)
    git_output(repo_root, "fetch", "--prune", "origin")
    exists = run_capture(
        ["git", "-C", str(repo_root), "cat-file", "-e", f"{target_sha}^{{commit}}"],
        timeout=30.0,
    )
    if exists.returncode != 0:
        raise RuntimeError(f"Green commit {target_sha} was not present after fetch.")

    git_output(repo_root, "reset", "--hard")
    git_output(repo_root, "clean", "-fd")
    git_output(repo_root, "checkout", "--detach", target_sha)
    actual = git_output(repo_root, "rev-parse", "HEAD").lower()
    if actual != target_sha:
        raise RuntimeError(f"Checkout identity mismatch: expected {target_sha}, got {actual}")
    return actual, preserved


def resolve_unreal_root(explicit_root: str | None = None) -> Path:
    candidates: list[Path] = []
    if explicit_root:
        candidates.append(Path(explicit_root))
    for name in ("UE58_ROOT", "UE_5_8_ROOT", "UE_5_8"):
        value = os.environ.get(name)
        if value:
            candidates.append(Path(value))

    if os.name == "nt":
        try:
            import winreg  # type: ignore

            for hive, key_path in (
                (winreg.HKEY_LOCAL_MACHINE, r"SOFTWARE\EpicGames\Unreal Engine\5.8"),
                (winreg.HKEY_CURRENT_USER, r"SOFTWARE\EpicGames\Unreal Engine\5.8"),
                (winreg.HKEY_LOCAL_MACHINE, r"SOFTWARE\WOW6432Node\EpicGames\Unreal Engine\5.8"),
            ):
                try:
                    with winreg.OpenKey(hive, key_path) as key:
                        value, _ = winreg.QueryValueEx(key, "InstalledDirectory")
                    if value:
                        candidates.append(Path(str(value)))
                except OSError:
                    pass
        except ImportError:
            pass

    candidates.extend(
        [Path(r"C:\Program Files\Epic Games\UE_5.8"), Path(r"C:\Epic Games\UE_5.8")]
    )

    seen: set[str] = set()
    for candidate in candidates:
        key = str(candidate).lower()
        if key in seen:
            continue
        seen.add(key)
        build_bat = candidate / "Engine" / "Build" / "BatchFiles" / "Build.bat"
        editor_exe = candidate / "Engine" / "Binaries" / "Win64" / "UnrealEditor.exe"
        if build_bat.is_file() and editor_exe.is_file():
            return candidate.resolve()
    raise RuntimeError(
        "Unreal Engine 5.8 was not found by explicit path, environment, registry, or standard locations."
    )


def sanitize_text(text: str, *, repo_root: Path, unreal_root: Path | None = None) -> str:
    replacements = [(str(repo_root), "<REPO_ROOT>")]
    if unreal_root is not None:
        replacements.append((str(unreal_root), "<UE_5_8_ROOT>"))
    # Replace specific roots before home so nested paths retain semantic redaction.
    replacements.append((str(Path.home()), "%USERPROFILE%"))
    result = text
    for raw, replacement in replacements:
        if raw:
            for variant in (raw, raw.replace("\\", "/")):
                # Windows paths are case-insensitive and Unreal logs often change drive/letter case.
                result = re.sub(re.escape(variant), lambda _m, r=replacement: r, result, flags=re.IGNORECASE)
    user = os.environ.get("USERNAME") or os.environ.get("USER") or ""
    if len(user) >= 3:
        result = re.sub(rf"(?<![A-Za-z0-9_]){re.escape(user)}(?![A-Za-z0-9_])", "<USER>", result, flags=re.IGNORECASE)
    return result


def run_logged(
    args: list[str], *, cwd: Path, log_path: Path, timeout_seconds: float
) -> tuple[int, float]:
    log_path.parent.mkdir(parents=True, exist_ok=True)
    started = time.monotonic()
    with log_path.open("w", encoding="utf-8", errors="replace") as handle:
        proc = subprocess.Popen(
            args,
            cwd=str(cwd),
            stdout=handle,
            stderr=subprocess.STDOUT,
            text=True,
            creationflags=subprocess.CREATE_NEW_PROCESS_GROUP if os.name == "nt" else 0,
        )
        try:
            code = int(proc.wait(timeout=timeout_seconds))
        except subprocess.TimeoutExpired:
            if os.name == "nt":
                tree_killed = False
                for _attempt in range(2):
                    try:
                        result = subprocess.run(
                            ["taskkill.exe", "/PID", str(proc.pid), "/T", "/F"],
                            check=False,
                            capture_output=True,
                            text=True,
                            timeout=30.0,
                        )
                    except (OSError, subprocess.SubprocessError):
                        continue
                    if result.returncode == 0:
                        tree_killed = True
                        break
                if not tree_killed:
                    proc.kill()
                    handle.write("\nPROCESS_TREE_CLEANUP_FAILED\n")
            else:
                proc.kill()
            try:
                proc.wait(timeout=10.0)
            except subprocess.TimeoutExpired:
                proc.kill()
                proc.wait()
            handle.write(f"\nTIMEOUT after {timeout_seconds:.0f} seconds\n")
            code = 124
    return code, time.monotonic() - started


def log_tail(
    path: Path, *, repo_root: Path, unreal_root: Path | None = None, lines: int = 30
) -> str:
    try:
        content = path.read_text(encoding="utf-8", errors="replace").splitlines()
    except OSError:
        return ""
    return sanitize_text(
        "\n".join(content[-lines:]), repo_root=repo_root, unreal_root=unreal_root
    )


def _provision_cmake_path() -> str | None:
    if shutil.which("cmake"):
        return None
    candidates = [
        Path(r"C:\Program Files\Microsoft Visual Studio\18\Insiders\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin"),
        Path(r"C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin"),
        Path(r"C:\Program Files\Microsoft Visual Studio\2022\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin"),
    ]
    for candidate in candidates:
        if (candidate / "cmake.exe").is_file():
            os.environ["PATH"] = str(candidate) + os.pathsep + os.environ.get("PATH", "")
            return str(candidate)
    return None


def run_full_preflight(repo_root: Path, log_path: Path) -> dict[str, Any]:
    script = repo_root / "tools" / "quality_preflight.py"
    if not script.is_file():
        return {"status": "REVIEW_REQUIRED", "reason": "tools/quality_preflight.py is missing"}
    _provision_cmake_path()
    code, duration = run_logged(
        [sys.executable, str(script), "--full"],
        cwd=repo_root,
        log_path=log_path,
        timeout_seconds=60.0 * 45.0,
    )
    return {
        "status": "PASS" if code == 0 else "FAIL",
        "exit_code": code,
        "duration_seconds": round(duration, 3),
        "log": str(log_path),
        "failure_tail": "" if code == 0 else log_tail(log_path, repo_root=repo_root),
    }


def run_unreal_build(repo_root: Path, unreal_root: Path, log_path: Path) -> dict[str, Any]:
    uproject = repo_root / "unreal" / "Everward.uproject"
    build_bat = unreal_root / "Engine" / "Build" / "BatchFiles" / "Build.bat"
    if not uproject.is_file():
        return {"status": "REVIEW_REQUIRED", "reason": "unreal/Everward.uproject is missing"}
    if not build_bat.is_file():
        return {"status": "REVIEW_REQUIRED", "reason": "Unreal Engine 5.8 Build.bat is missing"}
    if os.name != "nt":
        return {"status": "REVIEW_REQUIRED", "reason": "Unreal build lane requires Windows"}

    code, duration = run_logged(
        [
            str(build_bat),
            "EverwardEditor",
            "Win64",
            "Development",
            str(uproject),
            "-WaitMutex",
            "-NoHotReloadFromIDE",
            "-MaxParallelActions=2",
        ],
        cwd=repo_root,
        log_path=log_path,
        timeout_seconds=60.0 * 60.0,
    )
    return {
        "status": "PASS" if code == 0 else "FAIL",
        "exit_code": code,
        "duration_seconds": round(duration, 3),
        "log": str(log_path),
        "failure_tail": ""
        if code == 0
        else log_tail(log_path, repo_root=repo_root, unreal_root=unreal_root),
    }


def run_unreal_headless_smoke(
    repo_root: Path, unreal_root: Path, log_path: Path
) -> dict[str, Any]:
    script = repo_root / "tools" / "run_unreal_headless_smoke.ps1"
    if not script.is_file():
        return {
            "status": "REVIEW_REQUIRED",
            "reason": "tools/run_unreal_headless_smoke.ps1 is missing",
        }
    if os.name != "nt":
        return {"status": "REVIEW_REQUIRED", "reason": "Unreal headless smoke requires Windows"}

    code, duration = run_logged(
        [
            "powershell.exe",
            "-NoProfile",
            "-ExecutionPolicy",
            "Bypass",
            "-File",
            str(script),
            "-UnrealRoot",
            str(unreal_root),
            "-TimeoutSeconds",
            "120",
        ],
        cwd=repo_root,
        log_path=log_path,
        timeout_seconds=930.0,
    )
    return {
        "status": "PASS" if code == 0 else "FAIL",
        "exit_code": code,
        "duration_seconds": round(duration, 3),
        "log": str(log_path),
        "failure_tail": ""
        if code == 0
        else log_tail(log_path, repo_root=repo_root, unreal_root=unreal_root),
    }


def read_unreal_engine_version(unreal_root: Path) -> str | None:
    try:
        payload = json.loads(
            (unreal_root / "Engine" / "Build" / "Build.version").read_text(encoding="utf-8-sig")
        )
        parts = [int(payload[key]) for key in ("MajorVersion", "MinorVersion", "PatchVersion")]
        changelist = int(payload.get("Changelist", 0))
    except (OSError, ValueError, TypeError, KeyError, AttributeError):
        return None
    return f"{parts[0]}.{parts[1]}.{parts[2]}-CL{changelist}"


def _finite_number(value: Any, *, positive: bool = False) -> float | None:
    if isinstance(value, bool) or not isinstance(value, (int, float)):
        return None
    number = float(value)
    if number != number or number in (float("inf"), float("-inf")):
        return None
    if number < 0 or (positive and number <= 0):
        return None
    return round(number, 1)


def interpret_low_spec_startup(
    payload: Any, *, expected_commit: str, runner_exit_code: int
) -> dict[str, Any]:
    """Classify raw low-spec startup facts fail-closed.

    Only allow-listed numeric/boolean facts are copied out of the probe result.
    PASS means bounded engine startup plus measured process memory for the exact
    commit; it never implies visual, control-feel, frame-time, or gameplay PASS.
    """
    check: dict[str, Any] = {
        "status": "REVIEW_REQUIRED",
        "profile": "low-spec-development",
        "scope": "bounded editor startup and process working-set telemetry only",
        "product_reality_claimed": False,
        "not_evidence_for": list(LOW_SPEC_NOT_EVIDENCE_FOR),
        "exit_code": runner_exit_code,
    }
    if runner_exit_code == 124:
        check.update(status="FAIL", failure_category="probe_timeout",
                     reason="Low-spec startup probe exceeded its outer time bound.")
        return check
    if not isinstance(payload, dict) or payload.get("schema_version") != LOW_SPEC_RESULT_SCHEMA_VERSION:
        check["reason"] = "Low-spec startup result was missing or had an unexpected schema."
        return check
    commit = str(payload.get("git_commit") or "").lower()
    if not re.fullmatch(r"[0-9a-f]{40}", commit) or commit != expected_commit:
        check["reason"] = "Low-spec startup build identity did not match the exact tested commit."
        return check
    check["commit"] = commit

    marker = payload.get("startup_marker_observed") is True
    exited = payload.get("exited_before_window_end") is True
    exit_code = payload.get("exit_code")
    facts = {
        "startup_marker_observed": marker,
        "startup_seconds": _finite_number(payload.get("startup_seconds")),
        "engine_reported_init_seconds": _finite_number(payload.get("engine_reported_init_seconds")),
        "startup_timeout_seconds": _finite_number(payload.get("startup_timeout_seconds")),
        "sample_window_seconds": _finite_number(payload.get("sample_window_seconds")),
        "sampled_after_startup_seconds": _finite_number(payload.get("sampled_after_startup_seconds")),
        "exited_before_window_end": exited,
        "editor_exit_code": exit_code if isinstance(exit_code, int) and not isinstance(exit_code, bool) else None,
    }
    check["startup"] = facts
    peak = _finite_number(payload.get("peak_working_set_mib"), positive=True)
    samples = payload.get("memory_sample_count")
    samples = samples if isinstance(samples, int) and not isinstance(samples, bool) and samples >= 0 else 0
    check["memory"] = {
        "metric": "editor_peak_working_set_mib",
        "value": peak,
        "sample_count": samples,
        "source": "UnrealEditor process WorkingSet64",
        "interpretation": "process working set only; not total system RAM or shared-GPU memory",
    }
    cleanup = payload.get("cleanup")
    check["cleanup"] = cleanup if cleanup in ("terminated", "already_exited", "failed", "pending", "not_started") else "unknown"

    if payload.get("editor_launched") is not True:
        check["reason"] = "Unreal Editor was not launched; low-spec startup telemetry is unavailable."
        return check
    if check["cleanup"] not in ("terminated", "already_exited"):
        check["reason"] = "The launched Unreal Editor could not be confirmed stopped."
        return check
    if not marker:
        category = "exited_before_startup" if exited else "startup_timeout"
        check.update(status="FAIL", failure_category=category,
                     reason="Unreal Editor did not reach the engine initialization marker within the bound.")
        return check
    if exited:
        check.update(status="FAIL", failure_category="exited_during_sample_window",
                     reason="Unreal Editor exited during the post-startup memory sample window.")
        return check
    window = facts["sample_window_seconds"]
    sampled = facts["sampled_after_startup_seconds"]
    if facts["startup_seconds"] is None or window is None or sampled is None or sampled + 1.0 < window:
        check["reason"] = "Startup timing or the post-startup sample window was incomplete."
        return check
    if peak is None or samples <= 0:
        check["reason"] = "Process memory telemetry was unavailable."
        return check
    check["status"] = "PASS"
    return check


def run_unreal_low_spec_startup(
    repo_root: Path, unreal_root: Path, tested_commit: str, log_dir: Path, state_dir: Path
) -> dict[str, Any]:
    script = repo_root / "tools" / "run_unattended_low_spec_startup.ps1"
    if not script.is_file():
        return {"status": "REVIEW_REQUIRED", "reason": "tools/run_unattended_low_spec_startup.ps1 is missing"}
    if os.name != "nt":
        return {"status": "REVIEW_REQUIRED", "reason": "Low-spec startup evidence requires Windows"}

    runner_log = log_dir / "unreal-low-spec-startup-runner.log"
    unreal_log = log_dir / "unreal-low-spec-startup.log"
    result_path = log_dir / "unreal-low-spec-startup.json"
    result_path.unlink(missing_ok=True)
    code, duration = run_logged(
        [
            "powershell.exe",
            "-NoProfile",
            "-ExecutionPolicy",
            "Bypass",
            "-File",
            str(script),
            "-UnrealRoot",
            str(unreal_root),
            "-ResultPath",
            str(result_path),
            "-LogPath",
            str(unreal_log),
            "-StartupTimeoutSeconds",
            str(LOW_SPEC_STARTUP_TIMEOUT_SECONDS),
            "-SampleWindowSeconds",
            str(LOW_SPEC_SAMPLE_WINDOW_SECONDS),
        ],
        cwd=repo_root,
        log_path=runner_log,
        timeout_seconds=float(LOW_SPEC_STARTUP_TIMEOUT_SECONDS + LOW_SPEC_SAMPLE_WINDOW_SECONDS + 240),
    )
    try:
        payload: Any = json.loads(result_path.read_text(encoding="utf-8-sig"))
    except (OSError, ValueError):
        payload = None
    check = interpret_low_spec_startup(payload, expected_commit=tested_commit, runner_exit_code=code)
    check["duration_seconds"] = round(duration, 3)
    check["build_identity"] = {
        "commit": tested_commit,
        "target": "EverwardEditor Win64 Development",
        "unreal_engine_version": read_unreal_engine_version(unreal_root),
    }
    check["log"] = unreal_log.relative_to(state_dir).as_posix() if unreal_log.is_relative_to(state_dir) else unreal_log.name
    if isinstance(payload, dict) and isinstance(payload.get("error"), str) and payload["error"]:
        check["probe_error"] = sanitize_text(
            payload["error"], repo_root=repo_root, unreal_root=unreal_root
        )[:300]
    if check["status"] != "PASS":
        tail_source = unreal_log if unreal_log.is_file() else runner_log
        check["failure_tail"] = log_tail(tail_source, repo_root=repo_root, unreal_root=unreal_root)
    return check


def acquire_lock(state_dir: Path) -> tuple[int | None, bool]:
    state_dir.mkdir(parents=True, exist_ok=True)
    path = state_dir / LOCK_NAME
    for attempt in range(2):
        try:
            fd = os.open(path, os.O_CREAT | os.O_EXCL | os.O_WRONLY)
            os.write(fd, str(os.getpid()).encode("ascii"))
            return fd, True
        except FileExistsError:
            try:
                pid = int(path.read_text(encoding="ascii").strip())
            except (OSError, ValueError):
                if attempt == 0:
                    time.sleep(0.25)
                    continue
                pid = -1
            if pid_is_alive(pid):
                return None, False
            path.unlink(missing_ok=True)
    return None, False


def release_lock(state_dir: Path, fd: int) -> None:
    try:
        os.close(fd)
    finally:
        path = state_dir / LOCK_NAME
        try:
            if path.read_text(encoding="ascii").strip() == str(os.getpid()):
                path.unlink(missing_ok=True)
        except OSError:
            pass


def aggregate_status(checks: dict[str, dict[str, Any]]) -> Status:
    statuses = [str(item.get("status")) for item in checks.values()]
    if "FAIL" in statuses:
        return "FAIL"
    if "REVIEW_REQUIRED" in statuses:
        return "REVIEW_REQUIRED"
    return "PASS"


def _read_commit_marker(path: Path) -> str | None:
    try:
        value = path.read_text(encoding="ascii").strip().lower()
    except OSError:
        return None
    return value if re.fullmatch(r"[0-9a-f]{40}", value) else None


def read_last_pass(state_dir: Path) -> str | None:
    """Return the exact commit that passed every unattended lane, including low-spec startup."""
    smoke = _read_commit_marker(state_dir / LAST_PASS_MARKER)
    low_spec = _read_commit_marker(state_dir / LOW_SPEC_PASS_MARKER)
    return smoke if smoke is not None and smoke == low_spec else None


def write_report(
    state_dir: Path, report: dict[str, Any], *, update_latest: bool = True
) -> Path:
    history = state_dir / "history"
    history.mkdir(parents=True, exist_ok=True)
    stamp = datetime.now(timezone.utc).strftime("%Y%m%dT%H%M%S.%fZ")
    history_path = history / f"{stamp}.json"
    payload = json.dumps(report, indent=2, sort_keys=True) + "\n"
    history_path.write_text(payload, encoding="utf-8")
    if update_latest:
        (state_dir / "latest.json").write_text(payload, encoding="utf-8")
    return history_path


def run_worker(
    repo_root: Path, state_dir: Path, explicit_unreal_root: str | None = None
) -> dict[str, Any]:
    started = utc_now()
    report: dict[str, Any] = {
        "schema_version": REPORT_SCHEMA_VERSION,
        "started_at_utc": started,
        "completed_at_utc": started,
        "host": socket.gethostname(),
        "repo_root": str(repo_root),
        "result": "REVIEW_REQUIRED",
        "target_commit": None,
        "tested_commit": None,
        "checks": {},
        "human_only_debt": [
            "visual/art-direction acceptance",
            "control and movement feel",
            "HUD/readability preference",
            "gameplay desire-to-continue judgment",
            "actual frame-time/playability until automated Unreal telemetry exists",
        ],
        "notes": [],
    }

    try:
        validate_dedicated_checkout(repo_root)
        report["checks"]["dedicated_checkout"] = {"status": "PASS"}
    except Exception as exc:
        report["checks"]["dedicated_checkout"] = {
            "status": "REVIEW_REQUIRED",
            "reason": str(exc),
        }
        report["completed_at_utc"] = utc_now()
        report["result"] = aggregate_status(report["checks"])
        return report

    if manual_playtest_active(repo_root):
        report["checks"]["editor_idle"] = {
            "status": "REVIEW_REQUIRED",
            "reason": "Manual Unreal playtest/build is active; unattended verification deferred without touching the checkout.",
        }
        report["completed_at_utc"] = utc_now()
        report["result"] = "REVIEW_REQUIRED"
        return report
    report["checks"]["editor_idle"] = {"status": "PASS"}

    try:
        target = latest_successful_main_sha()
        report["target_commit"] = target
        report["checks"]["green_main_discovery"] = {"status": "PASS", "commit": target}
    except Exception as exc:
        report["checks"]["green_main_discovery"] = {
            "status": "REVIEW_REQUIRED",
            "reason": str(exc),
        }
        report["completed_at_utc"] = utc_now()
        report["result"] = aggregate_status(report["checks"])
        return report

    if read_last_pass(state_dir) == target:
        report["tested_commit"] = target
        report["checks"]["cached_exact_commit"] = {
            "status": "PASS",
            "reason": "This exact Foundation-green commit already passed unattended full preflight, UBT build, and headless Unreal smoke, plus bounded low-spec startup/memory evidence.",
        }
        report["notes"].append("No rerun was needed; exact fully verified commit is unchanged.")
        report["completed_at_utc"] = utc_now()
        report["result"] = "PASS"
        return report

    try:
        synced, preserved = sync_to_target(repo_root, target, state_dir)
        report["tested_commit"] = synced
        report["checks"]["checkout_sync"] = {"status": "PASS", "commit": synced}
        if preserved is not None:
            report["notes"].append(
                "Existing Phase-2 observation evidence was copied to local worker preservation storage before checkout cleanup."
            )
    except BlockingIOError as exc:
        report["checks"]["checkout_sync"] = {
            "status": "REVIEW_REQUIRED",
            "reason": str(exc),
        }
        report["completed_at_utc"] = utc_now()
        report["result"] = "REVIEW_REQUIRED"
        return report
    except Exception as exc:
        report["checks"]["checkout_sync"] = {
            "status": "REVIEW_REQUIRED",
            "reason": str(exc),
        }
        report["completed_at_utc"] = utc_now()
        report["result"] = aggregate_status(report["checks"])
        return report

    logs = state_dir / "logs" / target[:12]
    preflight = run_full_preflight(repo_root, logs / "full-preflight.log")
    report["checks"]["full_preflight"] = preflight
    if preflight.get("status") != "PASS":
        report["notes"].append(
            "UBT build and headless smoke were not attempted because canonical full preflight did not pass."
        )
        report["completed_at_utc"] = utc_now()
        report["result"] = aggregate_status(report["checks"])
        return report

    try:
        unreal_root = resolve_unreal_root(explicit_unreal_root)
        report["checks"]["unreal_5_8"] = {"status": "PASS", "root": str(unreal_root)}
    except Exception as exc:
        report["checks"]["unreal_5_8"] = {
            "status": "REVIEW_REQUIRED",
            "reason": str(exc),
        }
        report["completed_at_utc"] = utc_now()
        report["result"] = aggregate_status(report["checks"])
        return report

    unreal_build = run_unreal_build(repo_root, unreal_root, logs / "unreal-build.log")
    report["checks"]["unreal_editor_build"] = unreal_build
    if unreal_build.get("status") != "PASS":
        report["notes"].append(
            "Headless Unreal smoke was not attempted because the Unreal editor build did not pass."
        )
        report["completed_at_utc"] = utc_now()
        report["result"] = aggregate_status(report["checks"])
        return report

    headless_smoke = run_unreal_headless_smoke(
        repo_root, unreal_root, logs / "unreal-headless-smoke.log"
    )
    report["checks"]["unreal_headless_smoke"] = headless_smoke
    if headless_smoke.get("status") != "PASS":
        report["notes"].append(
            "Low-spec startup evidence was not attempted because the headless Unreal smoke did not pass."
        )
        report["completed_at_utc"] = utc_now()
        report["result"] = aggregate_status(report["checks"])
        return report

    low_spec = run_unreal_low_spec_startup(repo_root, unreal_root, synced, logs, state_dir)
    report["checks"]["unreal_low_spec_startup"] = low_spec
    report["completed_at_utc"] = utc_now()
    report["result"] = aggregate_status(report["checks"])
    if report["result"] == "PASS":
        (state_dir / LAST_PASS_MARKER).write_text(target + "\n", encoding="ascii")
        (state_dir / LOW_SPEC_PASS_MARKER).write_text(target + "\n", encoding="ascii")
        report["notes"].append(
            "Deterministic engineering gates, headless map/load smoke, and bounded low-spec startup/memory telemetry passed. Visual, interaction-feel, frame-time, and gameplay Product Reality remain human-only."
        )
    return report


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description="Run Everward private Windows verification without human interaction."
    )
    parser.add_argument(
        "--repo-root", type=Path, required=True, help="Dedicated Everward playtest checkout"
    )
    parser.add_argument("--state-dir", type=Path, default=None, help="Optional local evidence root")
    parser.add_argument("--unreal-root", default=None, help="Optional Unreal Engine 5.8 root")
    return parser


def main(argv: list[str] | None = None) -> int:
    args = build_parser().parse_args(argv)
    repo_root = args.repo_root.expanduser().resolve()
    state_dir = (args.state_dir or default_state_dir()).expanduser().resolve()
    lock_fd, acquired = acquire_lock(state_dir)
    if not acquired or lock_fd is None:
        busy = {
            "schema_version": REPORT_SCHEMA_VERSION,
            "started_at_utc": utc_now(),
            "completed_at_utc": utc_now(),
            "host": socket.gethostname(),
            "repo_root": str(repo_root),
            "result": "BUSY",
            "checks": {},
            "human_only_debt": [],
            "notes": ["Another unattended Everward worker owns the local lock."],
        }
        write_report(state_dir, busy, update_latest=False)
        return 0

    try:
        try:
            report = run_worker(repo_root, state_dir, args.unreal_root)
        except Exception as exc:  # defensive unattended boundary
            report = {
                "schema_version": REPORT_SCHEMA_VERSION,
                "started_at_utc": utc_now(),
                "completed_at_utc": utc_now(),
                "host": socket.gethostname(),
                "repo_root": str(repo_root),
                "result": "REVIEW_REQUIRED",
                "checks": {
                    "worker_exception": {
                        "status": "REVIEW_REQUIRED",
                        "reason": f"{type(exc).__name__}: {exc}",
                    }
                },
                "human_only_debt": [],
                "notes": ["The unattended worker failed closed; no Product Reality was inferred."],
            }
        write_report(state_dir, report)
        return 0
    finally:
        release_lock(state_dir, lock_fd)


if __name__ == "__main__":
    raise SystemExit(main())
