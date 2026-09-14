# Mobile Product Reality review results

Issue #216 reserves human attention for facts that require actual phone judgment while keeping deterministic simulation/state evidence machine-owned. This helper records that explicit human verdict locally without turning it into simulation truth or Unreal acceptance.

## Command

```text
python tools/mobile_product_reality_review_result.py \
  --evidence path/to/generated-evidence.json \
  --result PASS \
  --reviewed-at-utc 2026-09-15T02:20:00Z \
  --output path/outside/the/repository/mobile-review-results.jsonl
```

`--result` is exactly `PASS`, `FAIL`, or `FLAG`. The evidence document must provide the authoritative `commit`, `scenario`, and `seed` fields already required by the mobile review surface. Each appended row binds the verdict to those fields with a deterministic SHA-256 identity digest.

## Safety boundary

The result history is append-only and caller-selected. Keep real review history outside the repository because notes can contain private Product Reality context. Do not commit generated result files.

A recorded mobile PASS means only that the reviewer accepted the deterministic evidence assigned to the phone review. Every record carries `acceptance_scope=mobile_deterministic_evidence_only` and `unreal_product_reality_cleared=false`. The loader rejects a record that tries to claim otherwise.

This mechanism does **not** verify or clear Unreal rendering, animation, mechanical articulation, HUD placement/readability in the real game, collision/game feel, camera behavior, packaged-build behavior, performance, manipulator alignment, or any other engine-specific acceptance gate.

## Fail-closed behavior

The ingestion helper rejects malformed or unsupported schema versions, missing commit/scenario/seed identity, non-PASS/FAIL/FLAG verdicts, timestamps without timezone information, tampered identity hashes, and any record claiming that a phone review cleared Unreal Product Reality. Malformed history rows stop the read rather than being silently skipped.
