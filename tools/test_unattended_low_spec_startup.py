from __future__ import annotations

import importlib.util
import json
import os
import re
import tempfile
import unittest
from pathlib import Path
from unittest import mock

ROOT = Path(__file__).resolve().parents[1]
WORKER_PATH = ROOT / "tools" / "everward_unattended_worker.py"
PROBE_PATH = ROOT / "tools" / "run_unattended_low_spec_startup.ps1"
HARNESS_PATH = ROOT / "tools" / "run_phase2_first_playtest.ps1"
PUBLISH_PATH = ROOT / "tools" / "publish_unattended_product_reality.ps1"

spec = importlib.util.spec_from_file_location("everward_unattended_worker_low_spec", WORKER_PATH)
assert spec is not None and spec.loader is not None
worker = importlib.util.module_from_spec(spec)
spec.loader.exec_module(worker)

COMMIT = "a" * 40


def probe_payload(**overrides: object) -> dict[str, object]:
    payload: dict[str, object] = {
        "schema_version": 1,
        "profile": "low-spec-development",
        "git_commit": COMMIT,
        "editor_launched": True,
        "startup_marker_observed": True,
        "startup_seconds": 212.4,
        "engine_reported_init_seconds": 198.7,
        "exited_before_window_end": False,
        "exit_code": None,
        "startup_timeout_seconds": 900,
        "sample_window_seconds": 60,
        "sampled_after_startup_seconds": 60.3,
        "peak_working_set_mib": 3121.5,
        "memory_sample_count": 540,
        "telemetry_errors": 0,
        "cleanup": "terminated",
        "error": None,
    }
    payload.update(overrides)
    return payload


def interpret(payload: object, exit_code: int = 0) -> dict[str, object]:
    return worker.interpret_low_spec_startup(payload, expected_commit=COMMIT, runner_exit_code=exit_code)


def low_spec_commands(source: str) -> str:
    match = re.search(r"\$LowSpecCommands = @\((.*?)\) -join \",\"", source)
    assert match is not None
    return match.group(1)


class LowSpecStartupInterpretationTests(unittest.TestCase):
    def test_complete_bounded_startup_with_memory_passes_without_product_reality_claim(self) -> None:
        check = interpret(probe_payload())
        self.assertEqual(check["status"], "PASS")
        self.assertEqual(check["commit"], COMMIT)
        self.assertEqual(check["memory"]["value"], 3121.5)
        self.assertEqual(check["startup"]["startup_seconds"], 212.4)
        self.assertIs(check["product_reality_claimed"], False)
        self.assertIn("frame time or playability", check["not_evidence_for"])
        self.assertIn("gameplay loop quality", check["not_evidence_for"])

    def test_missing_or_malformed_result_is_review_required(self) -> None:
        for payload in (None, [], "PASS", {"schema_version": 2}, probe_payload(schema_version="1")):
            with self.subTest(payload=payload):
                self.assertEqual(interpret(payload)["status"], "REVIEW_REQUIRED")

    def test_commit_identity_mismatch_is_review_required(self) -> None:
        for commit in ("b" * 40, None, "", "A" * 39, "not-a-commit"):
            with self.subTest(commit=commit):
                check = interpret(probe_payload(git_commit=commit))
                self.assertEqual(check["status"], "REVIEW_REQUIRED")
                self.assertIn("build identity", check["reason"])

    def test_unavailable_memory_telemetry_is_review_required(self) -> None:
        for overrides in (
            {"peak_working_set_mib": None},
            {"peak_working_set_mib": 0},
            {"peak_working_set_mib": -5.0},
            {"peak_working_set_mib": "3000"},
            {"peak_working_set_mib": float("nan")},
            {"peak_working_set_mib": True},
            {"memory_sample_count": 0},
            {"memory_sample_count": "540"},
        ):
            with self.subTest(overrides=overrides):
                check = interpret(probe_payload(**overrides))
                self.assertEqual(check["status"], "REVIEW_REQUIRED")

    def test_editor_not_launched_or_not_stopped_is_review_required(self) -> None:
        self.assertEqual(interpret(probe_payload(editor_launched=False))["status"], "REVIEW_REQUIRED")
        for cleanup in ("failed", "pending", "unexpected", None):
            with self.subTest(cleanup=cleanup):
                self.assertEqual(interpret(probe_payload(cleanup=cleanup))["status"], "REVIEW_REQUIRED")

    def test_startup_failures_are_fail_with_allow_listed_category(self) -> None:
        cases = {
            "startup_timeout": probe_payload(startup_marker_observed=False, startup_seconds=None),
            "exited_before_startup": probe_payload(
                startup_marker_observed=False, exited_before_window_end=True, exit_code=3
            ),
            "exited_during_sample_window": probe_payload(exited_before_window_end=True, exit_code=-1),
        }
        for category, payload in cases.items():
            with self.subTest(category=category):
                check = interpret(payload)
                self.assertEqual(check["status"], "FAIL")
                self.assertEqual(check["failure_category"], category)
        timeout = interpret(probe_payload(), exit_code=124)
        self.assertEqual(timeout["status"], "FAIL")
        self.assertEqual(timeout["failure_category"], "probe_timeout")

    def test_marker_without_marker_value_is_not_truthy_coerced(self) -> None:
        check = interpret(probe_payload(startup_marker_observed="true"))
        self.assertEqual(check["status"], "FAIL")

    def test_incomplete_post_startup_window_is_review_required(self) -> None:
        for overrides in (
            {"sampled_after_startup_seconds": 12.0},
            {"sampled_after_startup_seconds": None},
            {"startup_seconds": None},
            {"sample_window_seconds": None},
        ):
            with self.subTest(overrides=overrides):
                self.assertEqual(interpret(probe_payload(**overrides))["status"], "REVIEW_REQUIRED")

    def test_only_allow_listed_facts_are_copied_from_probe(self) -> None:
        secret = r"C:\Users\someone\secret\UnrealEditor.exe"
        check = interpret(probe_payload(error=secret, extra_path=secret, profile=secret))
        self.assertNotIn(secret, json.dumps(check))
        self.assertNotIn("someone", json.dumps(check))


class LowSpecStartupRunnerTests(unittest.TestCase):
    def _run(self, payload: object | None, *, exit_code: int = 0, log_text: str = "") -> dict[str, object]:
        with tempfile.TemporaryDirectory() as temp:
            base = Path(temp)
            repo = base / "Everward Playtest" / "Project-Everward"
            (repo / "tools").mkdir(parents=True)
            (repo / "tools" / "run_unattended_low_spec_startup.ps1").write_text("# probe\n", encoding="utf-8")
            unreal = base / "UE_5.8"
            (unreal / "Engine" / "Build").mkdir(parents=True)
            (unreal / "Engine" / "Build" / "Build.version").write_text(
                json.dumps({"MajorVersion": 5, "MinorVersion": 8, "PatchVersion": 0, "Changelist": 123}),
                encoding="utf-8",
            )
            state = base / "state"
            logs = state / "logs" / COMMIT[:12]
            # Paths are precomputed: os.name is patched below, so avoid constructing bare Path() there.
            result_path = logs / "unreal-low-spec-startup.json"
            unreal_log = logs / "unreal-low-spec-startup.log"

            def fake_run_logged(args: list[str], *, cwd: Path, log_path: Path, timeout_seconds: float):
                self.assertEqual(args[args.index("-ResultPath") + 1], str(result_path))
                self.assertEqual(args[args.index("-LogPath") + 1], str(unreal_log))
                self.assertLessEqual(timeout_seconds, 1500.0)
                log_path.parent.mkdir(parents=True, exist_ok=True)
                log_path.write_text("runner\n", encoding="utf-8")
                if log_text:
                    unreal_log.write_text(log_text.format(repo=repo, unreal=unreal), encoding="utf-8")
                if payload is not None:
                    result_path.write_text(json.dumps(payload), encoding="utf-8-sig")
                return exit_code, 5.0

            with mock.patch.object(worker.os, "name", "nt"), mock.patch.object(
                worker, "run_logged", side_effect=fake_run_logged
            ), mock.patch.object(worker.Path, "home", return_value=base / "home"), mock.patch.dict(
                os.environ, {"USERNAME": "privateuser"}
            ):
                return worker.run_unreal_low_spec_startup(repo, unreal, COMMIT, logs, state)

    def test_runner_records_exact_build_identity_and_relative_log(self) -> None:
        check = self._run(probe_payload())
        self.assertEqual(check["status"], "PASS")
        self.assertEqual(
            check["build_identity"],
            {"commit": COMMIT, "target": "EverwardEditor Win64 Development", "unreal_engine_version": "5.8.0-CL123"},
        )
        self.assertEqual(check["log"], f"logs/{COMMIT[:12]}/unreal-low-spec-startup.log")
        self.assertNotIn("failure_tail", check)

    def test_missing_result_file_fails_closed(self) -> None:
        check = self._run(None)
        self.assertEqual(check["status"], "REVIEW_REQUIRED")

    def test_failure_tail_and_probe_error_are_sanitized(self) -> None:
        log_text = (
            "LogInit: Command Line: {repo}\\unreal\\Everward.uproject\n"
            "LogInit: User: privateuser\n"
            "Error: crash in {unreal}\\Engine\\Binaries\\Win64\\UnrealEditor.exe\n"
        )
        payload = probe_payload(
            startup_marker_observed=False,
            exited_before_window_end=True,
            error="Failed near C:/Users/PRIVATEUSER/AppData/x",
        )
        check = self._run(payload, log_text=log_text)
        self.assertEqual(check["status"], "FAIL")
        serialized = json.dumps(check)
        self.assertNotIn("Everward Playtest", serialized)
        self.assertNotIn("UE_5.8", serialized)
        self.assertNotIn("privateuser", serialized.lower())
        self.assertIn("<REPO_ROOT>", check["failure_tail"])
        self.assertIn("<UE_5_8_ROOT>", check["failure_tail"])
        self.assertIn("<USER>", check["probe_error"])

    def test_missing_probe_script_is_review_required(self) -> None:
        with tempfile.TemporaryDirectory() as temp:
            check = worker.run_unreal_low_spec_startup(
                Path(temp), Path(temp), COMMIT, Path(temp) / "logs", Path(temp)
            )
        self.assertEqual(check["status"], "REVIEW_REQUIRED")

    def test_sanitize_text_is_case_insensitive_for_windows_paths(self) -> None:
        repo = Path(r"C:\Users\Someone\Documents\Everward Playtest\Project-Everward")
        text = r"c:\users\someone\documents\everward playtest\project-everward\unreal"
        self.assertEqual(worker.sanitize_text(text, repo_root=repo), r"<REPO_ROOT>\unreal")


class LowSpecStartupWorkerOrderingTests(unittest.TestCase):
    def test_cache_requires_low_spec_marker_for_the_same_commit(self) -> None:
        with tempfile.TemporaryDirectory() as temp:
            state = Path(temp)
            (state / worker.LAST_PASS_MARKER).write_text(COMMIT + "\n", encoding="ascii")
            self.assertIsNone(worker.read_last_pass(state))
            (state / worker.LOW_SPEC_PASS_MARKER).write_text("b" * 40 + "\n", encoding="ascii")
            self.assertIsNone(worker.read_last_pass(state))
            (state / worker.LOW_SPEC_PASS_MARKER).write_text(COMMIT + "\n", encoding="ascii")
            self.assertEqual(worker.read_last_pass(state), COMMIT)

    def test_low_spec_runs_only_after_headless_smoke_and_before_pass_markers(self) -> None:
        source = WORKER_PATH.read_text(encoding="utf-8")
        run_body = source.split("def run_worker", 1)[1].split("def build_parser", 1)[0]
        smoke = run_body.index("run_unreal_headless_smoke(")
        smoke_gate = run_body.index('if headless_smoke.get("status") != "PASS":')
        low_spec = run_body.index("run_unreal_low_spec_startup(")
        marker = run_body.index("LAST_PASS_MARKER).write_text")
        low_marker = run_body.index("LOW_SPEC_PASS_MARKER).write_text")
        self.assertLess(smoke, smoke_gate)
        self.assertLess(smoke_gate, low_spec)
        self.assertLess(low_spec, marker)
        self.assertLess(low_spec, low_marker)
        self.assertIn('report["checks"]["unreal_low_spec_startup"] = low_spec', run_body)
        # The exact synced commit, not a second lookup, is the identity the probe must match.
        self.assertIn("run_unreal_low_spec_startup(repo_root, unreal_root, synced,", run_body)

    def test_low_spec_failure_prevents_pass_and_cache(self) -> None:
        checks = {"unreal_headless_smoke": {"status": "PASS"}, "unreal_low_spec_startup": {"status": "REVIEW_REQUIRED"}}
        self.assertEqual(worker.aggregate_status(checks), "REVIEW_REQUIRED")
        checks["unreal_low_spec_startup"] = {"status": "FAIL"}
        self.assertEqual(worker.aggregate_status(checks), "FAIL")

    def test_human_only_debt_keeps_frame_time_and_gameplay(self) -> None:
        source = WORKER_PATH.read_text(encoding="utf-8")
        self.assertIn("actual frame-time/playability until automated Unreal telemetry exists", source)
        self.assertIn("gameplay desire-to-continue judgment", source)


class LowSpecLaunchContractTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        cls.probe = PROBE_PATH.read_text(encoding="utf-8")
        cls.harness = HARNESS_PATH.read_text(encoding="utf-8")

    def test_probe_reuses_the_exact_harness_low_spec_console_commands(self) -> None:
        self.assertEqual(low_spec_commands(self.probe), low_spec_commands(self.harness))

    def test_probe_reuses_the_harness_window_and_memory_source(self) -> None:
        for expected in ('"-WINDOWED", "-ResX=1280", "-ResY=720", "-ExecCmds=`"$LowSpecCommands`""', "WorkingSet64"):
            self.assertIn(expected, self.harness)
            self.assertIn(expected, self.probe)

    def test_probe_is_bounded_and_always_stops_its_editor(self) -> None:
        self.assertIn("[int]$StartupTimeoutSeconds = 900", self.probe)
        self.assertIn("[int]$SampleWindowSeconds = 60", self.probe)
        finally_body = self.probe.rsplit("finally {", 1)[1]
        self.assertIn("taskkill.exe /PID $EditorProcess.Id /T /F", finally_body)
        self.assertIn('$Result.cleanup = "failed"', finally_body)
        self.assertIn("Set-Content -Path $ResultPath", finally_body)

    def test_probe_does_not_change_defaults_mechanics_or_repository_evidence(self) -> None:
        for forbidden in (
            "DefaultEngine.ini",
            "DefaultGame.ini",
            "LowSpecPlay.ini",
            "playtests\\phase2",
            "everward-manual-playtest.lock",
            "register_unattended_product_reality",
            "src\\simulation",
            "Set-ObservationCheck",
        ):
            self.assertNotIn(forbidden, self.probe)

    def test_probe_result_contains_no_path_fields(self) -> None:
        result_block = self.probe.split("$Result = [ordered]@{", 1)[1].split("}", 1)[0]
        self.assertNotRegex(result_block, r"(?i)path|root|log\b")

    def test_probe_never_claims_product_reality(self) -> None:
        self.assertNotRegex(self.probe, r"(?i)\"PASS\"")
        self.assertIn("never decides PASS", self.probe)


class LowSpecPublisherTests(unittest.TestCase):
    def test_publisher_lists_low_spec_check_with_allow_listed_facts_only(self) -> None:
        text = PUBLISH_PATH.read_text(encoding="utf-8")
        self.assertIn('"unreal_low_spec_startup"', text)
        self.assertIn("Format-AllowListedNumber", text)
        self.assertIn('if ($Category -match "^[a-z_]{1,40}$")', text)
        self.assertIn("not visual, control-feel, frame-time, or gameplay evidence", text)
        self.assertNotIn("$Check.failure_tail)", text)
        self.assertNotIn("$Check.probe_error", text)
        self.assertNotIn("$Check.log", text)


if __name__ == "__main__":
    unittest.main()
