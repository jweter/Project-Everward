#!/usr/bin/env python3
"""Offline, read-only importer for a COMPLETED owner-run Aura Autopilot report.

Owner policy (docs/integrations/OWNER_RUN_AURA_REPORT_TO_ROADMAP.md, 2026-11-08): only Jeremy starts Aura. This tool is
the safe route for agents (Claude, ChatGPT, Iron God) to use a finished run's findings:

* it reads only the saved ``report.json`` and ``findings.md`` of one run directory; it never reads traces, media,
  harness scripts or account files, never contacts Aura or any network service, and never starts a process;
* it exports a whitelist of fields, with user paths, e-mail addresses and anything token-shaped redacted, so the
  result can be committed or attached to an issue;
* it refuses to write inside the Aura run directory.

Aura's own MCP server was checked and is not used: it lists ``start_run`` (a paid decision-model run) beside every
other tool, its read-only flags are client hints only, and it has no tool for reading completed reports.

Usage:
    python tools/playtest/aura_report_import.py --run-dir "%LOCALAPPDATA%\\Autopilot\\memory\\Everward\\runs\\<run>" \
        --out playtests/aura/<run>.sanitized.json
"""

from __future__ import annotations

import argparse
import hashlib
import json
import re
import sys
from pathlib import Path
from typing import Any

IMPORTER_VERSION = 1
# Only these report fields leave the laptop. Anything else (account data, indexing internals, usage by model) stays.
SCALAR_FIELDS = ("run", "project", "prompt", "outcome", "summary", "episodes", "seconds", "tool_seconds", "agent",
                 "model", "turns", "background_findings", "visual")
TEXT_LIST_FIELDS = ("findings", "evidence")

_REDACTIONS: tuple[tuple[re.Pattern[str], str], ...] = (
    (re.compile(r"(?i)[A-Z]:\\Users\\[^\\/\s\"']+"), r"%USERPROFILE%"),
    (re.compile(r"(?i)/(?:home|Users)/[^/\s\"']+"), r"~"),
    (re.compile(r"[A-Za-z0-9._%+-]+@[A-Za-z0-9.-]+\.[A-Za-z]{2,}"), "<email>"),
    (re.compile(r"(?i)\bbearer\s+[A-Za-z0-9._~+/=-]{8,}"), "Bearer <redacted>"),
    (re.compile(r"\b(?:sk|pk|rk|ghp|gho|xox[abp])[-_][A-Za-z0-9_-]{12,}"), "<redacted-token>"),
    (re.compile(r"(?i)\b(api[_-]?key|token|secret|password|session[_-]?id|account[_-]?id)\s*[:=]\s*\S+"), r"\1=<redacted>"),
    (re.compile(r"\b[0-9a-fA-F]{32,}\b"), "<redacted-hex>"),
)
# Long base64-looking runs are tokens only when they look random (many digits): source paths such as
# codex/source/Everward/ProbeSimulationAdapter are evidence and must survive.
_BLOB = re.compile(r"\b[A-Za-z0-9+/]{40,}={0,2}")


def _blob(match: re.Match[str]) -> str:
    value = match.group(0)
    return "<redacted-blob>" if sum(c.isdigit() for c in value) >= 6 else value


def sanitize(text: str) -> str:
    out = str(text)
    for pattern, replacement in _REDACTIONS:
        out = pattern.sub(replacement, out)
    return _BLOB.sub(_blob, out)


def _findings_md(path: Path) -> list[dict[str, str]]:
    """``## Finding N (context)`` sections of findings.md, sanitized."""
    if not path.is_file():
        return []
    entries: list[dict[str, str]] = []
    heading, body = None, []
    for line in path.read_text(encoding="utf-8").splitlines():
        if line.startswith("## "):
            if heading is not None:
                entries.append({"heading": sanitize(heading), "text": sanitize("\n".join(body).strip())})
            heading, body = line[3:].strip(), []
        elif heading is not None:
            body.append(line)
    if heading is not None:
        entries.append({"heading": sanitize(heading), "text": sanitize("\n".join(body).strip())})
    return entries


def import_run(run_dir: Path) -> dict[str, Any]:
    run_dir = Path(run_dir)
    report_path = run_dir / "report.json"
    if not report_path.is_file():
        raise FileNotFoundError(f"no completed Aura report in {run_dir.name}: report.json is missing")
    raw = report_path.read_bytes()
    report = json.loads(raw.decode("utf-8"))
    if not isinstance(report, dict):
        raise ValueError("report.json is not an object")

    out: dict[str, Any] = {
        "schema": "everward.aura_report.sanitized",
        "importer_version": IMPORTER_VERSION,
        "source": {"run_directory": sanitize(run_dir.name), "report_sha256": hashlib.sha256(raw).hexdigest()},
        "policy": "owner-run Aura only; imported offline and read-only; no Aura service was contacted",
    }
    for key in SCALAR_FIELDS:
        value = report.get(key)
        if key in report and (value is None or isinstance(value, (str, int, float, bool))):
            out[key] = sanitize(value) if isinstance(value, str) else value
    if isinstance(report.get("cost_usd"), (int, float)):
        out["estimated_cost_usd"] = round(float(report["cost_usd"]), 2)   # the report's estimate, not a billed amount
    for key in TEXT_LIST_FIELDS:
        values = report.get(key)
        if isinstance(values, list):
            out[key] = [sanitize(v) if isinstance(v, str) else sanitize(json.dumps(v, sort_keys=True)) for v in values]
    out["background_findings_md"] = _findings_md(run_dir / "findings.md")
    return out


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__.split("\n\n", 1)[0])
    parser.add_argument("--run-dir", required=True, type=Path)
    parser.add_argument("--out", type=Path, help="write the sanitized JSON here (default: stdout)")
    args = parser.parse_args(argv)
    run_dir = args.run_dir.resolve()
    result = import_run(run_dir)
    text = json.dumps(result, indent=2, sort_keys=True, ensure_ascii=False) + "\n"
    if args.out is None:
        sys.stdout.write(text)
        return 0
    out = args.out.resolve()
    if run_dir == out or run_dir in out.parents:
        parser.error("refusing to write inside the Aura run directory (it is the owner's evidence)")
    out.parent.mkdir(parents=True, exist_ok=True)
    out.write_text(text, encoding="utf-8")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
