"""Local/private ingestion for explicit Everward mobile Product Reality verdicts.

This module records a human PASS/FAIL/FLAG against already-generated authoritative
mobile evidence.  It never derives a verdict, mutates simulation/save state, or
converts a phone review into Unreal acceptance.
"""

from __future__ import annotations

import argparse
import hashlib
import json
from datetime import datetime, timezone
from pathlib import Path
from typing import Any, Mapping

_ALLOWED_RESULTS = {"PASS", "FAIL", "FLAG"}
_SCHEMA_VERSION = 1


def build_review_result(
    evidence: Mapping[str, Any],
    *,
    result: str,
    reviewed_at_utc: str,
    notes: str = "",
) -> dict[str, Any]:
    """Bind an explicit human verdict to authoritative mobile evidence identity."""

    commit = _required_text(evidence, "commit")
    scenario = _required_text(evidence, "scenario")
    seed = _required_text(evidence, "seed")

    normalized_result = result.strip().upper()
    if normalized_result not in _ALLOWED_RESULTS:
        raise ValueError("result must be PASS, FAIL, or FLAG")

    normalized_timestamp = _normalize_utc_timestamp(reviewed_at_utc)
    normalized_notes = notes.strip()
    identity = {
        "commit": commit,
        "scenario": scenario,
        "seed": seed,
    }
    identity_sha256 = hashlib.sha256(
        json.dumps(identity, sort_keys=True, separators=(",", ":")).encode("utf-8")
    ).hexdigest()

    return {
        "schema_version": _SCHEMA_VERSION,
        "evidence_identity_sha256": identity_sha256,
        **identity,
        "result": normalized_result,
        "reviewed_at_utc": normalized_timestamp,
        "notes": normalized_notes,
        "acceptance_scope": "mobile_deterministic_evidence_only",
        "unreal_product_reality_cleared": False,
    }


def append_review_result(path: Path, record: Mapping[str, Any]) -> None:
    """Append one validated review result to a caller-selected private JSONL file."""

    validated = _validate_record(record)
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("a", encoding="utf-8", newline="\n") as handle:
        handle.write(json.dumps(validated, sort_keys=True, separators=(",", ":")))
        handle.write("\n")
        handle.flush()


def load_review_results(path: Path) -> list[dict[str, Any]]:
    """Read append-only local review history, failing closed on malformed rows."""

    records: list[dict[str, Any]] = []
    with path.open("r", encoding="utf-8") as handle:
        for line_number, line in enumerate(handle, start=1):
            if not line.strip():
                continue
            try:
                raw = json.loads(line)
            except json.JSONDecodeError as exc:
                raise ValueError(f"invalid review JSONL row {line_number}") from exc
            if not isinstance(raw, Mapping):
                raise ValueError(f"invalid review JSONL row {line_number}")
            records.append(_validate_record(raw))
    return records


def _validate_record(record: Mapping[str, Any]) -> dict[str, Any]:
    schema_version = record.get("schema_version")
    if schema_version != _SCHEMA_VERSION:
        raise ValueError(f"unsupported review schema version: {schema_version!r}")

    commit = _required_text(record, "commit")
    scenario = _required_text(record, "scenario")
    seed = _required_text(record, "seed")
    result = _required_text(record, "result")
    if result not in _ALLOWED_RESULTS:
        raise ValueError("result must be PASS, FAIL, or FLAG")
    reviewed_at_utc = _normalize_utc_timestamp(_required_text(record, "reviewed_at_utc"))

    expected_identity_sha = hashlib.sha256(
        json.dumps(
            {"commit": commit, "scenario": scenario, "seed": seed},
            sort_keys=True,
            separators=(",", ":"),
        ).encode("utf-8")
    ).hexdigest()
    supplied_identity_sha = _required_text(record, "evidence_identity_sha256")
    if supplied_identity_sha != expected_identity_sha:
        raise ValueError("review evidence identity hash does not match bound evidence")

    if record.get("acceptance_scope") != "mobile_deterministic_evidence_only":
        raise ValueError("unsupported review acceptance scope")
    if record.get("unreal_product_reality_cleared") is not False:
        raise ValueError("mobile review must not clear Unreal Product Reality")

    notes = record.get("notes", "")
    if not isinstance(notes, str):
        raise ValueError("review notes must be text")

    return {
        "schema_version": _SCHEMA_VERSION,
        "evidence_identity_sha256": supplied_identity_sha,
        "commit": commit,
        "scenario": scenario,
        "seed": seed,
        "result": result,
        "reviewed_at_utc": reviewed_at_utc,
        "notes": notes.strip(),
        "acceptance_scope": "mobile_deterministic_evidence_only",
        "unreal_product_reality_cleared": False,
    }


def _normalize_utc_timestamp(value: str) -> str:
    normalized = value.strip()
    if not normalized:
        raise ValueError("reviewed_at_utc must not be empty")
    candidate = normalized[:-1] + "+00:00" if normalized.endswith("Z") else normalized
    try:
        parsed = datetime.fromisoformat(candidate)
    except ValueError as exc:
        raise ValueError("reviewed_at_utc must be an ISO-8601 timestamp") from exc
    if parsed.tzinfo is None or parsed.utcoffset() is None:
        raise ValueError("reviewed_at_utc must include a timezone")
    parsed_utc = parsed.astimezone(timezone.utc)
    return parsed_utc.isoformat(timespec="seconds").replace("+00:00", "Z")


def _required_text(data: Mapping[str, Any], key: str) -> str:
    value = data.get(key)
    if value is None or isinstance(value, bool) or not isinstance(value, (str, int)):
        raise ValueError(f"invalid required field: {key}")
    normalized = str(value).strip()
    if not normalized:
        raise ValueError(f"missing required field: {key}")
    return normalized


def _main() -> int:
    parser = argparse.ArgumentParser(
        description="Append a human PASS/FAIL/FLAG to a private Everward mobile-review JSONL history."
    )
    parser.add_argument("--evidence", required=True, type=Path)
    parser.add_argument("--result", required=True, choices=sorted(_ALLOWED_RESULTS))
    parser.add_argument("--reviewed-at-utc", required=True)
    parser.add_argument("--output", required=True, type=Path)
    parser.add_argument("--notes", default="")
    args = parser.parse_args()

    raw = json.loads(args.evidence.read_text(encoding="utf-8"))
    if not isinstance(raw, Mapping):
        raise ValueError("evidence root must be a mapping")
    record = build_review_result(
        raw,
        result=args.result,
        reviewed_at_utc=args.reviewed_at_utc,
        notes=args.notes,
    )
    append_review_result(args.output, record)
    print(
        json.dumps(
            {
                "status": "RECORDED",
                "commit": record["commit"],
                "scenario": record["scenario"],
                "result": record["result"],
                "unreal_product_reality_cleared": False,
            },
            sort_keys=True,
        )
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(_main())
