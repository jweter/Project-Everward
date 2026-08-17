# Coordinate Scale Prototype

## Purpose

Prove that Everward can represent and interact across meter-scale machinery, kilometer-scale local environments, AU-scale star systems, and light-year-scale interstellar positions without unacceptable floating-point precision failure.

## Questions

- hierarchical coordinates, floating origin, or another strategy?
- where do coordinate-domain boundaries live?
- how are deterministic astronomical trajectories represented?
- how do rendering coordinates map to authoritative simulation coordinates?
- how are collisions/local physics isolated from interstellar-scale values?

## Acceptance criteria

- stable local movement and machinery,
- no visible jitter in representative local scenes,
- stable system-scale trajectories,
- stable interstellar positions,
- clean transitions between coordinate domains,
- headless simulation and rendered presentation agree on authoritative positions.

Do not select the final spatial strategy before this proof is measured in both engine candidates where relevant.
