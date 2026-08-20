# Coordinate Scale Proof — Authoritative Position Model

## Scope

This slice tests one architectural boundary only: whether Everward can retain exact local displacement while entities occupy interstellar-scale absolute positions.

It does **not** select the final engine strategy, floating-origin implementation, collision system, or render transform hierarchy. Those still require engine-specific measurement.

## Candidate model

Authoritative simulation position is represented as:

- signed integer spatial-cell coordinates,
- normalized integer millimetre offsets within each cell,
- exact integer arithmetic for translation and subtraction,
- floating-point conversion only after subtracting a nearby presentation origin.

The current prototype uses 1e12-metre cells. The cell size is deliberately a prototype parameter, not a production constant.

## Properties under test

The automated tests verify:

1. exact round-trip storage across hundreds of light-years,
2. correct normalization of negative coordinates,
3. one-millimetre movement at extremely large absolute positions,
4. local render coordinates produced only after exact rebasing,
5. continuity while crossing cell boundaries,
6. rejection of a non-normalized offset on **every** axis, below the cell and at
   or above the exclusive cell bound, plus acceptance of the largest normalized
   offset on every axis,
7. rejection of a wrong axis count for **every** declared vector field
   (`VECTOR_FIELDS`) and at every entry point that accepts a 3-vector
   (`SpatialPosition`, `from_total_mm`, `translated_mm`).

The declared shape contract itself is expressed as the module-level `AXIS_COUNT`
and `VECTOR_FIELDS` constants in `coordinates.py`, and both are pinned by test so
a field or axis silently dropped from the contract fails CI rather than
shrinking the per-axis loops that iterate them.

## Interpretation

If these tests remain green, hierarchical exact simulation coordinates are a viable candidate for Everward's authoritative state model. Engine-specific prototypes must still prove that local physics and rendering can consume rebased coordinates without visible jitter and that transitions between coordinate domains remain operationally simple.

No production architecture decision is recorded by this proof alone.
