from __future__ import annotations

import unittest

from mobile_product_reality import render_state_report
from mobile_product_reality_adapters import adapt_material_consumption_evidence


class MaterialConsumptionEvidenceAdapterTests(unittest.TestCase):
    def _renderable_report(self, adapted: dict[str, object]) -> dict[str, object]:
        return {
            "commit": "abc123",
            "scenario": "material-consumption",
            "seed": "42",
            "initial_state": {},
            "final_state": {},
            "invariants": {"storage_conserved": True},
            **adapted,
        }

    def test_authoritative_material_consumption_evidence_renders_without_reimplementing_state(self) -> None:
        evidence = {
            "commit": "abc123",
            "scenario": "material-consumption",
            "timestamp": "2026-09-14T22:00:00Z",
            "status": "PASS",
            "events": [
                {
                    "at": "tick 120",
                    "event": "material plan accepted",
                    "detail": "requested material id satisfied by authoritative evidence",
                },
                {
                    "at": "tick 120",
                    "event": "storage invariant preserved",
                },
            ],
            "product_reality_debt": (
                "Player-facing material choice, repair UX, and Unreal interaction remain pending."
            ),
        }

        adapted = adapt_material_consumption_evidence(evidence)
        component = adapted["components"]["Material Consumption"]
        self.assertEqual(component["status"], "PASS")
        self.assertIn("Player-facing", component["product_reality_debt"])

        html = render_state_report(self._renderable_report(adapted))
        self.assertIn("Material Consumption", html)
        self.assertIn("material plan accepted", html)
        self.assertIn("storage invariant preserved", html)

    def test_product_reality_required_material_evidence_must_name_remaining_debt(self) -> None:
        evidence = {
            "commit": "abc123",
            "scenario": "material-consumption",
            "timestamp": "2026-09-14T22:00:00Z",
            "status": "PRODUCT_REALITY_REQUIRED",
            "events": [],
        }

        with self.assertRaisesRegex(ValueError, "must name remaining debt"):
            adapt_material_consumption_evidence(evidence)

    def test_material_evidence_fails_closed_without_authoritative_events(self) -> None:
        evidence = {
            "commit": "abc123",
            "scenario": "material-consumption",
            "timestamp": "2026-09-14T22:00:00Z",
            "status": "PASS",
        }

        with self.assertRaisesRegex(ValueError, "events must be a list"):
            adapt_material_consumption_evidence(evidence)


if __name__ == "__main__":
    unittest.main()
