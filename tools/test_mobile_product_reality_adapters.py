from __future__ import annotations

import unittest

from mobile_product_reality import render_state_report
from mobile_product_reality_adapters import (
    adapt_collision_damage_evidence,
    adapt_manipulator_evidence,
    adapt_mining_storage_evidence,
    adapt_persistence_evidence,
    adapt_repair_evidence,
    adapt_scanner_target_evidence,
    adapt_tractor_field_evidence,
)


COMMIT = "a" * 40


def _renderable_report(adapted: dict, scenario: str) -> dict:
    return {
        "commit": COMMIT,
        "scenario": scenario,
        "seed": "17",
        "initial_state": {},
        "final_state": {},
        "invariants": {},
        **adapted,
    }


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
        report = _renderable_report(adapted, "save-round-trip-v1")

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
        html = render_state_report(_renderable_report(adapted, "mining-storage-v1"))
        self.assertIn("PRODUCT REALITY REQUIRED", html)
        self.assertIn("Unreal mining feedback readability", html)

    def test_repair_adapter_renders_authoritative_fix_it_evidence(self) -> None:
        adapted = adapt_repair_evidence(
            {
                "commit": COMMIT,
                "scenario": "fix-it-repair-v1",
                "timestamp": "2026-09-13T15:00:00Z",
                "status": "PASS",
                "detail": "Authoritative Fix_It execution restored required capability.",
                "events": [
                    {
                        "at": "tick 210",
                        "event": "repair material consumed",
                        "detail": "Material breakdown supplied by simulation authority.",
                    },
                    {"at": "tick 240", "event": "required capability restored"},
                ],
            }
        )

        card = adapted["components"]["Fix_It Repair"]
        self.assertEqual(card["status"], "PASS")
        html = render_state_report(_renderable_report(adapted, "fix-it-repair-v1"))
        self.assertIn("Fix_It Repair", html)
        self.assertIn("repair material consumed", html)
        self.assertIn("required capability restored", html)

    def test_tractor_adapter_renders_authoritative_coupling_evidence(self) -> None:
        adapted = adapt_tractor_field_evidence(
            {
                "commit": COMMIT,
                "scenario": "tractor-heavy-target-v1",
                "timestamp": "2026-09-13T16:00:00Z",
                "status": "PASS",
                "detail": "Simulation authority reported equal-and-opposite tractor coupling.",
                "events": [
                    {
                        "at": "tick 80",
                        "event": "tractor coupled",
                        "detail": "Heavier target caused stronger probe acceleration toward target.",
                    }
                ],
            }
        )

        card = adapted["components"]["Tractor Field"]
        self.assertEqual(card["status"], "PASS")
        html = render_state_report(_renderable_report(adapted, "tractor-heavy-target-v1"))
        self.assertIn("Tractor Field", html)
        self.assertIn("tractor coupled", html)
        self.assertIn("stronger probe acceleration", html)

    def test_scanner_adapter_renders_authoritative_target_classification_evidence(self) -> None:
        adapted = adapt_scanner_target_evidence(
            {
                "commit": COMMIT,
                "scenario": "scanner-target-classification-v1",
                "timestamp": "2026-09-13T18:00:00Z",
                "status": "PRODUCT_REALITY_REQUIRED",
                "detail": "Simulation authority supplied full-confidence target composition and classification.",
                "product_reality_debt": "Unreal scan presentation readability",
                "events": [
                    {
                        "at": "scan 100%",
                        "event": "target classification revealed",
                        "detail": "Composition and classification came from authoritative scan state.",
                    }
                ],
            }
        )

        card = adapted["components"]["Scanner/Target"]
        self.assertEqual(card["status"], "PRODUCT_REALITY_REQUIRED")
        self.assertEqual(card["product_reality_debt"], "Unreal scan presentation readability")
        html = render_state_report(
            _renderable_report(adapted, "scanner-target-classification-v1")
        )
        self.assertIn("Scanner/Target", html)
        self.assertIn("target classification revealed", html)
        self.assertIn("Unreal scan presentation readability", html)

    def test_manipulator_adapter_renders_authoritative_interaction_evidence(self) -> None:
        adapted = adapt_manipulator_evidence(
            {
                "commit": COMMIT,
                "scenario": "manipulator-grasp-move-v1",
                "timestamp": "2026-09-13T20:00:00Z",
                "status": "PRODUCT_REALITY_REQUIRED",
                "detail": "Deterministic manipulator tests supplied deploy, reach, grasp, and move state.",
                "product_reality_debt": "Unreal manipulator geometry, alignment, and control feel",
                "events": [
                    {
                        "at": "tick 150",
                        "event": "target grasped",
                        "detail": "Grasp state supplied by authoritative manipulator runtime evidence.",
                    },
                    {"at": "tick 151", "event": "held target followed wrist state"},
                ],
            }
        )

        card = adapted["components"]["Manipulator"]
        self.assertEqual(card["status"], "PRODUCT_REALITY_REQUIRED")
        self.assertEqual(
            card["product_reality_debt"],
            "Unreal manipulator geometry, alignment, and control feel",
        )
        html = render_state_report(_renderable_report(adapted, "manipulator-grasp-move-v1"))
        self.assertIn("Manipulator", html)
        self.assertIn("target grasped", html)
        self.assertIn("held target followed wrist state", html)
        self.assertIn("Unreal manipulator geometry, alignment, and control feel", html)

    def test_collision_damage_adapter_renders_authoritative_impact_evidence(self) -> None:
        adapted = adapt_collision_damage_evidence(
            {
                "commit": COMMIT,
                "scenario": "collision-damage-v1",
                "timestamp": "2026-09-13T19:00:00Z",
                "status": "PRODUCT_REALITY_REQUIRED",
                "detail": "Deterministic simulation reported impact severity and component integrity changes.",
                "product_reality_debt": "Unreal collision feel and damage presentation",
                "events": [
                    {
                        "at": "tick 310",
                        "event": "impact damage applied",
                        "detail": "Component consequence supplied by authoritative simulation evidence.",
                    },
                    {"at": "tick 311", "event": "subsystem capability updated"},
                ],
            }
        )

        card = adapted["components"]["Collision/Damage"]
        self.assertEqual(card["status"], "PRODUCT_REALITY_REQUIRED")
        self.assertEqual(
            card["product_reality_debt"],
            "Unreal collision feel and damage presentation",
        )
        html = render_state_report(_renderable_report(adapted, "collision-damage-v1"))
        self.assertIn("Collision/Damage", html)
        self.assertIn("impact damage applied", html)
        self.assertIn("Unreal collision feel and damage presentation", html)

    def test_adapter_accepts_and_renders_canonical_unattended_worker_review_statuses(self) -> None:
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
                html = render_state_report(
                    _renderable_report(adapted, "save-round-trip-v1")
                )
                self.assertIn(status, html)

    def test_adapter_accepts_and_renders_machine_style_product_reality_status(self) -> None:
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
        html = render_state_report(_renderable_report(adapted, "mining-storage-v1"))
        self.assertIn("PRODUCT_REALITY_REQUIRED", html)
        self.assertIn("Unreal mining feedback readability", html)

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
