"""Guard the first-playable-loop evidence record against over-claiming."""

from __future__ import annotations

import re
from pathlib import Path

DOC = Path(__file__).resolve().parents[1] / "docs" / "FIRST_PLAYABLE_LOOP_EVIDENCE.md"
UNVERIFIABLE_ON_RUNNER = ("fabrication", "repeatable objective", "editor or packaged")


def _rows() -> list[list[str]]:
    rows = []
    for line in DOC.read_text(encoding="utf-8").splitlines():
        if line.startswith("| ") and line[2].isdigit():
            rows.append([c.strip() for c in re.split(r"(?<!\\)\|", line.strip().strip("|"))])
    return rows


def test_every_step_has_command_result_and_status() -> None:
    rows = _rows()
    assert len(rows) >= 10
    for cells in rows:
        assert len(cells) == 5, cells
        assert all(cells), cells
        assert cells[4].startswith(("PASSED", "PENDING")), cells


def test_unverifiable_steps_are_pending_never_passed() -> None:
    for cells in _rows():
        if any(key in cells[1].lower() for key in UNVERIFIABLE_ON_RUNNER):
            assert cells[4].startswith("PENDING"), cells
            assert cells[3] == "Not run" or cells[3].startswith("Not run"), cells


def test_passed_rows_still_list_in_engine_pending() -> None:
    for cells in _rows():
        if cells[4].startswith("PASSED"):
            assert "PENDING" in cells[4], cells
