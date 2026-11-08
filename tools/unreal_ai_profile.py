"""Gated UE5.8 editor AI pilot: inspect, or stage plugins in a disposable Git worktree.

This tool never starts Unreal, opens a port, sets API keys, or indexes assets.
It refuses to modify the primary checkout or a dirty project descriptor.
"""
from __future__ import annotations

import argparse
import json
import pathlib
import subprocess
import sys
from collections.abc import Mapping

PLUGIN_NAMES = (
    "PCG",
    "ModelContextProtocol",
    "ToolsetRegistry",
    "PCGToolset",
    "SemanticSearch",
)


def proposed_descriptor(existing: Mapping[str, object]) -> dict[str, object]:
    """Return an opt-in sandbox descriptor with five editor feature plugins enabled."""
    if not isinstance(existing.get("Modules"), list):
        raise ValueError("Not a valid Unreal project descriptor: Modules is missing")
    result = dict(existing)
    plugins = existing.get("Plugins", [])
    if not isinstance(plugins, list):
        raise ValueError("Not a valid Unreal project descriptor: Plugins must be a list")
    entries: list[dict[str, object]] = []
    seen: set[str] = set()
    for raw in plugins:
        if not isinstance(raw, dict) or not isinstance(raw.get("Name"), str):
            raise ValueError("Malformed Unreal plugin declaration")
        entry = dict(raw)
        name = str(entry["Name"])
        if name in seen:
            raise ValueError(f"Duplicate plugin declaration: {name}")
        seen.add(name)
        if name in PLUGIN_NAMES:
            entry["Enabled"] = True
        entries.append(entry)
    for name in PLUGIN_NAMES:
        if name not in seen:
            entries.append({"Name": name, "Enabled": True})
    result["Plugins"] = entries
    return result


def git_stdout(workdir: pathlib.Path, *args: str) -> str:
    return subprocess.check_output(
        ["git", "-C", str(workdir), *args],
        text=True,
        stderr=subprocess.DEVNULL,
    ).strip()


def require_linked_isolated_worktree(project: pathlib.Path) -> None:
    """Refuse the primary/main checkout even when it is on a non-main branch."""
    parent = project.parent.resolve()
    root = pathlib.Path(git_stdout(parent, "rev-parse", "--show-toplevel")).resolve()
    branch = git_stdout(root, "symbolic-ref", "--quiet", "--short", "HEAD")
    if branch in ("main", "master"):
        raise ValueError("Refusing to stage the Unreal AI pilot on main/master")
    lines = git_stdout(root, "worktree", "list", "--porcelain").splitlines()
    paths = [pathlib.Path(line[9:]).resolve() for line in lines if line.startswith("worktree ")]
    if len(paths) < 2 or not paths or root == paths[0]:
        raise ValueError("Refusing: use a *linked disposable worktree*, not the primary checkout")
    if root not in paths or not project.is_relative_to(root):
        raise ValueError("Project must be inside the current linked worktree")
    rel = str(project.relative_to(root))
    dirty = git_stdout(root, "status", "--porcelain", "--", rel)
    if dirty:
        raise ValueError(f"Refusing to overwrite an edited project descriptor: {rel}")
    if project.name != "Everward.uproject" or project.parent.name != "unreal":
        raise ValueError("Expected the isolated worktree's unreal/Everward.uproject only")


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--project", type=pathlib.Path, default=pathlib.Path("unreal/Everward.uproject"))
    parser.add_argument("--mode", choices=("inspect", "stage"), default="inspect")
    parser.add_argument("--ack-isolated-worktree", action="store_true")
    args = parser.parse_args(argv)
    project = args.project.resolve()
    if not project.is_file():
        parser.error(f"Project descriptor does not exist: {project}")
    try:
        existing = json.loads(project.read_text(encoding="utf-8"))
        if not isinstance(existing, dict):
            raise ValueError("Project descriptor must be a JSON object")
        preview = proposed_descriptor(existing)
        if args.mode == "inspect":
            installed = {
                entry["Name"]: entry.get("Enabled")
                for entry in existing.get("Plugins", [])
                if isinstance(entry, dict) and "Name" in entry
            }
            print(json.dumps({
                "project": str(project),
                "mode": "inspect-only",
                "current_plugin_declarations": {name: installed.get(name, "unspecified") for name in PLUGIN_NAMES},
                "proposed_plugin_names": list(PLUGIN_NAMES),
                "unreal_editor_live_verified": False,
                "semantic_provider_configured": False,
                "server_started": False,
            }, indent=2))
            return 0
        if not args.ack_isolated_worktree:
            raise ValueError("Stage requires --ack-isolated-worktree and a linked non-primary worktree")
        require_linked_isolated_worktree(project)
        project.write_text(json.dumps(preview, indent=2) + "\n", encoding="utf-8")
        print("Staged plugin declarations in disposable worktree only. No server or provider started.")
        return 0
    except (OSError, subprocess.CalledProcessError, ValueError, json.JSONDecodeError) as exc:
        print(f"REVIEW_REQUIRED: {exc}", file=sys.stderr)
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
