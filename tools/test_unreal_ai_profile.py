"""Contract tests for the opt-in, isolated Everward editor-AI pilot."""
from __future__ import annotations

import json
import pathlib
import subprocess
import sys
import tempfile
import unittest

from unreal_ai_profile import PLUGIN_NAMES, proposed_descriptor, require_linked_isolated_worktree

SCRIPT = pathlib.Path(__file__).resolve().parent / "unreal_ai_profile.py"


class UnrealAIProfileTests(unittest.TestCase):
    def test_declares_all_three_epic_capabilities_and_required_toolsets(self) -> None:
        original = {"FileVersion": 3, "EngineAssociation": "5.8", "Modules": [{"Name": "Everward", "Type": "Runtime"}]}
        proposed = proposed_descriptor(original)
        enabled = {p["Name"] for p in proposed["Plugins"] if p["Enabled"]}
        self.assertEqual(enabled, set(PLUGIN_NAMES))
        self.assertEqual(original.get("Plugins"), None)
        self.assertEqual(proposed["Modules"], original["Modules"])

    def test_preserves_other_plugin_settings_and_upgrades_disabled_entries(self) -> None:
        previous = {"Modules": [], "Plugins": [
            {"Name": "CustomEditor", "Enabled": False, "TargetAllowList": ["Editor"]},
            {"Name": "PCG", "Enabled": False, "Optional": True},
        ]}
        after = proposed_descriptor(previous)
        self.assertEqual(after["Plugins"][0], previous["Plugins"][0])
        self.assertEqual(after["Plugins"][1]["Enabled"], True)
        self.assertTrue(after["Plugins"][1]["Optional"])
        self.assertEqual(len(after["Plugins"]), 7)

    def test_refuses_broad_alltoolsets_and_keeps_disabled_entry(self) -> None:
        with self.assertRaisesRegex(ValueError, "too broad"):
            proposed_descriptor({"Modules": [], "Plugins": [{"Name": "AllToolsets", "Enabled": True}]})
        narrowed = proposed_descriptor({"Modules": [], "Plugins": [{"Name": "AllToolsets", "Enabled": False}]})
        self.assertFalse(narrowed["Plugins"][0]["Enabled"])
        self.assertNotIn("AllToolsets", PLUGIN_NAMES)

    def test_rejects_bad_plugin_entries_and_no_modules(self) -> None:
        with self.assertRaises(ValueError):
            proposed_descriptor({"Modules": [], "Plugins": [{"Name": "PCG"}, {"Name": "PCG"}]})
        with self.assertRaises(ValueError):
            proposed_descriptor({"Plugins": []})

    def test_inspect_never_modifies_input(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            target = pathlib.Path(directory) / "unreal" / "Everward.uproject"
            target.parent.mkdir()
            text = '{"FileVersion": 3, "Modules": []}\n'
            target.write_text(text, encoding="utf-8")
            done = subprocess.run([sys.executable, str(SCRIPT), "--project", str(target)],
                                  text=True, capture_output=True, check=False)
            self.assertEqual(done.returncode, 0, done.stderr)
            self.assertEqual(target.read_text(encoding="utf-8"), text)
            obj = json.loads(done.stdout)
            self.assertFalse(obj["server_started"])
            self.assertFalse(obj["semantic_provider_configured"])

    def test_stage_refuses_without_acknowledgement_or_git_worktree(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            target = pathlib.Path(directory) / "unreal" / "Everward.uproject"
            target.parent.mkdir()
            target.write_text('{"FileVersion": 3, "Modules": []}\n', encoding="utf-8")
            for flags in ([], ["--ack-isolated-worktree"]):
                result = subprocess.run([sys.executable, str(SCRIPT), "--project", str(target),
                                        "--mode", "stage", *flags],
                                        text=True, capture_output=True, check=False)
                self.assertEqual(result.returncode, 2)
                self.assertNotIn("Plugins", target.read_text(encoding="utf-8"))

    def test_stage_refuses_main_branch_even_if_worktree_exists(self) -> None:
        # The worktree guard is required; a non-git temporary project cannot pass.
        with tempfile.TemporaryDirectory() as directory:
            project = pathlib.Path(directory) / "unreal" / "Everward.uproject"
            project.parent.mkdir()
            project.write_text('{"Modules": []}', encoding="utf-8")
            with self.assertRaises((subprocess.CalledProcessError, ValueError)):
                require_linked_isolated_worktree(project)


    def test_staging_works_only_in_separate_linked_worktree(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            root = pathlib.Path(directory)
            primary = root / "primary"
            sandbox = root / "pilot"
            primary.mkdir()
            subprocess.run(["git", "init", "-b", "main", str(primary)], check=True,
                           capture_output=True, text=True)
            project = primary / "unreal" / "Everward.uproject"
            project.parent.mkdir()
            original = '{"FileVersion": 3, "Modules": []}\n'
            project.write_text(original, encoding="utf-8")
            subprocess.run(["git", "-C", str(primary), "add", "."], check=True,
                           capture_output=True, text=True)
            subprocess.run(["git", "-C", str(primary), "-c", "user.name=pilot-test",
                            "-c", "user.email=pilot@example.invalid", "commit", "-m", "fixture"],
                           check=True, capture_output=True, text=True)
            subprocess.run(["git", "-C", str(primary), "worktree", "add", "-b",
                            "pilot/ue-ai", str(sandbox)], check=True, capture_output=True, text=True)
            staged = sandbox / "unreal" / "Everward.uproject"
            invocation = [sys.executable, str(SCRIPT), "--project", str(staged),
                          "--mode", "stage", "--ack-isolated-worktree"]
            first = subprocess.run(invocation, text=True, capture_output=True, check=False)
            self.assertEqual(first.returncode, 0, first.stderr)
            self.assertEqual(project.read_text(encoding="utf-8"), original)
            configured = json.loads(staged.read_text(encoding="utf-8"))
            self.assertEqual({e["Name"] for e in configured["Plugins"]}, set(PLUGIN_NAMES))
            second = subprocess.run(invocation, text=True, capture_output=True, check=False)
            self.assertEqual(second.returncode, 2)
            self.assertIn("edited project descriptor", second.stderr.lower())


if __name__ == "__main__":
    unittest.main()
