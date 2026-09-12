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

    def test_component_cards_are_read_only_evidence_not_game_logic(self) -> None:
        html = render_state_report(
            {
                "commit": "abc123",
                "scenario": "component-console",
                "seed": 42,
                "components": {
                    "Save/Load": {
                        "status": "PASS",
                        "scenario": "save-load-roundtrip-v1",
                        "timestamp": "2026-09-12T22:00:00Z",
                        "detail": "authoritative round-trip invariant passed",
                        "product_reality_debt": "Unreal F5/F6 flow still requires local acceptance",
                    },
                    "Manipulator": {
                        "status": "PRODUCT REALITY REQUIRED",
                        "scenario": "manipulator-visual-v1",
                        "timestamp": "2026-09-12T22:00:00Z",
                        "detail": "deterministic state evidence is insufficient for visual alignment",
                    },
                },
            }
        )
        self.assertIn("Functional test console", html)
        self.assertIn("Save/Load", html)
        self.assertIn("Manipulator", html)
        self.assertIn("PRODUCT REALITY REQUIRED", html)
        self.assertIn("save-load-roundtrip-v1", html)
        self.assertIn("abc123", html)
        self.assertIn("Unreal F5/F6 flow still requires local acceptance", html)

    def test_component_card_rejects_unknown_status(self) -> None:
        with self.assertRaisesRegex(ValueError, "invalid component status"):
            render_state_report(
                {
                    "commit": "abc123",
                    "scenario": "component-console",
                    "seed": 42,
                    "components": {
                        "Simulation": {
                            "status": "GREENISH",
                            "scenario": "simulation-v1",
                            "timestamp": "2026-09-12T22:00:00Z",
                        }
                    },
                }
            )

    def test_component_cards_require_structured_evidence(self) -> None:
        with self.assertRaisesRegex(ValueError, "component entries must be mappings"):
            render_state_report(
                {
                    "commit": "abc123",
                    "scenario": "component-console",
                    "seed": 42,
                    "components": {"Simulation": "PASS"},
                }
            )

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
