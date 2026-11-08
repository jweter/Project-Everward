"""Pure contract tests: synthetic fixtures do NOT establish Unreal Product Reality."""
from __future__ import annotations

import importlib.util
import json
import pathlib
import tempfile
import unittest

MODULE_PATH = pathlib.Path(__file__).resolve().parent / "playtest" / "agent_session_gate.py"
SPEC = importlib.util.spec_from_file_location("agent_session_gate", MODULE_PATH)
assert SPEC and SPEC.loader
gate = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(gate)

SHA = "a" * 40
SCENARIO = {
    "id": "phase2_first_repair",
    "required_events": [
        "fix_it_awakened",
        "scan_completed",
        "mining_contact",
        "material_collected",
        "fix_it_repair_started",
        "fix_it_repair_completed",
    ],
    "required_assertions": [
        "selected_target_confirmed",
        "actual_tip_surface_contact",
        "stored_material_increased",
        "repair_consumed_material_and_energy",
        "component_integrity_increased",
    ],
}
EVENTS = ["session_started", "fix_it_runtime_bound", *SCENARIO["required_events"], "session_ended"]


def fixture():
    session = {
        "schema_version": 1,
        "status": "complete",
        "session_id": "test-id",
        "engine_version": "5.8.0",
        "project_name": "Everward",
    }
    context = {
        "repository": "jweter/Project-Everward",
        "git_sha": SHA,
        "execution_mode": "unreal_pie",
        "scenario_id": "phase2_first_repair",
        "session_id": "test-id",
        "map": "/Game/Phase2/Starter",
        "seed": 148,
    }
    events = [
        {"event_type": kind, "elapsed_seconds": float(i), "details": "synthetic"}
        for i, kind in enumerate(EVENTS)
    ]
    assertions = {
        "source_sha": SHA,
        "scenario_id": "phase2_first_repair",
        "checks": [
            {"id": ident, "source": "unreal_functional_test", "passed": True}
            for ident in SCENARIO["required_assertions"]
        ],
    }
    return session, events, context, dict(SCENARIO), assertions


def evaluate(*fixture_args):
    return gate.evaluate(*fixture_args, expected_sha=SHA)


class AgentSessionGateTests(unittest.TestCase):
    def test_complete_synthetic_contract_only(self):
        result = evaluate(*fixture())
        self.assertEqual(result["status"], "PASS")
        self.assertFalse(result["product_accepted"])
        self.assertEqual(result["tested_commit"], SHA)

    def test_real_unreal_evidence_required_not_headless_smoke(self):
        s, ev, ctx, scenario, a = fixture()
        ctx["execution_mode"] = "headless_smoke"
        self.assertEqual(evaluate(s, ev, ctx, scenario, a)["status"], "REVIEW_REQUIRED")

    def test_mismatched_exact_sha_is_environment_failure(self):
        s, ev, ctx, scenario, a = fixture()
        ctx["git_sha"] = "b" * 40
        self.assertEqual(evaluate(s, ev, ctx, scenario, a)["status"], "ENVIRONMENT_FAILURE")

    def test_incomplete_session_is_never_accepted(self):
        s, ev, ctx, scenario, a = fixture()
        s["status"] = "running"
        self.assertEqual(evaluate(s, ev, ctx, scenario, a)["status"], "REVIEW_REQUIRED")

    def test_missing_ordered_real_contact_event_is_review_required(self):
        s, ev, ctx, scenario, a = fixture()
        ev = [v for v in ev if v["event_type"] != "mining_contact"]
        self.assertEqual(evaluate(s, ev, ctx, scenario, a)["status"], "REVIEW_REQUIRED")

    def test_reversed_event_order_is_review_required(self):
        s, ev, ctx, scenario, a = fixture()
        ev[3]["event_type"], ev[4]["event_type"] = ev[4]["event_type"], ev[3]["event_type"]
        self.assertEqual(evaluate(s, ev, ctx, scenario, a)["status"], "REVIEW_REQUIRED")

    def test_unsorted_elapsed_seconds_is_review_required(self):
        s, ev, ctx, scenario, a = fixture()
        ev[-1]["elapsed_seconds"] = 0
        self.assertEqual(evaluate(s, ev, ctx, scenario, a)["status"], "REVIEW_REQUIRED")

    def test_unrecorded_adapter_assertions_cannot_be_inferred(self):
        s, ev, ctx, scenario, a = fixture()
        a["checks"] = []
        self.assertEqual(evaluate(s, ev, ctx, scenario, a)["status"], "REVIEW_REQUIRED")

    def test_false_unreal_assertion_is_fail(self):
        s, ev, ctx, scenario, a = fixture()
        a["checks"][3]["passed"] = False
        self.assertEqual(evaluate(s, ev, ctx, scenario, a)["status"], "FAIL")

    def test_issue_marker_needs_review_not_an_automatic_pass(self):
        s, ev, ctx, scenario, a = fixture()
        ev.insert(-1, {"event_type": "issue_marker", "elapsed_seconds": ev[-2]["elapsed_seconds"] + .5})
        self.assertEqual(evaluate(s, ev, ctx, scenario, a)["status"], "REVIEW_REQUIRED")

    def test_missing_assertions_file_or_invalid_json_never_passes(self):
        with tempfile.TemporaryDirectory() as directory:
            root = pathlib.Path(directory)
            (root / "session.json").write_text(json.dumps(fixture()[0]), encoding="utf-8")
            (root / "events.jsonl").write_text(
                "\n".join(json.dumps(v) for v in fixture()[1]), encoding="utf-8")
            context_path = root / "run_context.json"
            scenario_path = root / "scenario.json"
            context_path.write_text(json.dumps(fixture()[2]), encoding="utf-8")
            scenario_path.write_text(json.dumps(fixture()[3]), encoding="utf-8")
            report = gate.validate_files(root, context_path, scenario_path, expected_sha=SHA)
            self.assertEqual(report["status"], "REVIEW_REQUIRED")
            (root / "authoritative_assertions.json").write_text("{invalid", encoding="utf-8")
            report = gate.validate_files(root, context_path, scenario_path, expected_sha=SHA)
            self.assertEqual(report["status"], "REVIEW_REQUIRED")

    def test_serialized_file_evaluation_requires_actual_unreal_fields(self):
        with tempfile.TemporaryDirectory() as directory:
            root = pathlib.Path(directory)
            s, ev, ctx, scenario, assertions = fixture()
            (root / "session.json").write_text(json.dumps(s), encoding="utf-8")
            (root / "events.jsonl").write_text(
                "".join(json.dumps(v) + "\n" for v in ev), encoding="utf-8")
            (root / "authoritative_assertions.json").write_text(json.dumps(assertions), encoding="utf-8")
            ctx_path = root / "context.json"
            spec_path = root / "spec.json"
            ctx_path.write_text(json.dumps(ctx), encoding="utf-8")
            spec_path.write_text(json.dumps(scenario), encoding="utf-8")
            self.assertEqual(
                gate.validate_files(root, ctx_path, spec_path, expected_sha=SHA)["status"],
                "PASS",
            )


if __name__ == "__main__":
    unittest.main()
