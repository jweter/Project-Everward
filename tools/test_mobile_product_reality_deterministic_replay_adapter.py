from __future__ import annotations

import unittest

from mobile_product_reality import render_state_report
from mobile_product_reality_adapters import adapt_deterministic_replay_evidence


COMMIT = "d" * 40


class DeterministicReplayMobileEvidenceAdapterTests(unittest.TestCase):
    def test_replay_adapter_renders_authoritative_seed_and_replay_evidence(self) -> None:
        adapted = adapt_deterministic_replay_evidence(
            {
                "commit": COMMIT,
                "scenario": "seeded-replay-v1",
                "timestamp": "2026-09-14T03:34:00Z",
                "status": "PASS",
                "detail": "Authoritative deterministic lane reproduced the same seeded result.",
                "events": [
                    {
                        "at": "seed 847291",
                        "event": "replay identity verified",
                        "detail": "Same seed and horizon reproduced the authoritative state.",
                    },
                    {
                        "at": "checkpoint 1",
                        "event": "checkpoint continuation matched uninterrupted execution",
                    },
                ],
            }
        )
        report = {
            "commit": COMMIT,
            "scenario": "seeded-replay-v1",
            "seed": "847291",
            "initial_state": {},
            "final_state": {},
            "invariants": {},
            **adapted,
        }

        card = adapted["components"]["Deterministic Replay"]
        self.assertEqual(card["status"], "PASS")
        self.assertEqual(card["commit"], COMMIT)
        html = render_state_report(report)
        self.assertIn("Deterministic Replay", html)
        self.assertIn("replay identity verified", html)
        self.assertIn("checkpoint continuation matched uninterrupted execution", html)

    def test_replay_adapter_keeps_unreal_only_debt_explicit(self) -> None:
        adapted = adapt_deterministic_replay_evidence(
            {
                "commit": COMMIT,
                "scenario": "seeded-replay-v1",
                "timestamp": "2026-09-14T03:35:00Z",
                "status": "PRODUCT_REALITY_REQUIRED",
                "product_reality_debt": "Unreal presentation of replay-derived state",
                "events": [],
            }
        )

        card = adapted["components"]["Deterministic Replay"]
        self.assertEqual(card["status"], "PRODUCT_REALITY_REQUIRED")
        self.assertEqual(
            card["product_reality_debt"], "Unreal presentation of replay-derived state"
        )


if __name__ == "__main__":
    unittest.main()
