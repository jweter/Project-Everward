# Everward World-Building Craft Playbook

## Purpose

Everward is being built by a small team around an unusually large visual and simulation ambition. We should not reinvent mature environment-art, level-design, procedural-generation, lighting, and world-building practice from scratch.

This document is the project's living craft-learning layer:

1. find proven professional techniques;
2. understand the underlying principle rather than copying a specific game's look;
3. test whether the principle fits Everward;
4. prototype it cheaply;
5. measure visual, gameplay, and performance impact;
6. promote successful techniques into canonical standards;
7. reject techniques that conflict with scientific realism, simulation truth, scalability, or the game's identity.

A useful external technique is not automatically an Everward requirement. Adoption requires evidence.

## Source hierarchy

Prefer sources in roughly this order:

1. current Epic/Unreal Engine documentation for engine behavior and production constraints;
2. GDC talks and postmortems from shipped games for production practice;
3. technical art / level-design talks from experienced developers;
4. published books, papers, and studio breakdowns;
5. community tips only when we can explain, reproduce, and test the underlying principle.

Community advice can be excellent. It becomes project doctrine only after validation.

## The core Everward rule

**Build large worlds from readable structure, not from undirected detail.**

A strong environment should work at several scales:

- **macro:** silhouette, horizon, dominant astronomical body, major terrain forms, navigation landmarks;
- **meso:** crater fields, ridges, ice fractures, mining sites, industrial clusters, debris fields, regional material variation;
- **micro:** rocks, dust, scratches, pitting, decals, tool marks, cables, fasteners, surface roughness and local damage.

If the macro composition is weak, adding micro detail usually produces a more expensive weak scene.

---

# Adopted principles

## 1. Break repetition at several spatial frequencies

Never rely on one obvious texture scale or one noise scale.

Planetary and asteroid materials should combine:

- fine local surface detail;
- medium-scale variation;
- low-frequency world-space macro variation;
- regional/body-scale variation.

The goal is not maximum noise. The goal is to prevent the player's eye from discovering the repeating construction rule.

The canonical terrain anti-tiling standard is defined in `VISUAL_DIRECTION.md`.

## 2. Procedural generation should be rule-driven, not random

Random placement by itself produces visual static.

Procedural placement should use meaningful inputs such as:

- slope;
- altitude;
- curvature;
- material/geology;
- latitude or illumination;
- exposure;
- crater/ridge proximity;
- resource composition;
- hazard state;
- seeded regional identity;
- distance to infrastructure;
- gameplay clearance.

Unreal's PCG framework supports point attributes, density, seeds, spatial filters, hierarchical generation, and artist-authored graphs. Everward should exploit those semantics rather than using PCG as a scatter button.

A generated rock field should look as though physical processes put it there.

## 3. Use a hybrid of procedural systems and authored hero composition

Procedural tools are excellent for scale, variation, and iteration. They are not a substitute for composition.

Use procedural systems for:

- broad terrain populations;
- debris;
- boulder fields;
- crater secondary detail;
- ice fragments;
- dust/regolith distributions;
- industrial repetition;
- distant environmental density.

Use deliberate authored placement for:

- awakening location;
- first major vista;
- important mining target;
- first repaired structure;
- memorable landmarks;
- discovery sites;
- major narrative/environmental reveals.

This mirrors proven large-world practice: automate the bulk, preserve fine control where player attention is highest.

## 4. Build visual hierarchy before detail

Every scene should have an answer to:

**What does the eye read first, second, and third?**

Use:

- scale;
- silhouette;
- value contrast;
- lighting;
- color temperature;
- motion;
- atmospheric depth;
- negative space;
- framing.

Everward's environments can be visually dense without becoming visually flat.

A probe beside a moon should read first as:

1. astronomical scale;
2. probe / destination relationship;
3. local machinery and terrain detail.

Not as a uniform cloud of equally important objects.

## 5. Use landmarks and composition to orient the player

Navigation should not depend entirely on HUD arrows.

Useful spatial anchors include:

- planet or moon on the horizon;
- unique crater wall;
- unusual ridge;
- industrial tower;
- antenna;
- wreck;
- bright ice formation;
- station silhouette;
- star direction;
- strong shadow direction;
- moving orbital body.

Composition, lighting, FX, audio, and blockout geometry can all guide movement before explicit UI is required.

This is especially important in Everward because the player is a machine with rich sensors, but the physical universe should remain understandable without turning every destination into a floating marker.

## 6. Let the environment retain history

Worlds become believable when they imply that events occurred before the player looked at them.

Everward should show history through persistent physical evidence where practical:

- impact scars;
- excavation cuts;
- mining dust;
- displaced rubble;
- drill marks;
- abandoned equipment;
- repaired structures;
- replaced components;
- heat discoloration;
- micrometeorite damage;
- tracks / disturbed regolith where physically appropriate;
- debris from failure;
- salvage remnants;
- old landing/contact sites;
- construction growth over time.

Environmental storytelling is particularly valuable for Everward because much of its narrative is supposed to emerge from systems rather than scripted exposition.

## 7. Prefer system-generated environmental storytelling when possible

The strongest version is not a decorative prop pretending something happened.

It is:

**something actually happened, and the environment remembers it.**

Examples:

- mining physically changes the local site;
- discarded Gen-1 parts remain until recycled;
- a repair patch stays visible;
- a failed drone becomes salvage;
- industrial expansion changes traffic, lighting, debris, and surface disturbance;
- a radiation event changes exposed materials;
- repeated thruster operation darkens or disturbs a work zone;
- old infrastructure becomes visually obsolete beside new generations.

Simulation determines truth. Presentation preserves the evidence.

## 8. Use decals and overlays as controlled variation layers

Decals are valuable for:

- localized damage;
- dust;
- mineral staining;
- scorch marks;
- maintenance marks;
- impact scars;
- seams;
- residue;
- surface history.

They should break repetition and add local specificity without requiring a unique full texture set for every object.

Avoid uniform decal spam. A decal should usually explain something.

## 9. Make objects belong to the ground they occupy

Placed meshes should not look pasted onto terrain.

Potential integration methods include:

- Runtime Virtual Texturing where appropriate;
- localized dust/material blending;
- contact shadows;
- debris accumulation;
- slope-aware placement;
- partial burial;
- physically sensible support/contact geometry;
- matching local roughness/color contamination;
- footprints/disturbance where justified.

Epic's RVT workflow explicitly supports landscape/material compositing and blending of non-landscape actors into terrain. This is worth testing for Everward's surface environments.

## 10. Create reusable master material families

Do not author every rock, moon, probe panel, or industrial object as an isolated shader.

Build parameterized material families and derive instances.

For planetary surfaces, useful parameters may include:

- macro variation scale;
- macro variation strength;
- fine tiling scale;
- roughness range;
- mineral tint;
- ice fraction;
- dust coverage;
- exposed substrate;
- slope blend thresholds;
- normal intensity;
- decal/RVT response;
- damage/history layers.

Material instances allow variation without recompiling a new independent material for every asset.

Avoid uncontrolled static-switch combinations that create excessive shader permutations.

## 11. Author variation from physical cause

Everward's strongest art-direction advantage is that visual variation can come from science.

Instead of:

> make this region red because it looks cool

prefer:

> iron-rich oxidized material, unusual illumination, thermal history, irradiation, impact melt, ice contamination, or mineral composition produces this appearance.

Not every artistic choice needs a simulation equation, but the visual vocabulary should usually have a plausible cause.

## 12. Use modular kits, but hide the kit

Reusable modular assets increase production velocity dramatically, especially for industrial systems.

Everward should eventually use modular families for:

- structural trusses;
- storage;
- refinery equipment;
- fabrication modules;
- antenna systems;
- conduits;
- maintenance structures;
- docking assemblies;
- solar/energy structures;
- drone support equipment.

The danger is obvious repetition.

Counter it through:

- multiple module variants;
- functional arrangement differences;
- parameterized materials;
- decals;
- wear/history;
- cables/conduits;
- local terrain integration;
- different states of construction, repair, and age.

The player should understand that two structures share an engineering lineage without feeling that the level designer stamped the same prefab twenty times.

## 13. Greybox important spaces before polishing them

Before expensive art:

- establish scale;
- movement;
- collision;
- camera readability;
- manipulator reach;
- approach routes;
- sightlines;
- landmarks;
- interaction distance;
- lighting direction;
- composition.

A beautiful environment with poor spatial gameplay is expensive rework.

Everward's existing blockout-first philosophy should continue into planetary locations and industrial sites.

## 14. Design vistas intentionally

Everward's wallpaper-screenshot goal will not emerge automatically from high-resolution assets.

Create deliberate opportunities where:

- the player exits a confined or visually quiet space into a large reveal;
- the probe is silhouetted against a planet;
- mining machinery is framed against rings;
- a star rises behind a ridge;
- a new industrial complex becomes visible from distance;
- a repaired probe sees open space again;
- a successor launches past the original body.

A vista should ideally communicate gameplay state, scale, and emotion at the same time.

## 15. Protect negative space

Not every square meter needs detail.

Space, barren regolith, darkness, and long quiet sightlines are useful composition tools.

Dense detail becomes meaningful when contrasted against emptiness.

This is already consistent with Everward's visual direction: quiet space matters.

## 16. Performance architecture is part of world building

Do not build an environment that only becomes performant after its identity is destroyed.

Use appropriate Unreal systems from the beginning:

- Nanite for suitable high-detail/high-instance geometry;
- Virtual Texturing for large/complex texture workloads where justified;
- World Partition when world structure benefits from spatial streaming;
- HLOD for distant unloaded content;
- PCG hierarchical generation for different scales of detail;
- scalable material features;
- culling and presentation LOD where Nanite is not appropriate;
- explicit profiling rather than assumptions.

Nanite reduces many traditional geometry-management costs, but material complexity, instance counts, WPO, masking, output resolution, and streaming still require measurement.

## 17. Different scales need different representations

Do not attempt to render or simulate the same way at every distance.

Near the probe:
- full material detail;
- local debris;
- physical interaction;
- dense surface cues.

At medium distance:
- larger terrain forms;
- clustered detail;
- simplified small debris.

At orbital / system distance:
- silhouette;
- albedo/geological regions;
- atmospheric or ring structure;
- major landmarks.

This is the visual equivalent of Everward's simulation LOD philosophy.

## 18. Procedural generation must remain deterministic where it affects persistent world identity

A procedural environment is more useful to Everward when the same seed and inputs reproduce the same generated result.

That supports:

- debugging;
- regression testing;
- save consistency;
- player-shared coordinates/seeds;
- historical persistence;
- automated visual tests.

Artist overrides should also be serializable and traceable.

## 19. Test procedural output statistically and visually

One attractive screenshot does not validate a generator.

For important generation systems, test:

- many seeds;
- extreme parameter combinations;
- empty cases;
- dense cases;
- pathological slopes;
- poles/equator if relevant;
- very large/small bodies;
- transitions between regions;
- repeated structures;
- performance outliers.

Capture representative seed galleries so accidental visual regression becomes visible.

## 20. Every visual effect should answer: gameplay cue, physical consequence, atmosphere, or history?

Effects can serve more than one purpose.

Examples:

- mining sparks: physical process + feedback;
- radiator glow/temperature response: engineering state + visual identity;
- dust plume: contact event + motion cue;
- scan visualization: sensor feedback + probe embodiment;
- damaged material: history + system state.

Pure decoration is allowed, but it should be deliberate rather than the default.

---

# Unreal techniques worth prototyping

These are promising tools, not automatic mandates.

## Runtime Virtual Texturing

Potential Everward uses:

- terrain/mesh blending;
- localized surface disturbance;
- decal-like terrain layers;
- caching complex surface shading;
- integrating static industrial structures into planetary surfaces.

Prototype before committing because RVT has platform, mobility, bounds, and memory implications.

## PCG Framework

Potential uses:

- boulder fields;
- impact ejecta;
- ice fragments;
- procedural debris;
- geological dressing;
- industrial clutter;
- generated resource-site dressing;
- rule-based environmental populations.

Use hierarchical generation: large features on coarse grids, smaller detail on finer grids.

## Nanite

Strong candidates:

- rocks;
- cliffs;
- high-detail static machinery;
- structural modules;
- large numbers of suitable static meshes.

Do not treat Nanite as permission to ignore profiling.

## World Partition + HLOD

Potentially valuable once planetary/local environments become large enough to require spatial streaming while retaining distant silhouettes.

Do not introduce these systems before the current world scale actually benefits from them.

---

# Production learning loop

Whenever we encounter a strong professional tip, run this sequence:

```text
DISCOVER
   ↓
UNDERSTAND THE PRINCIPLE
   ↓
DOES IT FIT EVERWARD?
   ↓ no → archive as rejected/not-needed
  yes
   ↓
SMALL PROTOTYPE
   ↓
VISUAL + GAMEPLAY + PERFORMANCE TEST
   ↓
BETTER?
   ↓ no → reject or revise
  yes
   ↓
DOCUMENT STANDARD
   ↓
AUTOMATE / REUSE
   ↓
REGRESSION TEST
```

The project should maintain a difference between:

- **idea** — interesting advice;
- **candidate** — plausible Everward technique;
- **validated** — tested successfully;
- **canonical** — required project standard.

This prevents fashionable techniques from silently becoming architecture.

# Initial research sources

Primary engine references:

- Epic Games, **Building Virtual Worlds in Unreal Engine**  
  https://dev.epicgames.com/documentation/unreal-engine/building-virtual-worlds-in-unreal-engine
- Epic Games, **Procedural Content Generation Framework**  
  https://dev.epicgames.com/documentation/unreal-engine/procedural-content-generation-framework-in-unreal-engine
- Epic Games, **Runtime Virtual Texturing**  
  https://dev.epicgames.com/documentation/unreal-engine/runtime-virtual-texturing-in-unreal-engine
- Epic Games, **Landscape Materials**  
  https://dev.epicgames.com/documentation/unreal-engine/landscape-materials-in-unreal-engine
- Epic Games, **Instanced Materials**  
  https://dev.epicgames.com/documentation/unreal-engine/instanced-materials-in-unreal-engine
- Epic Games, **Decal Materials**  
  https://dev.epicgames.com/documentation/unreal-engine/decal-materials-in-unreal-engine
- Epic Games, **Nanite Virtualized Geometry**  
  https://dev.epicgames.com/documentation/unreal-engine/nanite-in-unreal-engine
- Epic Games, **World Partition HLOD**  
  https://dev.epicgames.com/documentation/unreal-engine/world-partition---hierarchical-level-of-detail-in-unreal-engine

Professional practice / postmortem references:

- GDC, **What Happened Here? Environmental Storytelling** — Harvey Smith & Matthias Worch
- GDC, **Fallout 4's Modular Level Design** — Joel Burgess & Nathan Purkeypile
- GDC, **Procedural World Generation of Far Cry 5** — Etienne Carrier
- GDC, **Continuous World Generation in No Man's Sky** — Innes McKendrick
- GDC, **GPU-Based Run-Time Procedural Placement in Horizon: Zero Dawn** — Jaap van Muijden
- GDC, **Level Design Workshop: Invisible Intuition** — David Shaver et al.
- GDC, **Level Design Fundamentals & Techniques** — Joel Burgess & Lee Perry

This list should grow as the project learns.

# Everward-specific quality question

For every environment, eventually ask:

> If the HUD vanished, could the player still understand where they are, what kind of place this is, what physical processes shaped it, what has happened here, what is important nearby, and why this scene belongs in Everward rather than in a generic science-fiction game?

If the answer is yes, the world building is doing real work.
