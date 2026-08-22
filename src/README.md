# Source

Phase 2 production code now lives here.

`src/simulation/` is the engine-independent authoritative simulation core. It is C++20, buildable headlessly with CMake/CTest, and contains no Unreal headers, macros, containers, or object types. Unreal presentation code consumes this core only through the single adapter boundary defined by ADR-0012 and `docs/PHASE2_KICKOFF_SCAFFOLD.md`.

Current Phase 2 kickoff scope is intentionally small: simulation time, the first canonical probe snapshot, deterministic fixed-step movement integration, and domain-event delivery. Additional modules such as astronomy, industry, research, messages, lineage, and autonomous agency remain gated by their roadmap phases.

Prototype code under `prototypes/` remains historical technical evidence and is not silently promoted into production. Production implementations must preserve the separation between authoritative simulation, application/orchestration, presentation, and persistence described in `docs/ARCHITECTURE.md`.
