from __future__ import annotations

import unittest

from mobile_product_reality import render_state_report
from mobile_product_reality_adapters import adapt_probe_state_evidence


COMMIT = "e" * 40


class ProbeStateAdapterTests(unittest.TestCase):
    def test_probe_state_adapter_renders_authoritative_identity_and_state(self) -> None:
        adapted = adapt_probe_state_evidence(
            {
                "commit": COMMIT,
                "scenario": "probe-state",
                "timestamp": "2026-09-14T05:30:00Z",
                "status": "PASS",
                "detail": "Authoritative probe identity and state were preserved for mobile review.",
                "events": [
                    {"at": "tick-200", "event": "identity", "detail": "Prime Gen-1"},
                    {"at": "tick-200", "event": "mass", "detail": "15000 kg"},
                    {"at": "tick-200", "event": "energy", "detail": "72 percent"},
                    {"at": "tick-200", "event": "storage", "detail": "18 kg occupied"},
                ],
                "product_reality_debt": (
                    "Unreal visual presentation, HUD readability, and embodied game feel remain pending."
                ),
            }
        )

        component = adapted["components"]["Probe State"]
        self.assertEqual(component["status"], "PASS")
        self.assertEqual(component["scenario"], "probe-state")
        self.assertIn("HUD readability", component["product_reality_debt"])
        self.assertEqual(
            adapted["scenario_views"]["probe-state"]["events"][0]["detail"],
            "Prime Gen-1",
        )

        report = {
            "commit": COMMIT,
            "scenario": "probe-state",
            "seed": "200",
            "initial_state": {},
            "final_state": {},
            "invariants": {},
            **adapted,
        }
        rendered = render_state_report(report)
        self.assertIn("Probe State", rendered)
        self.assertIn("Prime Gen-1", rendered)
        self.assertIn("HUD readability", rendered)

    def test_probe_state_adapter_requires_authoritative_events(self) -> None:
        with self.assertRaisesRegex(ValueError, "events must be a list"):
            adapt_probe_state_evidence(
                {
                    "commit": COMMIT,
                    "scenario": "probe-state",
                    "timestamp": "2026-09-14T05:30:00Z",
                    "status": "PASS",
                }
            )


if __name__ == "__main__":
    unittest.main()
