from __future__ import annotations

import unittest

from mobile_product_reality import render_state_report
from mobile_product_reality_adapters import adapt_control_state_evidence


class ControlStateAdapterTests(unittest.TestCase):
    def test_control_state_adapter_renders_authoritative_transition_evidence(self) -> None:
        adapted = adapt_control_state_evidence(
            {
                "commit": "a" * 40,
                "scenario": "control-transition",
                "timestamp": "2026-09-14T04:00:00Z",
                "status": "PASS",
                "detail": "Authoritative controller evidence preserved the declared state transition.",
                "events": [
                    {"at": "tick-100", "event": "state", "detail": "IDLE"},
                    {"at": "tick-101", "event": "input", "detail": "deploy manipulator"},
                    {"at": "tick-102", "event": "state", "detail": "MANIPULATOR_ACTIVE"},
                ],
                "product_reality_debt": (
                    "Unreal input feel, prompts, remapping, controller path, and HUD readability remain pending."
                ),
            }
        )

        component = adapted["components"]["Control State"]
        self.assertEqual(component["status"], "PASS")
        self.assertEqual(component["scenario"], "control-transition")
        self.assertIn("Unreal input feel", component["product_reality_debt"])
        self.assertEqual(
            adapted["scenario_views"]["control-transition"]["events"][2]["detail"],
            "MANIPULATOR_ACTIVE",
        )

        rendered = render_state_report(adapted)
        self.assertIn("Control State", rendered)
        self.assertIn("MANIPULATOR_ACTIVE", rendered)
        self.assertIn("remapping", rendered)

    def test_control_state_adapter_requires_debt_when_product_reality_is_pending(self) -> None:
        with self.assertRaisesRegex(ValueError, "must name remaining debt"):
            adapt_control_state_evidence(
                {
                    "commit": "b" * 40,
                    "scenario": "control-transition",
                    "timestamp": "2026-09-14T04:00:00Z",
                    "status": "PRODUCT_REALITY_REQUIRED",
                    "events": [],
                }
            )


if __name__ == "__main__":
    unittest.main()
