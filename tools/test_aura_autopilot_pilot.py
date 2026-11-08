"""Offline synthetic release tests; they do NOT load Unreal or run Aura."""
from __future__ import annotations

import hashlib
import importlib.util
import io
import json
from pathlib import Path
import subprocess
import tempfile
import unittest
import zipfile

SOURCE = Path(__file__).resolve().parent / "playtest" / "setup_aura_autopilot_pilot.py"
SPEC = importlib.util.spec_from_file_location("aura_setup_pilot", SOURCE)
assert SPEC and SPEC.loader
pilot = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(pilot)


def archive_fixture(root: Path, *, unsafe: bool = False) -> tuple[Path, dict]:
    blob = root / "Autopilot-Unreal5.8-Win64.zip"
    with zipfile.ZipFile(blob, "w") as z:
        z.writestr("Autopilot/Autopilot.uplugin", json.dumps({
            "FriendlyName": "Autopilot", "VersionName": "1.0.0",
        }))
        z.writestr("Autopilot/Binaries/Win64/UnrealEditor-AutopilotPlaytest.dll", b"fake fixture bytes")
        if unsafe:
            z.writestr("Autopilot/../../host-file.txt", "bad")
    data = blob.read_bytes()
    return blob, {"archive_filename": blob.name, "archive_bytes": len(data),
                  "sha256": hashlib.sha256(data).hexdigest()}


def prepare_clean_pilot(parent: Path) -> Path:
    folder = parent / "Everward-Aura-Pilot" / "Project-Everward"
    folder.mkdir(parents=True)
    project = folder / "unreal" / "Everward.uproject"
    project.parent.mkdir()
    project.write_text(json.dumps({
        "FileVersion": 3, "EngineAssociation": "5.8",
        "Modules": [{"Name": "Everward", "Type": "Runtime"}],
    }), encoding="utf-8")
    def run(*cmd: str) -> None:
        subprocess.run(["git", "-C", str(folder), *cmd], capture_output=True, text=True, check=True)
    run("init", "-b", "main")
    run("remote", "add", "origin", "https://github.com/jweter/Project-Everward.git")
    run("add", ".")
    run("-c", "user.name=fixture", "-c", "user.email=fixture@example.invalid", "commit", "-m", "test")
    run("checkout", "-b", pilot.REQUIRED_BRANCH)
    return folder


class AuraAutopilotSetupTests(unittest.TestCase):
    def test_valid_package_has_expected_manifest_and_bounds(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            path, spec = archive_fixture(Path(tmp))
            facts = pilot.verify_archive(path, spec)
            self.assertEqual(facts["entries"], 2)
            self.assertEqual(facts["plugin_version"], "1.0.0")

    def test_checksum_mismatch_fails_closed(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            path, spec = archive_fixture(Path(tmp))
            spec["sha256"] = "0" * 64
            with self.assertRaisesRegex(ValueError, "SHA256"):
                pilot.verify_archive(path, spec)

    def test_traversal_zip_rejected(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            path, spec = archive_fixture(Path(tmp), unsafe=True)
            with self.assertRaisesRegex(ValueError, "Unsafe path"):
                pilot.verify_archive(path, spec)

    def test_refuse_non_sandbox_path_and_main_branch(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = prepare_clean_pilot(Path(tmp))
            project = root / "unreal" / "Everward.uproject"
            subprocess.run(["git", "-C", str(root), "checkout", "main"], check=True, capture_output=True)
            with self.assertRaisesRegex(ValueError, "branch"):
                pilot.validate_clone(root, project)
            with self.assertRaisesRegex(ValueError, "dedicated"):
                pilot.validate_clone(root.parent.parent / "elsewhere", project)

    def test_refuse_dirty_checkout(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = prepare_clean_pilot(Path(tmp))
            (root / "unreal" / "Everward.uproject").write_text('{"dirty":true}', encoding="utf-8")
            with self.assertRaisesRegex(ValueError, "clean"):
                pilot.validate_clone(root, root / "unreal" / "Everward.uproject")

    def test_only_isolated_clone_installed_never_modifies_archive_or_other_repo(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            parent = Path(tmp)
            root = prepare_clean_pilot(parent)
            project = root / "unreal" / "Everward.uproject"
            original_sha = hashlib.sha256(project.read_bytes()).hexdigest()
            archive, spec = archive_fixture(parent)
            result = pilot.stage(root, archive, spec)
            self.assertFalse(result["agent_service_started"])
            self.assertFalse(result["playtest_executed"])
            self.assertFalse(result["provider_account_configured"])
            self.assertTrue((root / "unreal" / "Plugins" / "Autopilot" / "Autopilot.uplugin").is_file())
            descriptor = json.loads(project.read_text(encoding="utf-8"))
            self.assertIn({"Name": "Autopilot", "Enabled": True}, descriptor["Plugins"])
            self.assertNotEqual(original_sha, hashlib.sha256(project.read_bytes()).hexdigest())
            with self.assertRaisesRegex(ValueError, "clean"):
                pilot.stage(root, archive, spec)

    def test_existing_plugin_is_never_replaced(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = prepare_clean_pilot(Path(tmp))
            archive, spec = archive_fixture(Path(tmp))
            plugin = root / "unreal" / "Plugins" / "Autopilot"
            plugin.mkdir(parents=True)
            with self.assertRaises(ValueError):
                pilot.stage(root, archive, spec)


if __name__ == "__main__":
    unittest.main()
