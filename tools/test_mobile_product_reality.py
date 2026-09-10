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

    def test_report_rejects_non_boolean_invariant_results(self) -> None:
        with self.assertRaisesRegex(ValueError, "invariant results must be booleans"):
            render_state_report(
                {
                    "commit": "abc123",
                    "scenario": "bad-invariant",
                    "seed": 42,
                    "invariants": {"mass_conserved": "false"},
                }
            )

    def test_report_rejects_null_identity_fields(self) -> None:
        for field in ("commit", "scenario", "seed"):
            with self.subTest(field=field):
                evidence = {"commit": "abc123", "scenario": "identity", "seed": 42}
                evidence[field] = None
                with self.assertRaisesRegex(ValueError, f"missing required field: {field}"):
                    render_state_report(evidence)


if __name__ == "__main__":
    unittest.main()
