from __future__ import annotations

import argparse
import json
import os
import re
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
LOCK_NAME = "worker.lock"
REPORT_SCHEMA_VERSION = 1


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
        {
            "branch": "main",
            "status": "success",
            "event": "push",
            "per_page": "20",
        }
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


def run_capture(args: list[str], *, cwd: Path | None = None, timeout: float = 30.0) -> subprocess.CompletedProcess[str]:
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
            "Run tools/register_unattended_product_reality.ps1 from the dedicated playtest checkout first."
        )
    origin = git_output(repo_root, "remote", "get-url", "origin")
    if not origin_is_expected(origin):
        raise RuntimeError(f"Unexpected origin remote; refusing destructive sync: {origin}")


def process_running(image_name: str) -> bool:
    if os.name != "nt":
        return False
    proc = run_capture(["tasklist.exe", "/FI", f"IMAGENAME eq {image_name}", "/NH"], timeout=10.0)
    if proc.returncode != 0:
        return False
    return image_name.lower() in proc.stdout.lower()


def sync_to_latest_green(repo_root: Path) -> str:
    validate_dedicated_checkout(repo_root)
    if process_running("UnrealEditor.exe"):
        raise BlockingIOError("Unreal Editor is running; unattended checkout mutation is deferred.")

    target_sha = latest_successful_main_sha()
    git_output(repo_root, "fetch", "--prune", "origin")
    exists = run_capture(["git", "-C", str(repo_root), "cat-file", "-e", f"{target_sha}^{{commit}}"], timeout=30.0)
    if exists.returncode != 0:
        raise RuntimeError(f"Green commit {target_sha} was not present after fetch.")

    # This is intentionally destructive only after the dedicated-checkout sentinel
    # and expected origin have both been verified above.
    git_output(repo_root, "reset", "--hard")
    git_output(repo_root, "clean", "-fd")
    git_output(repo_root, "checkout", "--detach", target_sha)
    actual = git_output(repo_root, "rev-parse", "HEAD").lower()
    if actual != target_sha:
        raise RuntimeError(f"Checkout identity mismatch: expected {target_sha}, got {actual}")
    return target_sha


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
        [
            Path(r"C:\Program Files\Epic Games\UE_5.8"),
            Path(r"C:\Epic Games\UE_5.8"),
        ]
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
    raise RuntimeError("Unreal Engine 5.8 was not found by explicit path, environment, registry, or standard locations.")


def sanitize_text(text: str, *, repo_root: Path, unreal_root: Path | None = None) -> str:
    replacements = [
        (str(repo_root), "<REPO_ROOT>"),
        (str(Path.home()), "%USERPROFILE%"),
    ]
    if unreal_root is not None:
        replacements.append((str(unreal_root), "<UE_5_8_ROOT>"))
    result = text
    for raw, replacement in replacements:
        if raw:
            result = result.replace(raw, replacement)
            result = result.replace(raw.replace("\\", "/"), replacement)
    return result


def run_logged(
    args: list[str],
    *,
    cwd: Path,
    log_path: Path,
    timeout_seconds: float,
) -> tuple[int, float]:
    log_path.parent.mkdir(parents=True, exist_ok=True)
    started = time.monotonic()
    with log_path.open("w", encoding="utf-8", errors="replace") as handle:
        try:
            proc = subprocess.run(
                args,
                cwd=str(cwd),
                check=False,
                stdout=handle,
                stderr=subprocess.STDOUT,
                text=True,
                timeout=timeout_seconds,
            )
            code = int(proc.returncode)
        except subprocess.TimeoutExpired:
            handle.write(f"\nTIMEOUT after {timeout_seconds:.0f} seconds\n")
            code = 124
    return code, time.monotonic() - started


def log_tail(path: Path, *, repo_root: Path, unreal_root: Path | None = None, lines: int = 30) -> str:
    try:
        content = path.read_text(encoding="utf-8", errors="replace").splitlines()
    except OSError:
        return ""
    return sanitize_text("\n".join(content[-lines:]), repo_root=repo_root, unreal_root=unreal_root)


def run_full_preflight(repo_root: Path, log_path: Path) -> dict[str, Any]:
    script = repo_root / "tools" / "quality_preflight.py"
    if not script.is_file():
        return {"status": "REVIEW_REQUIRED", "reason": "tools/quality_preflight.py is missing"}
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

    command_line = subprocess.list2cmdline(
        [
            str(build_bat),
            "EverwardEditor",
            "Win64",
            "Development",
            str(uproject),
            "-WaitMutex",
            "-NoHotReloadFromIDE",
        ]
    )
    code, duration = run_logged(
        ["cmd.exe", "/d", "/s", "/c", command_line],
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
                raw = path.read_text(encoding="ascii").strip()
                pid = int(raw)
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


def read_last_pass(state_dir: Path) -> str | None:
    try:
        value = (state_dir / "last_passed_commit.txt").read_text(encoding="ascii").strip().lower()
    except OSError:
        return None
    return value if re.fullmatch(r"[0-9a-f]{40}", value) else None


def write_report(state_dir: Path, report: dict[str, Any], *, update_latest: bool = True) -> Path:
    history = state_dir / "history"
    history.mkdir(parents=True, exist_ok=True)
    stamp = datetime.now(timezone.utc).strftime("%Y%m%dT%H%M%S.%fZ")
    history_path = history / f"{stamp}.json"
    payload = json.dumps(report, indent=2, sort_keys=True) + "\n"
    history_path.write_text(payload, encoding="utf-8")
    if update_latest:
        (state_dir / "latest.json").write_text(payload, encoding="utf-8")
    return history_path


def run_worker(repo_root: Path, state_dir: Path, explicit_unreal_root: str | None = None) -> dict[str, Any]:
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
        report["checks"]["dedicated_checkout"] = {"status": "REVIEW_REQUIRED", "reason": str(exc)}
        report["completed_at_utc"] = utc_now()
        report["result"] = aggregate_status(report["checks"])
        return report

    if process_running("UnrealEditor.exe"):
        report["checks"]["editor_idle"] = {
            "status": "REVIEW_REQUIRED",
            "reason": "UnrealEditor.exe is running; unattended verification deferred without touching the checkout.",
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
        report["checks"]["green_main_discovery"] = {"status": "REVIEW_REQUIRED", "reason": str(exc)}
        report["completed_at_utc"] = utc_now()
        report["result"] = aggregate_status(report["checks"])
        return report

    if read_last_pass(state_dir) == target:
        report["tested_commit"] = target
        report["checks"]["cached_exact_commit"] = {
            "status": "PASS",
            "reason": "This exact Foundation-green commit already passed unattended full preflight and UBT build.",
        }
        report["notes"].append("No rebuild was needed; exact passed commit is unchanged.")
        report["completed_at_utc"] = utc_now()
        report["result"] = "PASS"
        return report

    try:
        synced = sync_to_latest_green(repo_root)
        report["tested_commit"] = synced
        report["checks"]["checkout_sync"] = {"status": "PASS", "commit": synced}
    except BlockingIOError as exc:
        report["checks"]["checkout_sync"] = {"status": "REVIEW_REQUIRED", "reason": str(exc)}
        report["completed_at_utc"] = utc_now()
        report["result"] = "REVIEW_REQUIRED"
        return report
    except Exception as exc:
        report["checks"]["checkout_sync"] = {"status": "REVIEW_REQUIRED", "reason": str(exc)}
        report["completed_at_utc"] = utc_now()
        report["result"] = aggregate_status(report["checks"])
        return report

    logs = state_dir / "logs" / target[:12]
    preflight = run_full_preflight(repo_root, logs / "full-preflight.log")
    report["checks"]["full_preflight"] = preflight
    if preflight.get("status") != "PASS":
        report["notes"].append("UBT build was not attempted because canonical full preflight did not pass.")
        report["completed_at_utc"] = utc_now()
        report["result"] = aggregate_status(report["checks"])
        return report

    try:
        unreal_root = resolve_unreal_root(explicit_unreal_root)
        report["checks"]["unreal_5_8"] = {"status": "PASS", "root": str(unreal_root)}
    except Exception as exc:
        report["checks"]["unreal_5_8"] = {"status": "REVIEW_REQUIRED", "reason": str(exc)}
        report["completed_at_utc"] = utc_now()
        report["result"] = aggregate_status(report["checks"])
        return report

    unreal_build = run_unreal_build(repo_root, unreal_root, logs / "unreal-build.log")
    report["checks"]["unreal_editor_build"] = unreal_build
    report["completed_at_utc"] = utc_now()
    report["result"] = aggregate_status(report["checks"])
    if report["result"] == "PASS":
        (state_dir / "last_passed_commit.txt").write_text(target + "\n", encoding="ascii")
        report["notes"].append(
            "Deterministic/headless engineering gates passed. Visual, interaction-feel, and gameplay Product Reality remain human-only."
        )
    return report


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description="Run Everward private Windows verification without human interaction.")
    parser.add_argument("--repo-root", type=Path, required=True, help="Dedicated Everward playtest checkout")
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
        # Scheduled runs always return zero. Evidence status is carried by latest.json;
        # a deterministic failure must not create an endless Task Scheduler retry storm.
        return 0
    finally:
        release_lock(state_dir, lock_fd)


if __name__ == "__main__":
    raise SystemExit(main())
