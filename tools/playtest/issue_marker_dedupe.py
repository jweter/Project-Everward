#!/usr/bin/env python3
"""Deduplicate issue marker details while preserving order and casing."""

from __future__ import annotations

def dedupe_issue_marker_details(details: list[str]) -> list[str]:
    """Return the list of issue-marker detail strings with duplicates removed, keeping only the first occurrence of each in its original order and casing. Two details are duplicates when they are equal after stripping leading/trailing whitespace and lowercasing. Entries that are empty or whitespace-only after stripping are dropped entirely, even if duplicated. An empty input list returns an empty list."""
    seen = set()
    deduped = []
    for detail in details:
        stripped = detail.strip().lower()
        if stripped and stripped not in seen:
            seen.add(stripped)
            deduped.append(detail)
    return deduped
