# Everward Godot/Jolt 3D Physics Integration Contract

**Status:** EXPERIMENTAL, not production gameplay acceptance or an engine-change ADR.

**Visual standard:** cinematic scientific realism and faithful canonical Prime Probe A. Correct collision math does not make primitive graphics acceptable.

## Single source of truth

The existing deterministic C++ Everward simulation owns masses, velocities, inertia, impulse, resources, damage, tool-tip reach, mining, Fix_It repair, energy, saves, and time progression. Godot/Jolt owns local collision *candidate queries*, visual presentation and non-authoritative decorative debris. Never replace C++ outcomes with Jolt contact callbacks. Advance C++ in fixed simulation ticks, independently from render delta. Interpolate rendered geometry as appropriate.

## Risk → control → present verification

| Risk | Rule | Evidence |
| --- | --- | --- |
| Tunneling at high relative speed | CCD on appropriate RigidBody3D and swept collision checks, never endpoint-only probes | Godot verifies sphere cast across 16 m toward 4 m target |
| Invisible probe stand-off | Canonical five-sample compound envelope, accurate tool-tip pose and contact radius | C++ bridge verifies 40 poses, 360 coordinates; GLB import preserves pivots; gameplay PENDING |
| Free-space drag | gravity scale zero, linear/angular damping mode REPLACE, drag zero. Engine forces are explicit | Godot local configuration PASS; long trajectory PENDING |
| Sensor overhead / drone swarm | Query only relevant collision layers and time slices; event-driven scanner cadence | Query layers checked; many-actor benchmark PENDING |
| Shape instability | Simple convex/capsule/box/sphere collision proxies; avoid concave RigidBody, thin geometry, negative scaling | Full collider QA PENDING |
| Stale collision cache | Query after fixed step; do not treat force-update raycast as globally refreshing moved physics bodies | Stationary contact PASS; teleports PENDING |
| Invalid physics mutation | Do not set physics-controlled rigid-body transforms every render tick; use forces/impulses or _integrate_forces for decorative actors | Source rule only |
| Unstable joint chains | Authoritative C++ arm FK and reach, Jolt joints secondary and measured with varying masses/solver iterations | Geometry tests PASS, runtime constraints PENDING |
| Galactic precision | Keep high-precision long-range coordinates in C++; locally recenter Godot scene and collision world | Rebase and far-distance regression PENDING |
| Nondeterministic engine physics | Godot/Jolt contacts advisory, never replace deterministically replayable C++ results | Replay end-to-end PENDING |

## Tractor beam conservation law

Apply equal-and-opposite impulse between target and probe:
- Target delta_v = +J / m_target.
- Probe delta_v = -J / m_probe.
- With no external force, delta_p_target + delta_p_probe = 0, within solver tolerance.
- Torque on each body is r cross F; off-center attachments create angular effects.
- Engine thrust adds a separately conserved impulse from propellant/energy bookkeeping.
- p = m v is momentum; F = dp/dt is force. Mass times velocity is not force.
- Energy, capacity, heat and range limits belong to C++ and must be recorded.

Next required oracle: two unequal masses, moving/off-center beam, reversible force, no free energy, failed attempt, save/reload, and engine-thrust assist. Do not invent impulse magnitudes without a selected mechanic contract.

## Executed acceptance fixture

The isolated runner in this directory uses current source and native Godot 4.7.2. It builds the C++ geometry oracle with Visual Studio on Azathoth, tests 40 poses/360 coordinates, exports GLB, checks nine imported meshes and eight named pivots, and executes explicit Jolt Physics 3D tests for configuration, masking, ray/shape queries, no drag, CCD enabled and actual swept target contact. Its proof artifacts live outside any source or production workspace.

Do not count tests as proof of rendered fidelity or working end-to-end gameplay. The owner reviews only truly subjective milestones; deterministic checks are machine owned.

## Reference docs

- https://docs.godotengine.org/en/4.7/tutorials/physics/using_jolt_physics.html
- https://docs.godotengine.org/en/4.7/classes/class_rigidbody3d.html
- https://docs.godotengine.org/en/stable/tutorials/physics/large_world_coordinates.html
- Source: src/simulation/, tools/procedural_mechanics/prime-arm/layout.json, assets/reference/probe/gen1-prime/
