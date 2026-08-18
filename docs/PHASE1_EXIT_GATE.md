# Phase 1 Exit Gate

Everward must not begin Phase 2 production gameplay work merely because the repository contains all five technical-prototype directories. The Phase 1 gate exists to prevent an unresolved engine decision from becoming accidental production architecture.

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

It also requires a real artifact produced by `prototypes/rendering-benchmark/finalize_engine_decision.py` whose decision packet is `decision_ready` and recommends either `godot` or `unreal`.

A missing decision artifact, `additional_evidence_required`, an unknown recommendation, malformed artifact, or missing prototype is a hard Phase 1 blocker. The command exits non-zero and explicitly states that Phase 2 — One Probe is not authorized.

This gate does not manufacture benchmark evidence and does not run either engine. It exists to convert the roadmap's Phase 1 exit condition into an executable contract: **choose the engine and simulation architecture from completed technical evidence before production gameplay begins.**
