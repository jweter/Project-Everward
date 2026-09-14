from __future__ import annotations

import json
import tempfile
import unittest
from pathlib import Path

from mobile_product_reality_review_result import (
    append_review_result,
    build_review_result,
    load_review_results,
)


COMMIT = "a" * 40
EVIDENCE = {
    "commit": COMMIT,
    "scenario": "lineage-review-v1",
    "seed": "42",
}


class MobileProductRealityReviewResultTests(unittest.TestCase):
    def test_build_result_binds_verdict_to_authoritative_identity(self) -> None:
        record = build_review_result(
            EVIDENCE,
            result="PASS",
            reviewed_at_utc="2026-09-14T21:20:00-05:00",
            notes="Readable on phone.",
        )

        self.assertEqual(record["commit"], COMMIT)
        self.assertEqual(record["scenario"], "lineage-review-v1")
        self.assertEqual(record["seed"], "42")
        self.assertEqual(record["result"], "PASS")
        self.assertEqual(record["reviewed_at_utc"], "2026-09-15T02:20:00Z")
        self.assertEqual(record["acceptance_scope"], "mobile_deterministic_evidence_only")
        self.assertIs(record["unreal_product_reality_cleared"], False)
        self.assertEqual(len(record["evidence_identity_sha256"]), 64)

    def test_invalid_result_fails_closed(self) -> None:
        with self.assertRaisesRegex(ValueError, "PASS, FAIL, or FLAG"):
            build_review_result(
                EVIDENCE,
                result="MAYBE",
                reviewed_at_utc="2026-09-15T02:20:00Z",
            )

    def test_naive_timestamp_fails_closed(self) -> None:
        with self.assertRaisesRegex(ValueError, "include a timezone"):
            build_review_result(
                EVIDENCE,
                result="FLAG",
                reviewed_at_utc="2026-09-15T02:20:00",
            )

    def test_append_is_history_preserving(self) -> None:
        first = build_review_result(
            EVIDENCE,
            result="FAIL",
            reviewed_at_utc="2026-09-15T02:20:00Z",
            notes="Text too small.",
        )
        second = build_review_result(
            EVIDENCE,
            result="PASS",
            reviewed_at_utc="2026-09-15T02:25:00Z",
            notes="Readable after local presentation adjustment.",
        )

        with tempfile.TemporaryDirectory() as temp_dir:
            path = Path(temp_dir) / "mobile-review" / "results.jsonl"
            append_review_result(path, first)
            append_review_result(path, second)
            records = load_review_results(path)

        self.assertEqual([record["result"] for record in records], ["FAIL", "PASS"])
        self.assertEqual(records[0]["notes"], "Text too small.")
        self.assertEqual(records[1]["notes"], "Readable after local presentation adjustment.")

    def test_tampered_identity_hash_is_rejected_on_read(self) -> None:
        record = build_review_result(
            EVIDENCE,
            result="FLAG",
            reviewed_at_utc="2026-09-15T02:20:00Z",
        )
        record["scenario"] = "different-scenario"

        with tempfile.TemporaryDirectory() as temp_dir:
            path = Path(temp_dir) / "results.jsonl"
            path.write_text(json.dumps(record) + "\n", encoding="utf-8")
            with self.assertRaisesRegex(ValueError, "identity hash"):
                load_review_results(path)

    def test_mobile_result_cannot_claim_unreal_acceptance(self) -> None:
        record = build_review_result(
            EVIDENCE,
            result="PASS",
            reviewed_at_utc="2026-09-15T02:20:00Z",
        )
        record["unreal_product_reality_cleared"] = True

        with tempfile.TemporaryDirectory() as temp_dir:
            path = Path(temp_dir) / "results.jsonl"
            path.write_text(json.dumps(record) + "\n", encoding="utf-8")
            with self.assertRaisesRegex(ValueError, "must not clear Unreal"):
                load_review_results(path)


if __name__ == "__main__":
    unittest.main()
