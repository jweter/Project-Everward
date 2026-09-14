from __future__ import annotations

import unittest

from mobile_product_reality import render_state_report
from mobile_product_reality_adapters import adapt_invariant_evidence


COMMIT = "f" * 40


class InvariantAdapterTests(unittest.TestCase):
    def test_invariant_adapter_renders_authoritative_regression_findings(self) -> None:
        adapted = adapt_invariant_evidence(
            {
                "commit": COMMIT,
                "scenario": "survival-loop-invariants",
                "timestamp": "2026-09-14T06:55:00Z",
                "status": "FAIL",
                "detail": "Authoritative deterministic invariant lane reported one regression.",
                "events": [
                    {
                        "at": "tick-240",
                        "event": "energy_nonnegative",
                        "detail": "PASS",
                    },
                    {
                        "at": "tick-240",
                        "event": "storage_mass_conserved",
                        "detail": "FAIL",
                    },
                ],
                "product_reality_debt": (
                    "Unreal presentation and gameplay feel remain separate acceptance debt."
                ),
            }
        )

        component = adapted["components"]["Invariant Checks"]
        self.assertEqual(component["status"], "FAIL")
        self.assertIn("one regression", component["detail"])
        self.assertEqual(
            adapted["scenario_views"]["survival-loop-invariants"]["events"][1]["detail"],
            "FAIL",
        )

        report = {
            "commit": COMMIT,
            "scenario": "survival-loop-invariants",
            "seed": "240",
            "initial_state": {},
            "final_state": {},
            "invariants": {"storage_mass_conserved": "FAIL"},
            **adapted,
        }
        rendered = render_state_report(report)
        self.assertIn("Invariant Checks", rendered)
        self.assertIn("storage_mass_conserved", rendered)
        self.assertIn("Unreal presentation", rendered)

    def test_invariant_adapter_requires_authoritative_events(self) -> None:
        with self.assertRaisesRegex(ValueError, "events must be a list"):
            adapt_invariant_evidence(
                {
                    "commit": COMMIT,
                    "scenario": "survival-loop-invariants",
                    "timestamp": "2026-09-14T06:55:00Z",
                    "status": "PASS",
                }
            )


if __name__ == "__main__":
    unittest.main()
