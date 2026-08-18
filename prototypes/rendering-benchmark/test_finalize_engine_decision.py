import importlib.util
import json
from pathlib import Path
import sys
import tempfile
import unittest
from unittest import mock


ROOT = Path(__file__).parent
for module_name in ("benchmark", "scenario", "run_record", "normalization", "decision_packet", "finalize_engine_decision"):
    if module_name in sys.modules:
        continue
    module_path = ROOT / f"{module_name}.py"
    spec = importlib.util.spec_from_file_location(module_name, module_path)
    module = importlib.util.module_from_spec(spec)
    sys.modules[module_name] = module
    assert spec.loader is not None
    spec.loader.exec_module(module)

normalization = sys.modules["normalization"]
finalizer = sys.modules["finalize_engine_decision"]


class RenderingDecisionFinalizerTests(unittest.TestCase):
    def qualitative(self, engine, score=8.0):
        return {
            "assessment_version": 1,
            "engine": engine,
            "metrics": {
                metric: {"score": score, "evidence": f"review evidence for {metric}"}
                for metric in normalization.QUALITATIVE_METRICS
            },
        }

    def test_qualitative_document_is_engine_bound_and_complete(self):
        parsed = finalizer.parse_qualitative_document(self.qualitative("godot"), "godot")
        self.assertEqual(set(parsed), set(normalization.QUALITATIVE_METRICS))
        self.assertTrue(all(item.evidence for item in parsed.values()))

    def test_engine_mismatch_is_rejected(self):
        with self.assertRaisesRegex(ValueError, "does not match run record"):
            finalizer.parse_qualitative_document(self.qualitative("unreal"), "godot")

    def test_missing_qualitative_metric_is_rejected(self):
        document = self.qualitative("godot")
        removed = next(iter(normalization.QUALITATIVE_METRICS))
        del document["metrics"][removed]
        with self.assertRaisesRegex(ValueError, "missing"):
            finalizer.parse_qualitative_document(document, "godot")

    def test_finalize_preserves_paths_and_delegates_to_decision_contract(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            left_record = root / "godot.json"
            right_record = root / "unreal.json"
            left_qual = root / "godot-qual.json"
            right_qual = root / "unreal-qual.json"
            left_record.write_text(json.dumps({"engine": "godot"}), encoding="utf-8")
            right_record.write_text(json.dumps({"engine": "unreal"}), encoding="utf-8")
            left_qual.write_text(json.dumps(self.qualitative("godot")), encoding="utf-8")
            right_qual.write_text(json.dumps(self.qualitative("unreal")), encoding="utf-8")

            packet = {"status": "decision_ready", "recommendation": "godot"}
            with mock.patch.object(finalizer, "load_scenario", return_value={"scenario_version": 2, "name": "test"}), mock.patch.object(
                finalizer, "build_decision_packet", return_value=packet
            ) as build:
                artifact = finalizer.finalize("scenario.json", left_record, right_record, left_qual, right_qual)

            self.assertEqual(artifact["artifact_type"], "everward_phase1_engine_decision")
            self.assertEqual(artifact["decision_packet"], packet)
            self.assertEqual(artifact["inputs"]["left_run_record"], str(left_record))
            args = build.call_args.args
            self.assertEqual(args[1]["engine"], "godot")
            self.assertEqual(args[2]["engine"], "unreal")
            self.assertEqual(args[3][normalization.QUALITATIVE_METRICS[0]].score, 8.0)


if __name__ == "__main__":
    unittest.main()
