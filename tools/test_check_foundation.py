"""Regression coverage for the repository foundation validation tool.

`check_foundation.py` is the first gate every pull request runs in
`foundation.yml`. Before this module existed, none of its guard clauses or
branches had any executable regression coverage: a silent weakening of the
required-files list, the conflict-marker scan, or the empty-docs check could
land undetected because the only signal was "the real repository currently
happens to pass". These tests exercise each declared branch directly against
controlled fixture directories instead of relying on the ambient repository
state.
"""

from __future__ import annotations

from pathlib import Path
import tempfile
import unittest
from contextlib import redirect_stdout
from io import StringIO

import check_foundation


class _TempRootTestCase(unittest.TestCase):
    """Base class that gives each test an isolated fake repository root."""

    def setUp(self) -> None:
        self._tmpdir = tempfile.TemporaryDirectory()
        self.root = Path(self._tmpdir.name)
        self._original_root = check_foundation.ROOT
        self._original_required_files = check_foundation.REQUIRED_FILES
        check_foundation.ROOT = self.root

    def tearDown(self) -> None:
        check_foundation.ROOT = self._original_root
        check_foundation.REQUIRED_FILES = self._original_required_files
        self._tmpdir.cleanup()

    def write(self, relative: str, content: str = "content") -> Path:
        path = self.root / relative
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(content, encoding="utf-8")
        return path


class ValidateRequiredFilesTests(_TempRootTestCase):
    def test_passes_when_every_required_file_is_present_and_non_empty(self) -> None:
        check_foundation.REQUIRED_FILES = ("README.md", "docs/VISION.md")
        self.write("README.md", "hello")
        self.write("docs/VISION.md", "vision")

        errors: list[str] = []
        check_foundation.validate_required_files(errors)

        self.assertEqual(errors, [])

    def test_reports_missing_required_file(self) -> None:
        check_foundation.REQUIRED_FILES = ("README.md",)
        # README.md intentionally not created.

        errors: list[str] = []
        check_foundation.validate_required_files(errors)

        self.assertEqual(len(errors), 1)
        self.assertIn("missing required foundation file: README.md", errors[0])

    def test_reports_empty_required_file_distinctly_from_missing(self) -> None:
        check_foundation.REQUIRED_FILES = ("README.md",)
        self.write("README.md", "")

        errors: list[str] = []
        check_foundation.validate_required_files(errors)

        self.assertEqual(len(errors), 1)
        self.assertIn("required foundation file is empty: README.md", errors[0])

    def test_reports_every_missing_or_empty_file_independently(self) -> None:
        check_foundation.REQUIRED_FILES = ("a.md", "b.md", "c.md")
        self.write("b.md", "")
        self.write("c.md", "present")
        # a.md left missing entirely.

        errors: list[str] = []
        check_foundation.validate_required_files(errors)

        self.assertEqual(len(errors), 2)
        joined = "\n".join(errors)
        self.assertIn("missing required foundation file: a.md", joined)
        self.assertIn("required foundation file is empty: b.md", joined)
        self.assertNotIn("c.md", joined)

    def test_directory_at_required_path_counts_as_missing(self) -> None:
        check_foundation.REQUIRED_FILES = ("README.md",)
        (self.root / "README.md").mkdir(parents=True)

        errors: list[str] = []
        check_foundation.validate_required_files(errors)

        self.assertEqual(len(errors), 1)
        self.assertIn("missing required foundation file: README.md", errors[0])


class ShouldScanTests(unittest.TestCase):
    def test_files_inside_ignored_directories_are_skipped(self) -> None:
        for ignored in check_foundation.IGNORED_DIRS:
            with self.subTest(ignored=ignored):
                path = Path("repo") / ignored / "nested" / "file.py"
                self.assertFalse(check_foundation.should_scan(path))

    def test_extensionless_allowlisted_names_are_scanned(self) -> None:
        for name in ("LICENSE", ".editorconfig", ".gitattributes", ".gitignore", "CODEOWNERS"):
            with self.subTest(name=name):
                self.assertTrue(check_foundation.should_scan(Path("repo") / name))

    def test_known_text_suffix_is_scanned(self) -> None:
        for suffix in sorted(check_foundation.TEXT_SUFFIXES):
            with self.subTest(suffix=suffix):
                self.assertTrue(check_foundation.should_scan(Path(f"module{suffix}")))

    def test_suffix_matching_is_case_insensitive(self) -> None:
        self.assertTrue(check_foundation.should_scan(Path("README.MD")))

    def test_unknown_binary_suffix_is_not_scanned(self) -> None:
        self.assertFalse(check_foundation.should_scan(Path("texture.png")))

    def test_unrelated_file_deep_under_an_allowed_directory_is_still_scanned(self) -> None:
        self.assertTrue(check_foundation.should_scan(Path("repo/docs/nested/notes.md")))


class ValidateConflictMarkersTests(_TempRootTestCase):
    def test_clean_tree_reports_no_errors(self) -> None:
        self.write("README.md", "Everything is fine.\n")

        errors: list[str] = []
        check_foundation.validate_conflict_markers(errors)

        self.assertEqual(errors, [])

    def test_detects_each_conflict_marker_variant(self) -> None:
        for marker in ("<<<<<<<", "=======", ">>>>>>>"):
            with self.subTest(marker=marker):
                with tempfile.TemporaryDirectory() as case_root:
                    check_foundation.ROOT = Path(case_root)
                    (check_foundation.ROOT / "README.md").write_text(
                        f"before\n{marker} ours\nafter\n", encoding="utf-8"
                    )

                    errors: list[str] = []
                    check_foundation.validate_conflict_markers(errors)

                    self.assertEqual(len(errors), 1)
                    self.assertIn("README.md:2", errors[0])

    def test_conflict_marker_inside_ignored_directory_is_not_reported(self) -> None:
        self.write("build/generated.md", "<<<<<<< ours\n")

        errors: list[str] = []
        check_foundation.validate_conflict_markers(errors)

        self.assertEqual(errors, [])

    def test_conflict_marker_in_unrecognized_binary_suffix_is_not_reported(self) -> None:
        self.write("art.png", "<<<<<<< ours\n")

        errors: list[str] = []
        check_foundation.validate_conflict_markers(errors)

        self.assertEqual(errors, [])

    def test_undecodable_file_is_skipped_without_raising(self) -> None:
        path = self.root / "binary.md"
        path.write_bytes(b"\xff\xfe\x00\x01broken")

        errors: list[str] = []
        try:
            check_foundation.validate_conflict_markers(errors)
        except UnicodeDecodeError:
            self.fail("validate_conflict_markers must skip undecodable files, not raise")

        self.assertEqual(errors, [])

    def test_reports_every_offending_file_and_line_number(self) -> None:
        self.write("a.md", "ok\n<<<<<<< ours\n")
        self.write("docs/b.md", "1\n2\n>>>>>>> theirs\n")

        errors: list[str] = []
        check_foundation.validate_conflict_markers(errors)

        self.assertEqual(len(errors), 2)
        joined = "\n".join(errors)
        self.assertIn("a.md:2", joined)
        self.assertIn(str(Path("docs") / "b.md") + ":3", joined)


class ValidateDocsTests(_TempRootTestCase):
    def test_missing_docs_directory_is_reported(self) -> None:
        errors: list[str] = []
        check_foundation.validate_docs(errors)

        self.assertEqual(errors, ["docs directory is missing"])

    def test_empty_markdown_file_in_docs_is_reported(self) -> None:
        self.write("docs/ROADMAP.md", "")
        self.write("docs/VISION.md", "content")

        errors: list[str] = []
        check_foundation.validate_docs(errors)

        self.assertEqual(len(errors), 1)
        self.assertIn("documentation file is empty: docs/ROADMAP.md", errors[0].replace("\\", "/"))

    def test_non_empty_docs_produce_no_errors(self) -> None:
        self.write("docs/VISION.md", "content")

        errors: list[str] = []
        check_foundation.validate_docs(errors)

        self.assertEqual(errors, [])

    def test_non_markdown_files_in_docs_are_not_checked(self) -> None:
        self.write("docs/notes.txt", "")

        errors: list[str] = []
        check_foundation.validate_docs(errors)

        self.assertEqual(errors, [])


class ErrorHelperTests(unittest.TestCase):
    def test_appends_message_and_prints_it(self) -> None:
        errors: list[str] = []
        buffer = StringIO()
        with redirect_stdout(buffer):
            check_foundation.error("something is wrong", errors)

        self.assertEqual(errors, ["something is wrong"])
        self.assertIn("ERROR: something is wrong", buffer.getvalue())


class MainTests(_TempRootTestCase):
    def _run_main_quietly(self) -> int:
        buffer = StringIO()
        with redirect_stdout(buffer):
            exit_code = check_foundation.main()
        return exit_code

    def test_returns_zero_and_reports_success_for_a_healthy_tree(self) -> None:
        check_foundation.REQUIRED_FILES = ("README.md",)
        self.write("README.md", "hello")
        self.write("docs/VISION.md", "content")

        buffer = StringIO()
        with redirect_stdout(buffer):
            exit_code = check_foundation.main()

        self.assertEqual(exit_code, 0)
        self.assertIn("Foundation validation passed", buffer.getvalue())

    def test_returns_one_and_reports_failure_count_for_a_broken_tree(self) -> None:
        check_foundation.REQUIRED_FILES = ("README.md", "LICENSE")
        self.write("LICENSE", "")
        self.write("docs/VISION.md", "content")
        # README.md left missing -> 2 total errors (missing + empty), with
        # validate_docs kept clean so this test isolates validate_required_files.

        buffer = StringIO()
        with redirect_stdout(buffer):
            exit_code = check_foundation.main()

        self.assertEqual(exit_code, 1)
        self.assertIn("Foundation validation failed with 2 error(s).", buffer.getvalue())

    def test_aggregates_errors_across_all_three_validators(self) -> None:
        # Missing docs directory (validate_docs) plus a missing required file
        # (validate_required_files) plus a conflict marker (validate_conflict_markers)
        # must all surface together, proving main() does not short-circuit.
        check_foundation.REQUIRED_FILES = ("README.md",)
        self.write("conflict.md", "<<<<<<< ours\n")
        # No docs/ directory and no README.md created.

        buffer = StringIO()
        with redirect_stdout(buffer):
            exit_code = check_foundation.main()

        output = buffer.getvalue()
        self.assertEqual(exit_code, 1)
        self.assertIn("missing required foundation file: README.md", output)
        self.assertIn("docs directory is missing", output)
        self.assertIn("conflict.md:1", output)
        self.assertIn("Foundation validation failed with 3 error(s).", output)


if __name__ == "__main__":
    unittest.main()
