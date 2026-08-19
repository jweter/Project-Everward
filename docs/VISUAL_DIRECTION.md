# Visual Direction

## Target

Everward aims for **cinematic scientific realism**: physically legible and grounded enough to feel credible, visually rich enough that exploration itself is a reward.

This is a first-class product requirement, not optional polish. Everward should be built as a high-fidelity 3D space experience in which the player feels physically present as the probe.

The accepted production direction is Unreal Engine. See `ENGINE_DIRECTION.md` and `TECHNOLOGY_DECISIONS.md`.

The project should avoid four failure modes:

- sterile visualization that makes space feel like a spreadsheet;
- arbitrary fantasy space that disconnects visual spectacle from mechanics;
- a primarily 2D/2.5D or abstract-map presentation that replaces physical presence with icons and panels;
- deliberately quirky, low-fidelity, or lightweight visuals chosen mainly because they are easier to implement.

Strategic maps, overlays, telemetry, lineage views, engineering diagrams, and schematic interfaces remain important, but they should support and contextualize the physical 3D universe rather than substitute for it.

## Visual promise

A successful Everward scene should sometimes make the player stop accelerating time, hide the HUD, and simply watch.

The long-term screenshot target is:

> Pause. Hide HUD. Take screenshot. Use as wallpaper.

The player should be able to watch mining equipment operating against a gas giant, cross a ring plane, orbit a cryovolcanic moon, approach a neutron star, or stare into a black-hole accretion environment and feel that the universe itself justified the journey.

## Required scales

### Galactic scale

Shows stars, long-distance trajectories, communications, lineages, exploration frontiers, and strategic expansion.

This scale may use abstraction where necessary for comprehension, but it must remain visually connected to the physical universe rather than becoming the game's dominant identity.

### System scale

Shows stars, planets, moons, belts, stations, probes, trajectories, orbits, and major environmental conditions in spatially meaningful 3D context.

### Local cinematic scale

Shows the embodied machine, mining operations, stations, planetary orbit, ice fields, ring systems, stellar events, and other environments at a scale where machinery and physical presence matter.

This is a core Everward experience, not merely a cutscene layer.

Transitions between scales should preserve orientation and continuity whenever possible.

## Astronomical priorities

The universe should eventually support visually and mechanically meaningful examples of:

- ring systems,
- gas giants,
- asteroid and ice fields,
- active stellar surfaces,
- coronal eruptions,
- eclipses,
- magnetospheric phenomena,
- cryovolcanic moons,
- binary systems,
- white dwarfs,
- neutron stars,
- pulsars,
- black holes and accretion environments,
- rogue planets,
- stellar remnants,
- dust structures,
- stellar nurseries,
- unusual atmospheres,
- and other scientifically motivated extreme environments.

Visual appearance should arise from the same physical properties that affect scanning, navigation, thermal load, radiation exposure, communications, research opportunity, and resource access.

## Quiet space matters

Not every scene should announce spectacle. Darkness, distance, isolation, sparse instrumentation, and long periods of calm are part of the identity.

A mining operation near a slowly rotating gas giant can be as important as a black-hole encounter.

Cinematic does not mean constant visual noise. Scale, light, motion, silence, and restraint should do much of the work.

## HUD as sensorium

The HUD represents how the probe perceives and operates, not a generic strategy-game overlay.

Likely major interfaces:

- spatial view,
- navigation,
- scanning,
- communications,
- engineering,
- software/programming,
- manufacturing,
- resource analysis,
- research,
- descendant management,
- alerts,
- historical log.

At minimum the game should ultimately support hiding the HUD, selective panel visibility, scaling, major-panel repositioning, and a photo/screenshot mode.

## Unreal production direction

Unreal Engine is the intended production presentation/runtime engine because Everward's visual identity requires a serious path toward high-end real-time 3D rendering, lighting, materials, volumetrics, particles, cinematic cameras, and large-environment presentation.

The engine must not own independent mechanical truth. Simulation state remains authoritative and presentation consumes it.

## Engine benchmark scene

The Phase 1 benchmark remains a representative Everward visual proof:

> A probe mining an icy asteroid near a large planet, with stellar lighting, volumetric effects, particles, moving machinery, interactive HUD telemetry, and accelerated time.

The benchmark should continue to measure:

- visual fidelity,
- frame rate,
- memory use,
- development complexity,
- procedural scene creation,
- UI capability,
- simulation integration,
- large-coordinate behavior,
- and scalability.

The benchmark's role is now to validate Unreal and expose material technical risk. Godot remains useful comparative evidence, but a simpler or lighter implementation does not by itself override the accepted cinematic 3D product direction.