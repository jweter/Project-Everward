from __future__ import annotations

import unittest

from mobile_product_reality import render_state_report
from mobile_product_reality_adapters import adapt_power_allocation_evidence


COMMIT = "c" * 40


class PowerAllocationMobileEvidenceAdapterTests(unittest.TestCase):
    def test_power_allocation_adapter_renders_authoritative_subsystem_evidence(self) -> None:
        adapted = adapt_power_allocation_evidence(
            {
                "commit": COMMIT,
                "scenario": "power-allocation-v1",
                "timestamp": "2026-09-14T01:20:00Z",
                "status": "PASS",
                "detail": "Authoritative simulation supplied subsystem allocation state.",
                "events": [
                    {
                        "at": "tick 700",
                        "event": "power allocation applied",
                        "detail": "Propulsion, Sensors, Computation, and Thermal Control came from simulation authority.",
                    },
                    {"at": "tick 701", "event": "capability state verified"},
                ],
            }
        )
        report = {
            "commit": COMMIT,
            "scenario": "power-allocation-v1",
            "seed": "17",
            "initial_state": {},
            "final_state": {},
            "invariants": {},
            **adapted,
        }

        card = adapted["components"]["Power Allocation"]
        self.assertEqual(card["status"], "PASS")
        self.assertEqual(card["commit"], COMMIT)
        html = render_state_report(report)
        self.assertIn("Power Allocation", html)
        self.assertIn("power allocation applied", html)
        self.assertIn("capability state verified", html)

    def test_power_allocation_adapter_keeps_unreal_only_debt_explicit(self) -> None:
        adapted = adapt_power_allocation_evidence(
            {
                "commit": COMMIT,
                "scenario": "power-allocation-v1",
                "timestamp": "2026-09-14T01:21:00Z",
                "status": "PRODUCT_REALITY_REQUIRED",
                "product_reality_debt": "Unreal HUD readability and control feel",
                "events": [],
            }
        )

        card = adapted["components"]["Power Allocation"]
        self.assertEqual(card["status"], "PRODUCT_REALITY_REQUIRED")
        self.assertEqual(
            card["product_reality_debt"], "Unreal HUD readability and control feel"
        )


if __name__ == "__main__":
    unittest.main()
