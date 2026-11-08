"""Safely stage a pinned Aura Autopilot UE 5.8 plugin ONLY in a disposable clone.

Offline operation: no downloading, installing agent services, opening Unreal,
logging in, sending frames/source, writing API keys, or running third-party code.
Use --install --ack-isolated only in the explicitly named isolated checkout.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import os
from pathlib import Path, PurePosixPath
import re
import shutil
import stat
import subprocess
import sys
import tempfile
import zipfile

MANIFEST = Path(__file__).with_name("aura_autopilot_release.json")
REPO = "https://github.com/jweter/Project-Everward"
REQUIRED_BRANCH = "pilot/aura-autopilot-ue58-sandbox-20261108"
MAX_UNCOMPRESSED_BYTES = 700_000_000
MAX_ENTRIES = 3000


def git(root: Path, *args: str) -> str:
    return subprocess.check_output(
        ["git", "-C", str(root), *args],
        text=True, encoding="utf-8", stderr=subprocess.DEVNULL
    ).strip()


def validate_clone(root: Path, project: Path, *, strict_branch: bool = True) -> None:
    root = root.resolve()
    project = project.resolve()
    if root.parent.name != "Everward-Aura-Pilot" or root.name != "Project-Everward":
        raise ValueError("Refusing: not the dedicated Documents/Everward-Aura-Pilot/Project-Everward clone")
    if project != root / "unreal" / "Everward.uproject" or not project.is_file():
        raise ValueError("Refusing: Everward.uproject is not inside the dedicated sandbox clone")
    if not (root / ".git").is_dir():
        raise ValueError("Refusing: a separate standalone Git clone is required")
    origin = git(root, "remote", "get-url", "origin").replace("git@github.com:", "https://github.com/")
    if origin.lower().removesuffix(".git").rstrip("/") != REPO.lower():
        raise ValueError("Refusing: clone origin is not Everward")
    if strict_branch and git(root, "branch", "--show-current") != REQUIRED_BRANCH:
        raise ValueError("Refusing: clone must be on the dedicated Aura pilot branch")
    if git(root, "status", "--porcelain"):
        raise ValueError("Refusing: cloned project must be clean before plugin staging")
    if (root / ".git" / "everward-unattended-worker").exists():
        raise ValueError("Refusing: unattended production worker sentinel found")


def verify_archive(path: Path, manifest: dict[str, object]) -> dict[str, object]:
    if path.name != manifest.get("archive_filename"):
        raise ValueError("Wrong Aura release archive filename")
    if not path.is_file() or path.stat().st_size != manifest["archive_bytes"]:
        raise ValueError("Aura archive missing or release-size mismatch")
    with path.open("rb") as stream:
        actual_hash = hashlib.file_digest(stream, "sha256").hexdigest()
    if actual_hash != manifest["sha256"]:
        raise ValueError("Aura archive SHA256 does not match pinned official release")
    with zipfile.ZipFile(path) as archive:
        entries = archive.infolist()
        expanded = sum(item.file_size for item in entries)
        if len(entries) > MAX_ENTRIES or expanded > MAX_UNCOMPRESSED_BYTES:
            raise ValueError("Unsafe Aura ZIP entry/expanded size limit")
        for item in entries:
            name = item.filename
            parts = PurePosixPath(name).parts
            mode = item.external_attr >> 16
            if (not name.startswith("Autopilot/") or "\\" in name or
                not parts or parts[0] != "Autopilot" or ".." in parts or
                any(part in ("", ".") for part in name.rstrip("/").split("/")) or
                re.match(r"^[a-zA-Z]:", name) or stat.S_ISLNK(mode)):
                raise ValueError("Unsafe path or symlink in Aura ZIP")
        required = {"Autopilot/Autopilot.uplugin", "Autopilot/Binaries/Win64/UnrealEditor-AutopilotPlaytest.dll"}
        if not required.issubset(set(archive.namelist())):
            raise ValueError("Aura ZIP is missing expected UE5.8 plugin binaries")
        plugin = json.loads(archive.read("Autopilot/Autopilot.uplugin"))
        if plugin.get("FriendlyName") != "Autopilot" or plugin.get("VersionName") != "1.0.0":
            raise ValueError("Unexpected plugin identity/version")
        return {"sha256": actual_hash, "archive_bytes": path.stat().st_size,
                "expanded_bytes": expanded, "entries": len(entries), "plugin_version": "1.0.0"}


def is_editor_running() -> bool:
    if os.name != "nt":
        return False
    p = subprocess.run(["tasklist", "/FI", "IMAGENAME eq UnrealEditor.exe", "/NH"],
                       capture_output=True, text=True, check=False)
    return "UnrealEditor.exe" in p.stdout


def stage(root: Path, archive_path: Path, manifest: dict[str, object]) -> dict[str, object]:
    root = root.resolve()
    project = root / "unreal" / "Everward.uproject"
    validate_clone(root, project)
    checked = verify_archive(archive_path, manifest)
    if is_editor_running():
        raise ValueError("An Unreal Editor process is running; refuse plugin modification")
    plugins = root / "unreal" / "Plugins"
    installed = plugins / "Autopilot"
    if installed.exists() or installed.is_symlink():
        raise ValueError("Autopilot folder already exists; refusing overwrite")
    descriptor = json.loads(project.read_text(encoding="utf-8"))
    if descriptor.get("EngineAssociation") != "5.8" or not isinstance(descriptor.get("Modules"), list):
        raise ValueError("Not an Everward UE5.8 project descriptor")
    declared = descriptor.get("Plugins", [])
    if not isinstance(declared, list) or any(
        not isinstance(entry, dict) or not isinstance(entry.get("Name"), str) for entry in declared
    ):
        raise ValueError("Malformed Unreal plugin list")
    if any(entry["Name"] == "Autopilot" for entry in declared):
        raise ValueError("Autopilot already declared; refusing overwrite")
    updated = dict(descriptor)
    updated["Plugins"] = [*declared, {"Name": "Autopilot", "Enabled": True}]
    plugins.mkdir(exist_ok=True)
    with tempfile.TemporaryDirectory(prefix=".aura-staging-", dir=plugins) as temp:
        staging = Path(temp)
        with zipfile.ZipFile(archive_path) as zipped:
            zipped.extractall(staging)
        if not (staging / "Autopilot" / "Autopilot.uplugin").is_file():
            raise ValueError("Extracted plugin manifest missing")
        shutil.move(str(staging / "Autopilot"), str(installed))
    try:
        temporary = project.with_suffix(".uproject.aura-temp")
        temporary.write_text(json.dumps(updated, indent=2) + "\n", encoding="utf-8")
        os.replace(temporary, project)
    except BaseException:
        shutil.rmtree(installed)
        raise
    return {**checked, "sandbox": str(root), "installed_plugin": str(installed),
            "editor_loaded": False, "agent_service_started": False,
            "playtest_executed": False, "provider_account_configured": False}


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--archive", type=Path, required=True,
                        help="Already downloaded, pinned official Win64 UE5.8 release ZIP")
    parser.add_argument("--project-root", type=Path, required=True)
    parser.add_argument("--install", action="store_true",
                        help="Apply only to clean dedicated sandbox clone")
    parser.add_argument("--ack-isolated", action="store_true")
    args = parser.parse_args(argv)
    try:
        release = json.loads(MANIFEST.read_text(encoding="utf-8"))
        archive = args.archive.resolve()
        checked = verify_archive(archive, release)
        project_root = args.project_root.resolve()
        if args.install:
            if not args.ack_isolated:
                raise ValueError("Install requires --ack-isolated")
            result = stage(project_root, archive, release)
        else:
            result = {**checked, "dry_run": True, "no_files_modified": True}
        print(json.dumps(result, indent=2, sort_keys=True))
        return 0
    except (OSError, ValueError, KeyError, TypeError, subprocess.CalledProcessError,
            json.JSONDecodeError, zipfile.BadZipFile) as exc:
        print(f"REVIEW_REQUIRED: {exc}", file=sys.stderr)
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
