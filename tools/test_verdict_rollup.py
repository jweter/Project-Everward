from __future__ import annotations

import importlib.util
import pathlib
import unittest

MODULE_PATH = pathlib.Path(__file__).resolve().parent / "playtest" / "verdict_rollup.py"
SPEC = importlib.util.spec_from_file_location("verdict_rollup", MODULE_PATH)
assert SPEC and SPEC.loader
verdict_rollup = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(verdict_rollup)

rollup_playtest_verdict = verdict_rollup.rollup_playtest_verdict


class VerdictRollupTests(unittest.TestCase):
    def test_empty_list_returns_pass(self) -> None:
        self.assertEqual(rollup_playtest_verdict([]), "PASS")

    def test_no_recognized_entries_returns_pass(self) -> None:
        self.assertEqual(rollup_playtest_verdict(["unknown", "", "   ", "whatever"]), "PASS")

    def test_mixed_case_blocker_outranks_everything(self) -> None:
        self.assertEqual(rollup_playtest_verdict(["pass", "partial", "blocker"]), "BLOCKER")
        self.assertEqual(rollup_playtest_verdict(["Fail", "  BlOcKeR  ", "Partial"]), "BLOCKER")

    def test_duplicate_entries_do_not_change_result(self) -> None:
        self.assertEqual(rollup_playtest_verdict(["fail", "FAIL", "partial"]), "FAIL")
        self.assertEqual(rollup_playtest_verdict(["pass", "PASS", "Pass"]), "PASS")

    def test_unrecognized_entries_are_ignored(self) -> None:
        self.assertEqual(rollup_playtest_verdict(["unknown", "  Pass  "]), "PASS")
        self.assertEqual(rollup_playtest_verdict(["nonsense", "fail", "???"]), "FAIL")

    def test_whitespace_is_stripped_before_matching(self) -> None:
        self.assertEqual(rollup_playtest_verdict(["  partial  "]), "PARTIAL")

    def test_result_is_always_one_of_the_four_canonical_strings(self) -> None:
        for findings, expected in (
            ([], "PASS"),
            (["partial"], "PARTIAL"),
            (["fail"], "FAIL"),
            (["blocker"], "BLOCKER"),
        ):
            result = rollup_playtest_verdict(findings)
            self.assertIn(result, {"PASS", "PARTIAL", "FAIL", "BLOCKER"})
            self.assertEqual(result, expected)


if __name__ == "__main__":
    unittest.main()
