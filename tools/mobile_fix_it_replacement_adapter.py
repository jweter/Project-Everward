"""Presentation-only adapter for authoritative Fix_It replacement evidence."""

from __future__ import annotations

from typing import Any, Mapping

from mobile_product_reality_adapters import adapt_repair_evidence


def adapt_fix_it_replacement_evidence(evidence: Mapping[str, Any]) -> dict[str, Any]:
    """Expose authoritative replacement/fabrication evidence without simulating it.

    Fix_It replacement execution remains authoritative in the deterministic simulation.
    This adapter only reshapes already-produced evidence for the mobile review surface;
    it cannot select material, debit inventory, fabricate a component, or clear Unreal
    Product Reality debt.
    """

    prepared = dict(evidence)
    prepared.setdefault("label", "Fix_It replacement fabrication and recovery")
    report = adapt_repair_evidence(prepared)
    component = report["components"].pop("Fix_It Repair")
    report["components"]["Fix_It Replacement"] = component
    return report
