from __future__ import annotations

import unittest

from mobile_product_reality import render_state_report
from mobile_product_reality_adapters import adapt_lineage_evidence


COMMIT = "f" * 40


class LineageAdapterTests(unittest.TestCase):
    def test_lineage_adapter_renders_authoritative_parent_successor_state(self) -> None:
        adapted = adapt_lineage_evidence(
            {
                "commit": COMMIT,
                "scenario": "lineage-successor",
                "timestamp": "2026-09-14T20:30:00Z",
                "status": "PASS",
                "detail": "Authoritative parent/successor lineage evidence was preserved.",
                "events": [
                    {"at": "generation-1", "event": "parent", "detail": "probe-prime"},
                    {
                        "at": "generation-2",
                        "event": "successor",
                        "detail": "probe-child-01 parent=probe-prime",
                    },
                ],
                "product_reality_debt": (
                    "Unreal successor construction, presentation, and embodied interaction remain pending."
                ),
            }
        )

        component = adapted["components"]["Lineage/Evolution"]
        self.assertEqual(component["status"], "PASS")
        self.assertEqual(component["scenario"], "lineage-successor")
        self.assertIn("successor construction", component["product_reality_debt"])
        self.assertEqual(
            adapted["scenario_views"]["lineage-successor"]["events"][1]["detail"],
            "probe-child-01 parent=probe-prime",
        )

        report = {
            "commit": COMMIT,
            "scenario": "lineage-successor",
            "seed": "42",
            "initial_state": {},
            "final_state": {},
            "invariants": {},
            **adapted,
        }
        rendered = render_state_report(report)
        self.assertIn("Lineage/Evolution", rendered)
        self.assertIn("probe-child-01", rendered)
        self.assertIn("successor construction", rendered)

    def test_lineage_adapter_requires_authoritative_events(self) -> None:
        with self.assertRaisesRegex(ValueError, "events must be a list"):
            adapt_lineage_evidence(
                {
                    "commit": COMMIT,
                    "scenario": "lineage-successor",
                    "timestamp": "2026-09-14T20:30:00Z",
                    "status": "PASS",
                }
            )


if __name__ == "__main__":
    unittest.main()
