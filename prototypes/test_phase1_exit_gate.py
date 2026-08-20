import importlib.util
import json
from pathlib import Path
import sys
import tempfile
import unittest


ROOT = Path(__file__).parent
MODULE_PATH = ROOT / "phase1_exit_gate.py"
SPEC = importlib.util.spec_from_file_location("phase1_exit_gate", MODULE_PATH)
MODULE = importlib.util.module_from_spec(SPEC)
sys.modules["phase1_exit_gate"] = MODULE
assert SPEC.loader is not None
SPEC.loader.exec_module(MODULE)


class Phase1ExitGateTests(unittest.TestCase):
    def make_prototypes(self, root: Path) -> None:
        for relative in MODULE.PROTOTYPE_REQUIREMENTS.values():
            path = root / relative
            path.mkdir(parents=True)
            (path / "proof.txt").write_text("fixture\n", encoding="utf-8")

    def write_decision(self, root: Path, status="decision_ready", recommendation="unreal") -> Path:
        path = root / "decision.json"
        path.write_text(
            json.dumps(
                {
                    "artifact_version": 1,
                    "artifact_type": "everward_phase1_engine_decision",
                    "decision_packet": {"status": status, "recommendation": recommendation},
                }
            ),
            encoding="utf-8",
        )
        return path

    def test_missing_real_engine_decision_blocks_phase2(self):
        with tempfile.TemporaryDirectory() as temp_dir:
            root = Path(temp_dir)
            self.make_prototypes(root)
            report = MODULE.audit_phase1_exit(root)
        self.assertFalse(report["phase1_complete"])
        self.assertFalse(report["phase2_one_probe_authorized"])
        self.assertIn("real rendering benchmark decision artifact not supplied", report["blockers"])

    def test_non_decision_ready_artifact_blocks_phase2(self):
        with tempfile.TemporaryDirectory() as temp_dir:
            root = Path(temp_dir)
            self.make_prototypes(root)
            decision = self.write_decision(root, status="additional_evidence_required", recommendation=None)
            report = MODULE.audit_phase1_exit(root, decision)
        self.assertFalse(report["phase1_complete"])
        self.assertIn("engine benchmark is not decision-ready", " ".join(report["blockers"]))

    def test_decision_ready_godot_recommendation_does_not_authorize_phase2(self):
        with tempfile.TemporaryDirectory() as temp_dir:
            root = Path(temp_dir)
            self.make_prototypes(root)
            decision = self.write_decision(root, recommendation="godot")
            report = MODULE.audit_phase1_exit(root, decision)
        self.assertFalse(report["phase1_complete"])
        self.assertFalse(report["phase2_one_probe_authorized"])
        self.assertEqual(report["engine_decision"]["recommendation"], "godot")
        self.assertIn("does not validate the accepted Unreal production direction", " ".join(report["blockers"]))

    def test_complete_prototypes_and_unreal_decision_authorize_phase2(self):
        with tempfile.TemporaryDirectory() as temp_dir:
            root = Path(temp_dir)
            self.make_prototypes(root)
            decision = self.write_decision(root, recommendation="unreal")
            report = MODULE.audit_phase1_exit(root, decision)
        self.assertTrue(report["phase1_complete"])
        self.assertTrue(report["phase2_one_probe_authorized"])
        self.assertEqual(report["engine_decision"]["recommendation"], "unreal")
        self.assertEqual(report["engine_decision"]["required_production_engine"], "unreal")
        self.assertEqual(report["next_step"], "Begin Phase 2 — One Probe in Unreal Engine.")

    def test_missing_prototype_is_a_hard_blocker(self):
        with tempfile.TemporaryDirectory() as temp_dir:
            root = Path(temp_dir)
            self.make_prototypes(root)
            target = root / MODULE.PROTOTYPE_REQUIREMENTS["coordinate_scale"] / "proof.txt"
            target.unlink()
            decision = self.write_decision(root)
            report = MODULE.audit_phase1_exit(root, decision)
        self.assertFalse(report["phase1_complete"])
        self.assertIn("coordinate-scale", " ".join(report["blockers"]))

    def test_every_declared_prototype_is_a_hard_blocker_when_empty(self):
        # The test above only ever empties "coordinate_scale". A prototype
        # silently dropped from the emptiness check for any of the other
        # four PROTOTYPE_REQUIREMENTS keys (typo, refactor, merge conflict)
        # would not have been caught.
        for name, relative in MODULE.PROTOTYPE_REQUIREMENTS.items():
            with self.subTest(name=name):
                with tempfile.TemporaryDirectory() as temp_dir:
                    root = Path(temp_dir)
                    self.make_prototypes(root)
                    target = root / relative / "proof.txt"
                    target.unlink()
                    decision = self.write_decision(root)
                    report = MODULE.audit_phase1_exit(root, decision)
                self.assertFalse(report["phase1_complete"])
                self.assertFalse(report["phase2_one_probe_authorized"])
                self.assertFalse(report["prototypes"][name]["present"])
                self.assertIn(
                    f"missing or empty Phase 1 prototype: {relative}", report["blockers"]
                )

    def test_every_declared_prototype_is_a_hard_blocker_when_absent(self):
        # Same per-key gap, the other half of the `path.is_dir() and
        # any(path.iterdir())` presence check: a prototype directory that
        # does not exist at all, rather than merely being empty.
        for name, relative in MODULE.PROTOTYPE_REQUIREMENTS.items():
            with self.subTest(name=name):
                with tempfile.TemporaryDirectory() as temp_dir:
                    root = Path(temp_dir)
                    self.make_prototypes(root)
                    target = root / relative
                    for child in target.iterdir():
                        child.unlink()
                    target.rmdir()
                    decision = self.write_decision(root)
                    report = MODULE.audit_phase1_exit(root, decision)
                self.assertFalse(report["phase1_complete"])
                self.assertFalse(report["phase2_one_probe_authorized"])
                self.assertFalse(report["prototypes"][name]["present"])
                self.assertIn(
                    f"missing or empty Phase 1 prototype: {relative}", report["blockers"]
                )

    def test_prototype_requirements_are_exactly_the_five_expected_prototypes(self):
        # The loop tests above iterate whatever PROTOTYPE_REQUIREMENTS
        # currently contains and cannot by themselves detect a prototype
        # being silently added to or removed from it; pin the declared set.
        self.assertEqual(
            MODULE.PROTOTYPE_REQUIREMENTS,
            {
                "simulation_clock": "simulation-clock",
                "procedural_system": "procedural-system",
                "coordinate_scale": "coordinate-scale",
                "headless_simulation": "headless-simulation",
                "rendering_benchmark": "rendering-benchmark",
            },
        )


if __name__ == "__main__":
    unittest.main()
