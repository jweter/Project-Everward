# Phase 1 Exit Gate

Everward must not begin Phase 2 production gameplay work merely because the repository contains all five technical-prototype directories. The Phase 1 gate exists to ensure that the accepted Unreal Engine production direction has been validated by representative technical evidence before gameplay architecture hardens around it.

Run:

```bash
python prototypes/phase1_exit_gate.py \
  --engine-decision path/to/phase1-engine-decision.json
```

The audit requires all five roadmap prototypes to be present:

- simulation clock,
- procedural star system,
- coordinate-scale handling,
- headless simulation,
- rendering benchmark.

It also requires a real artifact produced by `prototypes/rendering-benchmark/finalize_engine_decision.py` whose decision packet is `decision_ready` and recommends **`unreal`**.

A missing decision artifact, `additional_evidence_required`, an unknown recommendation, a recommendation other than `unreal`, a malformed artifact, or a missing prototype is a hard Phase 1 blocker. The command exits non-zero and explicitly states that Phase 2 — One Probe is not authorized.

A Godot capture may remain in the repository as comparative Phase 1 benchmark evidence, but Godot is not an alternate production direction under the current accepted decision. A benchmark result that does not validate Unreal triggers Unreal-specific blocker investigation or an explicit new ADR; it does not silently switch Everward to Godot.

This gate does not manufacture benchmark evidence and does not run either engine. It converts the roadmap's Phase 1 exit condition into an executable contract: **validate Unreal Engine and the simulation architecture with completed technical evidence before production gameplay begins.**

See `ENGINE_DIRECTION.md`, `TECHNOLOGY_DECISIONS.md`, and ADR-0001 in `DECISION_LOG.md` for the authoritative engine decision.