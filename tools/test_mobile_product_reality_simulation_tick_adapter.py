from __future__ import annotations

import unittest

from mobile_product_reality import render_state_report
from mobile_product_reality_adapters import adapt_simulation_tick_evidence


COMMIT = "b" * 40


class SimulationTickMobileEvidenceAdapterTests(unittest.TestCase):
    def test_simulation_tick_adapter_renders_authoritative_tick_evidence(self) -> None:
        adapted = adapt_simulation_tick_evidence(
            {
                "commit": COMMIT,
                "scenario": "deterministic-tick-v1",
                "timestamp": "2026-09-14T00:30:00Z",
                "status": "PASS",
                "detail": "Authoritative simulation supplied the deterministic tick transition.",
                "events": [
                    {
                        "at": "tick 420",
                        "event": "fixed tick advanced",
                        "detail": "Resulting state came from simulation authority.",
                    },
                    {"at": "tick 421", "event": "state invariant verified"},
                ],
            }
        )
        report = {
            "commit": COMMIT,
            "scenario": "deterministic-tick-v1",
            "seed": "17",
            "initial_state": {},
            "final_state": {},
            "invariants": {},
            **adapted,
        }

        card = adapted["components"]["Simulation Tick"]
        self.assertEqual(card["status"], "PASS")
        self.assertEqual(card["commit"], COMMIT)
        html = render_state_report(report)
        self.assertIn("Simulation Tick", html)
        self.assertIn("fixed tick advanced", html)
        self.assertIn("state invariant verified", html)

    def test_simulation_tick_adapter_keeps_engine_only_debt_explicit(self) -> None:
        adapted = adapt_simulation_tick_evidence(
            {
                "commit": COMMIT,
                "scenario": "deterministic-tick-v1",
                "timestamp": "2026-09-14T00:31:00Z",
                "status": "PRODUCT_REALITY_REQUIRED",
                "product_reality_debt": "Unreal presentation and frame behavior",
                "events": [],
            }
        )

        card = adapted["components"]["Simulation Tick"]
        self.assertEqual(card["status"], "PRODUCT_REALITY_REQUIRED")
        self.assertEqual(
            card["product_reality_debt"], "Unreal presentation and frame behavior"
        )


if __name__ == "__main__":
    unittest.main()
