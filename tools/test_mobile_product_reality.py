import unittest

from mobile_product_reality import render_state_report


class MobileProductRealityTests(unittest.TestCase):
    def test_report_is_build_seed_bound_responsive_and_escaped(self) -> None:
        html = render_state_report(
            {
                "commit": "abc123",
                "scenario": "save/load <roundtrip>",
                "seed": 42,
                "initial_state": {"storage_kg": 10.0, "probe_id": "prime"},
                "final_state": {"storage_kg": 12.5, "probe_id": "prime"},
                "invariants": {"identity_preserved": True, "mass_conserved": True},
            }
        )
        self.assertIn('name="viewport"', html)
        self.assertIn("abc123", html)
        self.assertIn("save/load &lt;roundtrip&gt;", html)
        self.assertIn("42", html)
        self.assertIn("storage_kg", html)
        self.assertIn("identity_preserved</strong>: PASS", html)
        self.assertIn("Human Product Reality:</strong> UNREVIEWED", html)
        self.assertIn("Unreal rendering", html)


if __name__ == "__main__":
    unittest.main()
