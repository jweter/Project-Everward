import importlib.util
from pathlib import Path
import sys
import unittest


ROOT = Path(__file__).parent
for module_name in ("scenario", "run_record", "capture_pair"):
    if module_name in sys.modules:
        continue
    module_path = ROOT / f"{module_name}.py"
    spec = importlib.util.spec_from_file_location(module_name, module_path)
    module = importlib.util.module_from_spec(spec)
    sys.modules[module_name] = module
    assert spec.loader is not None
    spec.loader.exec_module(module)

capture_pair = sys.modules["capture_pair"]


class CapturePairTests(unittest.TestCase):
    def scenario(self):
        return {"scenario_version": 2, "name": "icy_asteroid_mining_v1"}

    def record(self, engine, **capture_overrides):
        capture = {
            "engine_version": "test",
            "os_version": "Windows 11",
            "cpu_model": "CPU",
            "gpu_model": "GPU",
            "ram_gib": 32,
            "project_settings": {"quality": "benchmark"},
            "cpu_frame_time_ms": 10.0,
            "gpu_frame_time_ms": 12.0,
            "peak_memory_mib": 1000.0,
            "implementation_hours": 20.0,
            "build_size_mib": 500.0,
            "screenshots": ["capture.png"],
            "notes": "",
        }
        capture.update(capture_overrides)
        return {
            "record_version": 1,
            "engine": engine,
            "scenario_version": 2,
            "scenario_name": "icy_asteroid_mining_v1",
            "captured_at_utc": "2026-08-18T20:00:00Z",
            "capture": capture,
        }

    def provenance(self, revision="abc123", handoff="deadbeef"):
        return {"source_revision": revision, "handoff_sha256": handoff}

    def test_matching_pair_is_comparable(self):
        result = capture_pair.validate_capture_pair(
            self.scenario(),
            self.record("godot"),
            self.record("unreal"),
            self.provenance(),
            self.provenance(),
        )
        self.assertEqual(result["status"], "comparable")
        self.assertEqual(result["source_revision"], "abc123")
        self.assertEqual(result["hardware"]["gpu_model"], "GPU")

    def test_source_revision_mismatch_is_rejected(self):
        with self.assertRaisesRegex(ValueError, "source revisions do not match"):
            capture_pair.validate_capture_pair(
                self.scenario(),
                self.record("godot"),
                self.record("unreal"),
                self.provenance(revision="left"),
                self.provenance(revision="right"),
            )

    def test_handoff_hash_mismatch_is_rejected(self):
        with self.assertRaisesRegex(ValueError, "handoff hashes do not match"):
            capture_pair.validate_capture_pair(
                self.scenario(),
                self.record("godot"),
                self.record("unreal"),
                self.provenance(handoff="left"),
                self.provenance(handoff="right"),
            )

    def test_hardware_mismatch_is_rejected(self):
        with self.assertRaisesRegex(ValueError, "benchmark hardware does not match: gpu_model"):
            capture_pair.validate_capture_pair(
                self.scenario(),
                self.record("godot"),
                self.record("unreal", gpu_model="Different GPU"),
                self.provenance(),
                self.provenance(),
            )

    def test_every_declared_hardware_field_mismatch_is_rejected(self):
        # test_hardware_mismatch_is_rejected above only exercises gpu_model.
        # HARDWARE_FIELDS declares four distinct fields that must each be
        # compared independently; a field silently dropped from that tuple
        # (typo, refactor, merge conflict) would let a real hardware mismatch
        # for that field pass validation unnoticed, since every other field
        # still matches between the two records.
        mismatched_values = {
            "os_version": "Windows 10",
            "cpu_model": "Different CPU",
            "gpu_model": "Different GPU",
            "ram_gib": 64,
        }
        self.assertEqual(set(mismatched_values), set(capture_pair.HARDWARE_FIELDS))
        for field, value in mismatched_values.items():
            with self.subTest(field=field):
                with self.assertRaisesRegex(
                    ValueError, f"benchmark hardware does not match: {field}"
                ):
                    capture_pair.validate_capture_pair(
                        self.scenario(),
                        self.record("godot"),
                        self.record("unreal", **{field: value}),
                        self.provenance(),
                        self.provenance(),
                    )

    def test_hardware_fields_are_exactly_the_four_expected_fields(self):
        # The loop test above iterates whatever HARDWARE_FIELDS currently
        # contains and cannot by itself detect a field being removed from
        # it; pin the declared tuple's contents independently.
        self.assertEqual(
            set(capture_pair.HARDWARE_FIELDS),
            {"os_version", "cpu_model", "gpu_model", "ram_gib"},
        )

    def test_incomplete_provenance_is_rejected(self):
        with self.assertRaisesRegex(ValueError, "invalid left provenance fields"):
            capture_pair.validate_capture_pair(
                self.scenario(),
                self.record("godot"),
                self.record("unreal"),
                {"source_revision": "abc123"},
                self.provenance(),
            )

    def test_every_declared_provenance_field_is_required(self):
        # test_incomplete_provenance_is_rejected above only ever removes
        # handoff_sha256. REQUIRED_PROVENANCE_FIELDS declares two distinct
        # fields that must each be present; a field silently dropped from
        # that check (typo, refactor, merge conflict) would let a
        # provenance document missing that field pass validation
        # unnoticed, since the other declared field is still present.
        for field in capture_pair.REQUIRED_PROVENANCE_FIELDS:
            with self.subTest(field=field):
                provenance = self.provenance()
                del provenance[field]
                with self.assertRaisesRegex(ValueError, f"missing: {field}"):
                    capture_pair.validate_capture_pair(
                        self.scenario(),
                        self.record("godot"),
                        self.record("unreal"),
                        provenance,
                        self.provenance(),
                    )

    def test_every_declared_provenance_field_rejects_a_blank_value(self):
        # _validate_provenance()'s per-field non-empty-string check had
        # zero test coverage for either declared field: no existing test
        # ever supplied both required keys with a blank value for either.
        for field in capture_pair.REQUIRED_PROVENANCE_FIELDS:
            with self.subTest(field=field):
                provenance = self.provenance()
                provenance[field] = "   "
                with self.assertRaisesRegex(
                    ValueError, f"provenance field {field!r} must be non-empty"
                ):
                    capture_pair.validate_capture_pair(
                        self.scenario(),
                        self.record("godot"),
                        self.record("unreal"),
                        provenance,
                        self.provenance(),
                    )

    def test_every_declared_provenance_field_rejects_a_non_string_value(self):
        # Same per-field check, non-string branch: also zero coverage
        # before this test, for either declared field.
        for field in capture_pair.REQUIRED_PROVENANCE_FIELDS:
            with self.subTest(field=field):
                provenance = self.provenance()
                provenance[field] = 12345
                with self.assertRaisesRegex(
                    ValueError, f"provenance field {field!r} must be non-empty"
                ):
                    capture_pair.validate_capture_pair(
                        self.scenario(),
                        self.record("godot"),
                        self.record("unreal"),
                        provenance,
                        self.provenance(),
                    )

    def test_required_provenance_fields_are_exactly_the_two_expected_fields(self):
        # The loop tests above iterate whatever REQUIRED_PROVENANCE_FIELDS
        # currently contains and cannot by themselves detect a field being
        # silently added to or removed from it; pin the declared set itself.
        self.assertEqual(
            capture_pair.REQUIRED_PROVENANCE_FIELDS,
            {"source_revision", "handoff_sha256"},
        )


if __name__ == "__main__":
    unittest.main()
