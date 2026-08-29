from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[1]
SIM = ROOT / "src/simulation/include/everward/simulation"
TYPES = SIM / "types.hpp"
COMPOUND_CONTACT = SIM / "compound_contact.hpp"
RUNTIME = SIM / "software_policy.hpp"


class Phase2CompoundContactEnvelopeSurfaceTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        cls.types = TYPES.read_text(encoding="utf-8")
        cls.compound_contact = COMPOUND_CONTACT.read_text(encoding="utf-8")
        cls.runtime = RUNTIME.read_text(encoding="utf-8")

    def test_prime_contact_shape_has_five_engine_independent_samples(self) -> None:
        self.assertIn("struct ProbeCollisionSphereSample", self.types)
        self.assertIn("struct ProbeCompoundCollisionEnvelope", self.types)
        self.assertIn("static constexpr std::size_t SampleCount = 5", self.types)
        self.assertIn("ProbeCompoundCollisionEnvelope compound_collision_envelope", self.types)

    def test_shape_covers_long_hull_and_both_wing_regions(self) -> None:
        for token in (
            "{{5.0, 0.0, 0.0}, 1.35}",
            "{{0.0, 0.0, 0.0}, 1.60}",
            "{{-5.0, 0.0, 0.0}, 1.50}",
            "{{-0.5, -3.0, 0.0}, 1.00}",
            "{{-0.5, 3.0, 0.0}, 1.00}",
        ):
            self.assertIn(token, self.types)

    def test_legacy_eight_meter_sphere_is_now_diagnostic_only(self) -> None:
        # The compound-envelope solver has landed (see the wiring test below),
        # so the single 8 m radius is no longer the swept-collision input; it
        # is retained purely as a coarse diagnostic/telemetry figure.
        self.assertIn("Legacy single-sphere radius", self.types)
        self.assertIn("collision_envelope_radius_m{8.0}", self.types)
        self.assertIn("solver no longer", self.types)
        self.assertIn("consumes this value", self.types)

    def test_compound_envelope_solver_is_wired_into_probe_runtime(self) -> None:
        # This is the actual migration off the single bounding sphere: the
        # authoritative swept-contact solver in ProbeRuntime must consume the
        # five-sample compound envelope (rotated by attitude) rather than the
        # legacy collision_envelope_radius_m scalar.
        self.assertIn("sweep_compound_probe_against_body", self.runtime)
        self.assertIn("resolve_compound_contact", self.runtime)
        self.assertIn("core_.snapshot().compound_collision_envelope", self.runtime)
        self.assertIn("state.attitude_degrees", self.runtime)
        self.assertNotIn("core_.snapshot().collision_envelope_radius_m", self.runtime)


if __name__ == "__main__":
    unittest.main()
