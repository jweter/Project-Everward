# Rendering Benchmark Decision Packet

This layer turns the already-validated Phase 1 rendering evidence into a single auditable engine-decision packet.

It deliberately does **not** invent benchmark measurements, choose an engine without complete evidence, or allow a near-tie to become a preference-based architecture decision.

## Inputs

`build_decision_packet(...)` consumes:

- the canonical rendering scenario,
- one validated Godot run record,
- one validated Unreal run record,
- evidence-bearing qualitative assessments for both engines.

The existing normalization layer derives objective scores from raw capture fields and preserves qualitative evidence. The existing weighted comparison contract then evaluates the complete 13-metric result.

Before scoring, `validate_capture_pair(...)` should be used to prove that the two records are genuinely comparable. It rejects pairs unless both captures share:

- the same source revision,
- the same canonical handoff SHA-256 digest,
- the same operating-system version,
- the same CPU model,
- the same GPU model,
- the same installed RAM quantity,
- the same canonical scenario identity.

This is an evidence-integrity boundary, not a scoring rule. A faster result from different hardware or a different source revision is not decision-grade Everward engine evidence.

Each qualitative evidence document used by the finalizer is engine-bound and has this shape:

```json
{
  "assessment_version": 1,
  "engine": "godot",
  "metrics": {
    "visual_fidelity": {
      "score": 8.0,
      "evidence": "A/B screenshot review and notes"
    }
  }
}
```

`metrics` must contain exactly every qualitative metric declared by `normalization.QUALITATIVE_METRICS`; every entry must contain `score` and `evidence`. The normalizer remains responsible for validating the 0..10 score range and non-empty evidence.

## Finalization command

After both engine captures have been assembled into complete run records, the pair-provenance gate has passed, and qualitative reviews are complete, generate the single decision artifact with:

```text
python prototypes/rendering-benchmark/finalize_engine_decision.py \
  --scenario <canonical-scenario.json> \
  --left-record <godot-run-record.json> \
  --right-record <unreal-run-record.json> \
  --left-qualitative <godot-qualitative.json> \
  --right-qualitative <unreal-qualitative.json> \
  --output <phase1-engine-decision.json>
```

The command rejects qualitative evidence assigned to the wrong engine, incomplete/unknown qualitative metric sets, invalid scenario data, invalid run records, and any downstream normalization or comparison contract violation. It does not mutate the evidence inputs.

## Output

The final artifact records:

- artifact type/version and the exact evidence paths supplied to finalization,
- scenario identity,
- normalized weighted comparison,
- decision status,
- recommendation when evidence is decisive,
- the five largest weighted differentiators,
- all metric deltas,
- the complete normalization audit,
- structured fields suitable for the final `DECISION_LOG.md` entry.

## Decision behavior

If the weighted score difference is at least the benchmark contract's declared decision threshold, the packet is marked `decision_ready` and names the leading engine.

If the result falls inside the tie threshold, the packet is marked `additional_evidence_required` and contains no engine recommendation. The correct Phase 1 response is then to gather better representative evidence rather than force a winner.

## Scope boundary

This is still technical-proof tooling. It does not complete Prototype C by itself. Everward still needs representative Godot and Unreal executions on the same benchmark hardware plus real captured evidence before TD-001 can be closed and Phase 2 may begin.
