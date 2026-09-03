from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[1]
SIM = ROOT / "src/simulation/include/everward/simulation"
SOURCE = ROOT / "unreal/Source/Everward"


class Phase2ManipulatorReleaseSurfaceTests(unittest.TestCase):
    """Slice 7's completion sub-slice, "release-with-consequence"
    (PHASE2_VERTICAL_SLICE_PLAN.md / PHASE2_MANIPULATOR_RELEASE_TEST.md):
    every prior sub-slice explicitly named the same outstanding gap --
    releasing a grasped target always simply let go of it wherever it
    currently was, with no re-collision against the probe's own hull. This
    adds exactly one gate, in a wrapper module parallel to
    manipulator_grasp.hpp's own gate over begin_grasp, without touching
    ManipulatorRig::release_grasp's own documented unconditional behavior.
    Exercised as a source contract the same way grasp/move's own integration
    layers are (see test_phase2_manipulator_grasp_surface.py)."""

    def setUp(self) -> None:
        self.manipulator = (SIM / "manipulator.hpp").read_text(encoding="utf-8")
        self.manipulator_release = (SIM / "manipulator_release.hpp").read_text(encoding="utf-8")
        self.adapter_h = (SOURCE / "ProbeSimulationAdapter.h").read_text(encoding="utf-8")
        self.adapter_cpp = (SOURCE / "ProbeSimulationAdapter.cpp").read_text(encoding="utf-8")
        self.cmake = (ROOT / "src/simulation/CMakeLists.txt").read_text(encoding="utf-8")

    def test_release_grasp_itself_stays_unconditional(self) -> None:
        # The new gate lives in the wrapper module, not in ManipulatorRig
        # itself -- release_grasp keeps its own documented "always allowed"
        # behavior so other callers (e.g. a future forced-release path) are
        # not silently re-gated by this sub-slice.
        self.assertIn("void release_grasp(ManipulatorArmId id)", self.manipulator)
        self.assertIn("Releasing is always allowed while grasping", self.manipulator)

    def test_release_gate_reuses_the_existing_compound_hull_envelope(self) -> None:
        self.assertIn("everward/simulation/manipulator_hull_contact.hpp", self.manipulator_release)
        self.assertIn("ProbeCompoundCollisionEnvelope", self.manipulator_release)
        self.assertIn("sphere_intersects_compound_hull", self.manipulator_release)
        self.assertIn("rig.release_grasp(id)", self.manipulator_release)
        # No second placement/rotation convention invented.
        self.assertIn("rotate_local_contact_offset(", self.manipulator_release)
        self.assertNotIn("#include \"CoreMinimal.h\"", self.manipulator_release)
        self.assertNotIn("USTRUCT", self.manipulator_release)

    def test_release_fails_closed_on_missing_grasp_or_deregistered_body(self) -> None:
        self.assertIn("if (held_id.empty()) return false;", self.manipulator_release)
        self.assertIn("if (found == bodies.end()) return false;", self.manipulator_release)

    def test_runtime_overload_reads_live_authoritative_state_without_caching(self) -> None:
        self.assertIn("const DamageAwareProbeRuntime& runtime", self.manipulator_release)
        self.assertIn("runtime.snapshot()", self.manipulator_release)
        self.assertIn("runtime.static_bodies()", self.manipulator_release)

    def test_cmake_registers_manipulator_release_tests(self) -> None:
        self.assertIn("manipulator_release_tests.cpp", self.cmake)
        self.assertIn("everward_manipulator_release_tests", self.cmake)

    def test_unreal_adapter_forwards_the_gated_outcome_without_owning_it(self) -> None:
        self.assertIn("everward/simulation/manipulator_release.hpp", self.adapter_cpp)
        self.assertIn(
            "everward::simulation::attempt_release_grasped_target(*Manipulators, *Core", self.adapter_cpp)
        # The command surface (CommandReleaseGraspedTarget) is unchanged in
        # ProbeSimulationAdapter.h -- only its implementation's gating logic
        # moved off a direct, unconditional rig call.
        self.assertIn("CommandReleaseGraspedTarget", self.adapter_h)


if __name__ == "__main__":
    unittest.main()
