from __future__ import annotations

import importlib.util
import pathlib
import unittest

MODULE_PATH = pathlib.Path(__file__).resolve().parent / "playtest" / "ordered_event_check.py"
SPEC = importlib.util.spec_from_file_location("ordered_event_check", MODULE_PATH)
assert SPEC and SPEC.loader
ordered_event_check = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(ordered_event_check)

find_missing_ordered_event = ordered_event_check.find_missing_ordered_event


class OrderedEventCheckTests(unittest.TestCase):
    def test_empty_required_returns_none(self) -> None:
        self.assertIsNone(find_missing_ordered_event(["a"], []))
        self.assertIsNone(find_missing_ordered_event([], []))

    def test_missing_entry_is_reported(self) -> None:
        self.assertEqual(find_missing_ordered_event(["a", "b", "c"], ["a", "z"]), "z")

    def test_empty_observed_with_nonempty_required_returns_first_required(self) -> None:
        self.assertEqual(find_missing_ordered_event([], ["x"]), "x")
        self.assertEqual(find_missing_ordered_event([], ["x", "y"]), "x")

    def test_duplicate_required_entries_need_distinct_later_positions(self) -> None:
        self.assertEqual(find_missing_ordered_event(["x"], ["x", "x"]), "x")
        self.assertIsNone(find_missing_ordered_event(["x", "x"], ["x", "x"]))

    def test_all_entries_match_increasing_positions_returns_none(self) -> None:
        self.assertIsNone(find_missing_ordered_event(["a", "b", "c"], ["a", "c"]))

    def test_out_of_order_match_is_reported_missing(self) -> None:
        self.assertEqual(find_missing_ordered_event(["a", "b"], ["b", "a"]), "a")

    def test_matching_is_case_sensitive(self) -> None:
        self.assertEqual(find_missing_ordered_event(["A"], ["a"]), "a")


if __name__ == "__main__":
    unittest.main()
