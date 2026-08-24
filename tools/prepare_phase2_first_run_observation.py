from __future__ import annotations

import argparse
import json
import platform
import subprocess
from datetime import datetime, timezone
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
TEMPLATE = ROOT / "playtests/phase2/first_run_observation.template.json"


def current_git_commit() -> str:
    result = subprocess.run(
        ["git", "-C", str(ROOT), "rev-parse", "HEAD"],
        check=True,
        capture_output=True,
        text=True,
    )
    return result.stdout.strip()


def prepare_observation(
    output: Path,
    *,
    git_commit: str,
    cpu: str = "",
    gpu: str = "",
) -> dict:
    data = json.loads(TEMPLATE.read_text(encoding="utf-8"))
    data["captured_at_utc"] = datetime.now(timezone.utc).strftime("%Y-%m-%dT%H:%M:%SZ")
    data["git_commit"] = git_commit
    data["unreal_engine_version"] = "5.8"
    data["host"]["os"] = platform.platform()
    data["host"]["cpu"] = cpu or platform.processor()
    data["host"]["gpu"] = gpu

    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(json.dumps(data, indent=2) + "\n", encoding="utf-8")
    return data


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Create a commit-stamped Phase 2 first-run observation from the canonical template."
    )
    parser.add_argument("--output", required=True, type=Path)
    parser.add_argument("--git-commit", default=None)
    parser.add_argument("--cpu", default="")
    parser.add_argument("--gpu", default="")
    args = parser.parse_args()

    git_commit = args.git_commit or current_git_commit()
    prepare_observation(args.output, git_commit=git_commit, cpu=args.cpu, gpu=args.gpu)
    print(args.output)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
