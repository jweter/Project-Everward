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

## Output

The packet records:

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

This is still technical-proof tooling. It does not complete Prototype C by itself. Everward still needs representative Godot and Unreal implementations of the canonical scene and real captured evidence before TD-001 can be closed and Phase 2 may begin.
