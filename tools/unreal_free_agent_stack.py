"""Free-only UE 5.8 agent utilities for Everward.

Read-only inspection and MCP discovery are safe defaults. Native commands are
opt-in, isolated-worktree operations; this does not grant Iron God authority to
start an editor, launch paid Aura, merge PRs, or promote Product Reality.
"""
from __future__ import annotations

import argparse
import ctypes
import hashlib
import json
import os
import pathlib
import re
import shutil
import subprocess
import sys
import tempfile
import time
import urllib.error
import urllib.request
from datetime import datetime, timezone
from typing import Any

UPSTREAM = "https://github.com/quodsoler/unreal-engine-skills"
PINNED_SHA = "f3742d7b688690810df369802b90430324e380b9"
EXPECTED_SKILLS = frozenset((
    "ue-actor-component-architecture", "ue-ai-navigation", "ue-animation-system",
    "ue-async-threading", "ue-audio-system", "ue-blueprint-cpp-interop",
    "ue-character-movement", "ue-cpp-foundations", "ue-data-assets-tables",
    "ue-editor-tools", "ue-game-features", "ue-gameplay-abilities",
    "ue-gameplay-cameras", "ue-gameplay-framework", "ue-gameplay-tags-messaging",
    "ue-input-system", "ue-mass-entity", "ue-materials-rendering",
    "ue-module-build-system", "ue-mover", "ue-networking-replication",
    "ue-niagara-effects", "ue-physics-collision", "ue-procedural-generation",
    "ue-project-context", "ue-sequencer-cinematics", "ue-serialization-savegames",
    "ue-state-trees", "ue-testing-debugging", "ue-ui-umg-slate",
    "ue-world-level-streaming",
))
MCP_URL = "http://127.0.0.1:8000/mcp"
KINDS = ("ubt", "automation", "gauntlet", "package")
MIN_AVAILABLE_GIB = 7.0
MIN_FREE_DISK_GIB = 10.0


def skills_inventory(root: pathlib.Path) -> dict[str, Any]:
    skill_root = root / ".agents" / "skills"
    installed = {p.name for p in skill_root.glob("ue-*") if p.is_dir()}
    missing = sorted(EXPECTED_SKILLS - installed)
    unexpected = sorted(installed - EXPECTED_SKILLS)
    invalid = []
    for name in sorted(EXPECTED_SKILLS & installed):
        folder = skill_root / name
        if folder.is_symlink() or not (folder / "SKILL.md").is_file():
            invalid.append(name)
            continue
        for item in folder.rglob("*"):
            if item.is_symlink() or (item.is_file() and item.suffix.lower() != ".md"):
                invalid.append(name)
                break
    license_ok = (skill_root / "QUODSOLER-MIT-LICENSE.txt").is_file()
    lock_valid = False
    lock_path = skill_root / "UPSTREAM_LOCK.json"
    if lock_path.is_file():
        try:
            manifest = json.loads(lock_path.read_text(encoding="utf-8"))
            hashed = {
                p.relative_to(skill_root).as_posix(): hashlib.sha256(p.read_bytes().replace(b"\r\n", b"\n")).hexdigest()
                for p in skill_root.glob("ue-*/**/*.md") if p.is_file() and not p.is_symlink()
            }
            if license_ok:
                hashed["QUODSOLER-MIT-LICENSE.txt"] = hashlib.sha256(
                    (skill_root / "QUODSOLER-MIT-LICENSE.txt").read_bytes().replace(b"\r\n", b"\n")).hexdigest()
            lock_valid = (
                manifest.get("commit") == PINNED_SHA
                and manifest.get("files") == hashed
            )
        except (OSError, ValueError, json.JSONDecodeError):
            lock_valid = False
    return {
        "source": UPSTREAM, "pinned_commit": PINNED_SHA,
        "skill_count": len(installed), "expected_count": len(EXPECTED_SKILLS),
        "missing": missing, "unexpected": unexpected, "invalid": invalid,
        "license_present": license_ok, "lock_verified": lock_valid,
        "ready": not (missing or unexpected or invalid) and license_ok and lock_valid,
    }


def unreal_paths(engine_root: pathlib.Path, project: pathlib.Path) -> dict[str, pathlib.Path]:
    return {
        "editor": engine_root / "Engine/Binaries/Win64/UnrealEditor-Cmd.exe",
        "ubt": engine_root / "Engine/Build/BatchFiles/Build.bat",
        "uat": engine_root / "Engine/Build/BatchFiles/RunUAT.bat",
        "project": project,
    }


def native_command(kind: str, paths: dict[str, pathlib.Path],
                   evidence_dir: pathlib.Path, *, test: str = "") -> list[str]:
    if kind not in KINDS:
        raise ValueError("Unsupported free native test kind")
    project = str(paths["project"])
    if kind == "ubt":
        return [str(paths["ubt"]), "EverwardEditor", "Win64", "Development",
                "-Project=" + project, "-WaitMutex", "-NoHotReload"]
    if kind == "automation":
        # The test prefix must be a declared native test, not an arbitrary console command.
        prefix = test or "Everward"
        if not re.fullmatch(r"[A-Za-z0-9_.]{1,100}", prefix):
            raise ValueError("Automation test prefix must be an identifier")
        return [str(paths["editor"]), project, "-unattended", "-nop4", "-NullRHI",
                "-nosplash", "-ExecCmds=Automation RunTests " + prefix + "; Quit",
                "-TestExit=Automation Test Queue Empty",
                "-ReportOutputPath=" + str(evidence_dir)]
    if kind == "gauntlet":
        if not re.fullmatch(r"[A-Za-z0-9_.]{1,100}", test):
            raise ValueError("Gauntlet requires an explicitly named installed test")
        return [str(paths["uat"]), "RunUnreal", "-project=" + project,
                "-platform=Win64", "-configuration=Development", "-test=" + test]
    return [str(paths["uat"]), "BuildCookRun", "-project=" + project,
            "-noP4", "-platform=Win64", "-clientconfig=Development",
            "-build", "-cook", "-stage", "-pak", "-archive",
            "-archivedirectory=" + str(evidence_dir / "package")]


def require_isolated_checkout(root: pathlib.Path) -> str:
    root = root.resolve()
    top = pathlib.Path(subprocess.check_output(
        ["git", "-C", str(root), "rev-parse", "--show-toplevel"],
        text=True, stderr=subprocess.DEVNULL).strip()).resolve()
    if top != root:
        raise ValueError("Expected the exact repository root")
    branch = subprocess.check_output(
        ["git", "-C", str(root), "branch", "--show-current"],
        text=True, stderr=subprocess.DEVNULL).strip()
    if not branch or branch in {"main", "master"}:
        raise ValueError("Do not run native writes on main or a detached HEAD")
    worktrees = subprocess.check_output(
        ["git", "-C", str(root), "worktree", "list", "--porcelain"],
        text=True, stderr=subprocess.DEVNULL).splitlines()
    locations = [pathlib.Path(line[9:]).resolve() for line in worktrees
                 if line.startswith("worktree ")]
    if len(locations) < 2 or root == locations[0] or root not in locations:
        raise ValueError("Native tests require a linked isolated worktree")
    dirty = subprocess.check_output(
        ["git", "-C", str(root), "status", "--porcelain"],
        text=True, stderr=subprocess.DEVNULL).strip()
    if dirty:
        raise ValueError("Uncommitted changes; preserve owner and worker files")
    return subprocess.check_output(
        ["git", "-C", str(root), "rev-parse", "HEAD"],
        text=True, stderr=subprocess.DEVNULL).strip()


def resource_status(drive: pathlib.Path) -> dict[str, Any]:
    free_disk_gib = shutil.disk_usage(drive).free / 2**30
    available_gib = None
    if sys.platform == "win32":
        class MemoryStatus(ctypes.Structure):
            _fields_ = [
                ("dwLength", ctypes.c_ulong), ("dwMemoryLoad", ctypes.c_ulong),
                ("ullTotalPhys", ctypes.c_ulonglong), ("ullAvailPhys", ctypes.c_ulonglong),
                ("ullTotalPageFile", ctypes.c_ulonglong), ("ullAvailPageFile", ctypes.c_ulonglong),
                ("ullTotalVirtual", ctypes.c_ulonglong), ("ullAvailVirtual", ctypes.c_ulonglong),
                ("ullAvailExtendedVirtual", ctypes.c_ulonglong),
            ]
        memory = MemoryStatus()
        memory.dwLength = ctypes.sizeof(memory)
        if not ctypes.windll.kernel32.GlobalMemoryStatusEx(ctypes.byref(memory)):
            raise OSError("Cannot measure physical memory")
        available_gib = memory.ullAvailPhys / 2**30
    return {
        "free_disk_gib": round(free_disk_gib, 2),
        "available_memory_gib": None if available_gib is None else round(available_gib, 2),
        "admitted": free_disk_gib >= MIN_FREE_DISK_GIB and
                    available_gib is not None and available_gib >= MIN_AVAILABLE_GIB,
    }


def _parse_mcp_body(data: bytes, content_type: str) -> dict[str, Any]:
    body = data.decode("utf-8")
    if "text/event-stream" in content_type:
        lines = [line[5:].strip() for line in body.splitlines()
                 if line.startswith("data:")]
        if not lines:
            raise ValueError("MCP SSE returned no data")
        body = lines[-1]
    reply = json.loads(body)
    if not isinstance(reply, dict):
        raise ValueError("MCP response was not a JSON object")
    return reply


def mcp_discover(*, url: str = MCP_URL, timeout: float = 3.0,
                 opener: Any = urllib.request.urlopen) -> dict[str, Any]:
    """Only MCP initialize, initialized notification and tools/list.

    No call_tool or editor mutation; no server startup; no paid transport.
    """
    if url != MCP_URL:
        raise ValueError("Only the approved localhost Unreal MCP endpoint is allowed")
    session = None
    def exchange(method: str, params: dict[str, Any] | None, seq: int | None):
        nonlocal session
        payload: dict[str, Any] = {"jsonrpc": "2.0", "method": method}
        if seq is not None:
            payload["id"] = seq
        if params is not None:
            payload["params"] = params
        headers = {"Content-Type": "application/json",
                   "Accept": "application/json, text/event-stream",
                   "MCP-Protocol-Version": "2025-06-18"}
        if session:
            headers["Mcp-Session-Id"] = session
        req = urllib.request.Request(
            url, data=json.dumps(payload).encode(), headers=headers, method="POST")
        with opener(req, timeout=timeout) as response:
            session = response.headers.get("Mcp-Session-Id", session)
            data = response.read(2 * 1024 * 1024 + 1)
            if len(data) > 2 * 1024 * 1024:
                raise ValueError("MCP response too large")
            if seq is None:
                return {}
            return _parse_mcp_body(data, response.headers.get("Content-Type", ""))
    hello = exchange("initialize", {
        "protocolVersion": "2025-06-18", "capabilities": {},
        "clientInfo": {"name": "everward-readonly-probe", "version": "1.0"},
    }, 1)
    if "error" in hello or "result" not in hello:
        raise ValueError("MCP initialize failed")
    exchange("notifications/initialized", {}, None)
    listing = exchange("tools/list", {}, 2)
    if "error" in listing or not isinstance(listing.get("result", {}).get("tools"), list):
        raise ValueError("MCP tool discovery failed")
    return {"status": "CONNECTED_READ_ONLY", "tools": sorted(
        x.get("name", "") for x in listing["result"]["tools"] if isinstance(x, dict)),
        "server": hello["result"].get("serverInfo", {}),
        "editor_test_executed": False, "write_executed": False}


def run_native(kind: str, root: pathlib.Path, engine: pathlib.Path,
               project: pathlib.Path, evidence_dir: pathlib.Path,
               *, test: str, timeout_seconds: int = 900) -> dict[str, Any]:
    sha = require_isolated_checkout(root)
    if project.resolve() != (root / "unreal/Everward.uproject").resolve():
        raise ValueError("Only Everward in the isolated checkout is supported")
    status = resource_status(root)
    if not status["admitted"]:
        return {"status": "RESOURCE_WAIT", "resources": status, "head": sha}
    paths = unreal_paths(engine, project)
    command = native_command(kind, paths, evidence_dir, test=test)
    for key in ("project", "ubt" if kind == "ubt" else
                "editor" if kind == "automation" else "uat"):
        if not paths[key].is_file():
            raise ValueError("Missing required engine/project component: " + key)
    if evidence_dir.exists() and any(evidence_dir.iterdir()):
        raise ValueError("Evidence directory must be empty for each run")
    evidence_dir.mkdir(parents=True, exist_ok=True)
    # Exclusive lock: refuse concurrent sessions; never unlink someone else's lease.
    lock_path = pathlib.Path(tempfile.gettempdir()) / "everward-native-agent.lock"
    lock_fd = os.open(str(lock_path), os.O_CREAT | os.O_EXCL | os.O_WRONLY, 0o600)
    try:
        os.write(lock_fd, json.dumps({"pid": os.getpid(), "head": sha}).encode())
        os.close(lock_fd)
        lock_fd = -1
        started = datetime.now(timezone.utc).isoformat()
        try:
            proc = subprocess.run(command, cwd=root, capture_output=True,
                                  text=True, timeout=timeout_seconds, shell=False)
            exit_code = proc.returncode
            log = ((proc.stdout or "") + "\n" + (proc.stderr or ""))[-16000:]
        except subprocess.TimeoutExpired:
            exit_code = None
            log = "Native command timed out"
        (evidence_dir / "native.log").write_text(log, encoding="utf-8")
        # A zero process code alone does NOT prove that a gameplay test passed.
        report = list(evidence_dir.rglob("index.json")) + list(evidence_dir.rglob("Index.json"))
        result = {"status": "FAIL" if exit_code not in (0, None) else
                  "ENVIRONMENT_FAILURE" if exit_code is None else "REVIEW_REQUIRED",
                  "head": sha, "kind": kind, "exit_code": exit_code,
                  "started_utc": started, "finished_utc": datetime.now(timezone.utc).isoformat(),
                  "command": command, "resources": status,
                  "automation_report_found": bool(report), "paid_tools_used": False,
                  "merge_authority": False, "product_reality_verified": False}
        (evidence_dir / "result.json").write_text(
            json.dumps(result, indent=2), encoding="utf-8")
        return result
    finally:
        if lock_fd >= 0:
            os.close(lock_fd)
        lock_path.unlink(missing_ok=True)


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--mode", choices=("inspect", "mcp-read", "plan", "run-native"),
                        default="inspect")
    parser.add_argument("--root", type=pathlib.Path, default=pathlib.Path.cwd())
    parser.add_argument("--engine", type=pathlib.Path,
                        default=pathlib.Path(r"C:\Program Files\Epic Games\UE_5.8"))
    parser.add_argument("--kind", choices=KINDS, default="automation")
    parser.add_argument("--test", default="")
    parser.add_argument("--evidence-dir", type=pathlib.Path, default=None)
    parser.add_argument("--ack-isolated-worktree", action="store_true")
    args = parser.parse_args(argv)
    root = args.root.resolve()
    evidence = args.evidence_dir or (root / "unreal/Saved/AgentEvidence")
    project = root / "unreal/Everward.uproject"
    try:
        if args.mode == "inspect":
            result = {"mode": "inspect", "skills": skills_inventory(root),
                      "project_present": project.is_file(), "engine_version_target": "5.8",
                      "mcp_started": False, "paid_integrations_enabled": False}
        elif args.mode == "mcp-read":
            result = mcp_discover()
        elif args.mode == "plan":
            result = {"mode": "plan", "kind": args.kind,
                      "command": native_command(args.kind, unreal_paths(args.engine, project),
                                                evidence, test=args.test),
                      "executed": False, "paid_integrations": False}
        else:
            if not args.ack_isolated_worktree:
                raise ValueError("Native execution needs --ack-isolated-worktree")
            result = run_native(args.kind, root, args.engine, project, evidence,
                                test=args.test)
        print(json.dumps(result, indent=2))
        return 0 if result.get("status") not in {"FAIL", "ENVIRONMENT_FAILURE"} else 2
    except (OSError, ValueError, subprocess.CalledProcessError,
            urllib.error.URLError, json.JSONDecodeError) as exc:
        print(json.dumps({"status": "REVIEW_REQUIRED", "reason": str(exc)}))
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
