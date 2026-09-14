from __future__ import annotations

import unittest

from mobile_fix_it_replacement_adapter import adapt_fix_it_replacement_evidence


class MobileFixItReplacementAdapterTests(unittest.TestCase):
    def test_replacement_evidence_is_presented_without_clearing_unreal_debt(self) -> None:
        report = adapt_fix_it_replacement_evidence(
            {
                "commit": "a" * 40,
                "scenario": "fix-it-replacement",
                "timestamp": "2026-09-14T23:40:00Z",
                "status": "PRODUCT_REALITY_REQUIRED",
                "detail": "Authoritative replacement executor completed deterministically.",
                "product_reality_debt": "Unreal fabrication visuals and interaction remain pending.",
                "events": [
                    {
                        "at": "tick:120",
                        "event": "fix_it_replacement_completed",
                        "detail": "Replacement fabricated and installed.",
                    }
                ],
            }
        )

        component = report["components"]["Fix_It Replacement"]
        self.assertEqual(component["status"], "PRODUCT_REALITY_REQUIRED")
        self.assertIn("Unreal", component["product_reality_debt"])
        scenario = report["scenario_views"]["fix-it-replacement"]
        self.assertEqual(scenario["events"][0]["event"], "fix_it_replacement_completed")

    def test_missing_authoritative_events_fail_closed(self) -> None:
        with self.assertRaisesRegex(ValueError, "authoritative events must be a list"):
            adapt_fix_it_replacement_evidence(
                {
                    "commit": "b" * 40,
                    "scenario": "fix-it-replacement",
                    "timestamp": "2026-09-14T23:40:00Z",
                    "status": "PASS",
                }
            )

    def test_product_reality_required_must_name_remaining_debt(self) -> None:
        with self.assertRaisesRegex(ValueError, "must name remaining debt"):
            adapt_fix_it_replacement_evidence(
                {
                    "commit": "c" * 40,
                    "scenario": "fix-it-replacement",
                    "timestamp": "2026-09-14T23:40:00Z",
                    "status": "PRODUCT_REALITY_REQUIRED",
                    "events": [],
                }
            )


if __name__ == "__main__":
    unittest.main()
