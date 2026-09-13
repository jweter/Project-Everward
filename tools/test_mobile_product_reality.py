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

    def test_scenario_selector_and_timeline_are_read_only_and_escaped(self) -> None:
        html = render_state_report(
            {
                "commit": "abc123",
                "scenario": "scenario-console",
                "seed": 42,
                "scenario_views": {
                    "mining": {
                        "label": "Mining <flow>",
                        "events": [
                            {"at": "tick 10", "event": "scan target"},
                            {
                                "at": "tick 12",
                                "event": "store mass",
                                "detail": "storage 10 -> 12.5 kg & conserved",
                            },
                        ],
                    },
                    "save-load": {
                        "label": "Save / Load",
                        "events": [
                            {"at": "tick 20", "event": "save authoritative state"},
                            {"at": "tick 21", "event": "reload identical state"},
                        ],
                    },
                },
            }
        )
        self.assertIn('id="scenario-selector"', html)
        self.assertIn("Scenario timeline", html)
        self.assertIn("Mining &lt;flow&gt;", html)
        self.assertIn("storage 10 -&gt; 12.5 kg &amp; conserved", html)
        self.assertIn('data-scenario="mining"', html)
        self.assertIn('data-scenario="save-load"', html)
        self.assertIn("selector.addEventListener('change',show)", html)
        self.assertIn("tick 21", html)
        self.assertIn("reload identical state", html)
        self.assertNotIn("authoritative state =", html)

    def test_scenario_view_validation_fails_closed(self) -> None:
        invalid_cases = [
            ({"scenario_views": []}, "scenario_views must be a mapping"),
            ({"scenario_views": {"save-load": "events"}}, "scenario view entries must be mappings"),
            (
                {"scenario_views": {"save-load": {"events": "not-a-list"}}},
                "scenario view events must be lists",
            ),
            (
                {"scenario_views": {"save-load": {"events": ["bad"]}}},
                "scenario timeline events must be mappings",
            ),
            (
                {"scenario_views": {"save-load": {"events": [{"event": "load"}]}}},
                "missing required field: at",
            ),
            (
                {"scenario_views": {"save-load": {"events": [{"at": "tick 1"}]}}},
                "missing required field: event",
            ),
            (
                {"scenario_views": {"mining": {"events": []}, " mining ": {"events": []}}},
                "scenario view ids must be unique after normalization",
            ),
            (
                {"scenario_views": {1: {"events": []}, "1": {"events": []}}},
                "scenario view ids must be unique after normalization",
            ),
        ]
        for extra, message in invalid_cases:
            with self.subTest(message=message):
                evidence = {"commit": "abc123", "scenario": "identity", "seed": 42}
                evidence.update(extra)
                with self.assertRaisesRegex(ValueError, message):
                    render_state_report(evidence)

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
                        "product_reality_debt": "Manipulator visual alignment requires Unreal acceptance",
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
        self.assertIn("Manipulator visual alignment requires Unreal acceptance", html)

    def test_component_card_rejects_explicit_invalid_commit_override(self) -> None:
        for invalid in (None, True, [], {}):
            with self.subTest(commit=invalid):
                with self.assertRaisesRegex(ValueError, "(missing|invalid) required field: commit"):
                    render_state_report(
                        {
                            "commit": "abc123",
                            "scenario": "component-console",
                            "seed": 42,
                            "components": {
                                "Simulation": {
                                    "status": "PASS",
                                    "scenario": "simulation-v1",
                                    "timestamp": "2026-09-12T22:00:00Z",
                                    "commit": invalid,
                                }
                            },
                        }
                    )

    def test_product_reality_required_component_requires_debt(self) -> None:
        with self.assertRaisesRegex(ValueError, "missing required field: product_reality_debt"):
            render_state_report(
                {
                    "commit": "abc123",
                    "scenario": "component-console",
                    "seed": 42,
                    "components": {
                        "Manipulator": {
                            "status": "PRODUCT REALITY REQUIRED",
                            "scenario": "manipulator-visual-v1",
                            "timestamp": "2026-09-12T22:00:00Z",
                        }
                    },
                }
            )

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