from __future__ import annotations

import unittest

from mobile_product_reality import render_state_report
from mobile_product_reality_adapters import (
    adapt_mining_storage_evidence,
    adapt_persistence_evidence,
)


COMMIT = "a" * 40


class MobileProductRealityAdapterTests(unittest.TestCase):
    def test_persistence_adapter_is_renderer_compatible(self) -> None:
        adapted = adapt_persistence_evidence(
            {
                "commit": COMMIT,
                "scenario": "save-round-trip-v1",
                "timestamp": "2026-09-13T14:00:00Z",
                "status": "PASS",
                "detail": "Canonical save/load test preserved authoritative state.",
                "events": [
                    {"at": "tick 120", "event": "save written"},
                    {"at": "tick 120", "event": "save reloaded", "detail": "State matched."},
                ],
            }
        )
        report = {
            "commit": COMMIT,
            "scenario": "save-round-trip-v1",
            "seed": "17",
            "initial_state": {},
            "final_state": {},
            "invariants": {"round_trip_preserved": True},
            **adapted,
        }

        html = render_state_report(report)

        self.assertIn("Save/Load", html)
        self.assertIn("Canonical save/load test preserved authoritative state.", html)
        self.assertIn("save reloaded", html)

    def test_mining_adapter_preserves_product_reality_debt(self) -> None:
        adapted = adapt_mining_storage_evidence(
            {
                "commit": COMMIT,
                "scenario": "mining-storage-v1",
                "timestamp": "2026-09-13T14:00:00Z",
                "status": "PRODUCT REALITY REQUIRED",
                "product_reality_debt": "Unreal mining feedback readability",
                "events": [{"at": "tick 44", "event": "stored mass updated"}],
            }
        )

        card = adapted["components"]["Mining/Storage"]
        self.assertEqual(card["status"], "PRODUCT REALITY REQUIRED")
        self.assertEqual(card["product_reality_debt"], "Unreal mining feedback readability")

    def test_adapter_accepts_canonical_unattended_worker_review_statuses(self) -> None:
        for status in ("REVIEW_REQUIRED", "ENVIRONMENT_FAILURE"):
            with self.subTest(status=status):
                adapted = adapt_persistence_evidence(
                    {
                        "commit": COMMIT,
                        "scenario": "save-round-trip-v1",
                        "timestamp": "2026-09-13T14:00:00Z",
                        "status": status,
                        "events": [],
                    }
                )
                self.assertEqual(adapted["components"]["Save/Load"]["status"], status)

    def test_adapter_accepts_machine_style_product_reality_status(self) -> None:
        adapted = adapt_mining_storage_evidence(
            {
                "commit": COMMIT,
                "scenario": "mining-storage-v1",
                "timestamp": "2026-09-13T14:00:00Z",
                "status": "PRODUCT_REALITY_REQUIRED",
                "product_reality_debt": "Unreal mining feedback readability",
                "events": [],
            }
        )
        self.assertEqual(
            adapted["components"]["Mining/Storage"]["status"],
            "PRODUCT_REALITY_REQUIRED",
        )

    def test_adapter_rejects_unknown_status_instead_of_inventing_truth(self) -> None:
        with self.assertRaisesRegex(ValueError, "invalid authoritative status"):
            adapt_persistence_evidence(
                {
                    "commit": COMMIT,
                    "scenario": "save-round-trip-v1",
                    "timestamp": "2026-09-13T14:00:00Z",
                    "status": "UNKNOWN",
                    "events": [],
                }
            )

    def test_adapter_requires_structured_authoritative_events(self) -> None:
        with self.assertRaisesRegex(ValueError, "authoritative events must be a list"):
            adapt_mining_storage_evidence(
                {
                    "commit": COMMIT,
                    "scenario": "mining-storage-v1",
                    "timestamp": "2026-09-13T14:00:00Z",
                    "status": "PASS",
                    "events": "stored mass updated",
                }
            )

    def test_product_reality_required_must_name_debt(self) -> None:
        for status in ("PRODUCT REALITY REQUIRED", "PRODUCT_REALITY_REQUIRED"):
            with self.subTest(status=status):
                with self.assertRaisesRegex(ValueError, "must name remaining debt"):
                    adapt_mining_storage_evidence(
                        {
                            "commit": COMMIT,
                            "scenario": "mining-storage-v1",
                            "timestamp": "2026-09-13T14:00:00Z",
                            "status": status,
                            "events": [],
                        }
                    )


if __name__ == "__main__":
    unittest.main()
