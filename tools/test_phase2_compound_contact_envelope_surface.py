from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[1]
TYPES = ROOT / "src/simulation/include/everward/simulation/types.hpp"


class Phase2CompoundContactEnvelopeSurfaceTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        cls.types = TYPES.read_text(encoding="utf-8")

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

    def test_legacy_eight_meter_sphere_is_explicitly_transitional(self) -> None:
        self.assertIn("Transitional compatibility field", self.types)
        self.assertIn("collision_envelope_radius_m{8.0}", self.types)
        self.assertIn("until the compound-envelope solver lands", self.types)


if __name__ == "__main__":
    unittest.main()
