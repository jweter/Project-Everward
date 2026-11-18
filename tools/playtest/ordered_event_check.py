#!/usr/bin/env python3
"""Check that a first-loop playtest's recorded events occurred in order.

Pure logic only: no file, path, stream, clock, random or network input.
"""

from __future__ import annotations


def find_missing_ordered_event(observed: list[str], required: list[str]) -> str | None:
    """Return the first entry of ``required`` that cannot be matched in order.

    Walks ``required`` in order, matching each entry to the earliest position
    in ``observed`` that is strictly after the previous match's position.
    Matching is exact and case-sensitive. Returns the first required entry
    with no such position, or ``None`` once every required entry has
    matched. An empty ``required`` list returns ``None``. Duplicate entries
    in ``required`` each need a distinct, later position in ``observed``.
    """
    last_matched_position = -1
    for entry in required:
        found_position = None
        for position in range(last_matched_position + 1, len(observed)):
            if observed[position] == entry:
                found_position = position
                break
        if found_position is None:
            return entry
        last_matched_position = found_position
    return None
