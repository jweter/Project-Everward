# Headless Simulation Prototype

## Purpose

Prove that Everward's authoritative simulation can run without graphics and can advance very long time horizons efficiently and deterministically.

## Minimum proof

- run a fixed scenario from a seed,
- execute without presentation/rendering,
- advance thousands of simulated years,
- emit a canonical state summary/hash,
- repeat with the same result,
- save/load during the run and verify equivalent final state,
- measure simulation throughput and memory growth.

## Desired developer interface

Conceptually:

```text
everward-sim --years 10000 --seed 847291
```

The exact executable, language, and command surface remain open until the architecture/engine decision.

## Acceptance criteria

Headless execution is a supported architecture path, not a hacked test mode coupled to a running renderer.
