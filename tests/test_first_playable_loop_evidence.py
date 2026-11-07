"""Guard the first-playable-loop evidence record against over-claiming.

Run with: python3 -m unittest discover -s tests -p "test_first_playable_loop_evidence.py"
"""

from __future__ import annotations

import re
import unittest
from pathlib import Path

DOC = Path(__file__).resolve().parents[1] / "docs" / "FIRST_PLAYABLE_LOOP_EVIDENCE.md"
UNVERIFIABLE_ON_RUNNER = ("fabrication", "repeatable objective", "editor or packaged")


def _rows() -> list[list[str]]:
    rows = []
    for line in DOC.read_text(encoding="utf-8").splitlines():
        if line.startswith("| ") and line[2].isdigit():
            rows.append([c.strip() for c in re.split(r"(?<!\\)\|", line.strip().strip("|"))])
    return rows


class FirstPlayableLoopEvidenceTests(unittest.TestCase):
    def test_every_step_has_command_result_and_status(self) -> None:
        rows = _rows()
        self.assertGreaterEqual(len(rows), 10)
        for cells in rows:
            self.assertEqual(len(cells), 5, cells)
            self.assertTrue(all(cells), cells)
            self.assertTrue(cells[4].startswith(("PASSED", "PENDING")), cells)

    def test_ctest_commands_name_the_build_directory(self) -> None:
        for cells in _rows():
            if "ctest" in cells[2]:
                for part in re.findall(r"`ctest[^`]*`", cells[2]):
                    self.assertIn("--test-dir build/simulation", part, cells)

    def test_unverifiable_steps_are_pending_never_passed(self) -> None:
        for cells in _rows():
            if any(key in cells[1].lower() for key in UNVERIFIABLE_ON_RUNNER):
                self.assertTrue(cells[4].startswith("PENDING"), cells)
                self.assertTrue(cells[3].startswith("Not run"), cells)

    def test_passed_rows_still_list_in_engine_pending(self) -> None:
        for cells in _rows():
            if cells[4].startswith("PASSED"):
                self.assertIn("PENDING", cells[4], cells)


if __name__ == "__main__":
    unittest.main()
