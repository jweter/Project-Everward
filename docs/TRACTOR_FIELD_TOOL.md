# Tractor Field / Inertial-Coupling Tool

This document records an accepted Everward mining/general-purpose tool: a non-contact field projector that behaves like a tractor beam while preserving believable zero-gravity momentum behavior.

The field technology itself is speculative. Once the field couples two bodies, however, the motion should obey ordinary mass, momentum, force, thrust, and reaction principles rather than arcade pickup rules.

This capability complements the existing manipulator/grapple/mining systems. It does not replace physical arms, cutting tools, drilling, storage, or hauling.

## 1. Core player fantasy

The player can project a tractor field onto nearby debris, mined fragments, disabled machines, cargo, components, or other physically eligible objects and pull them toward the probe without direct contact.

The quirk is essential:

- If the target is much less massive than the probe, the target comes toward the probe quickly while the probe moves only a little.
- If the target is much more massive than the probe, the probe is pulled toward the target much more strongly than the target moves.
- If the masses are similar, both bodies move toward one another substantially.
- If the probe uses its engines while the field remains coupled, engine thrust can add net momentum to the coupled system and enable real towing behavior.

A large asteroid is therefore not a giant inventory item. Tractor it carelessly and the probe effectively uses the asteroid as an anchor.

That behavior is intentional and should become a useful traversal and engineering mechanic, not only a penalty.

## 2. Physics rule

The game should preserve this distinction:

```text
momentum p = m * v
force F = dp/dt
impulse J = F * dt = delta-p
```

`mass * velocity` is momentum, not force.

The player's intended "accelerate first, then engage the tractor" idea still works in a physically meaningful way:

1. Engine thrust gives the probe momentum.
2. The tractor field couples the probe and target.
3. Equal-and-opposite field forces transfer some of the probe's existing momentum into the target while changing the probe's own velocity.
4. If the engines continue thrusting while the field is coupled, they supply new external momentum to the probe/target system.
5. A sufficiently capable probe can therefore tow, redirect, brake, or accelerate masses far above what the tractor field alone would appear able to "pick up."

The field itself must never create hidden net momentum for the pair.

For a unit direction `r_hat` pointing from probe to target and tractor magnitude `F_b`:

```text
force on probe  = +F_b * r_hat
force on target = -F_b * r_hat
```

The acceleration magnitudes are:

```text
a_probe  = F_b / m_probe
a_target = F_b / m_target
```

So the lower-mass body changes velocity more.

With engine thrust `T_engine` acting on the probe:

```text
m_probe * dv_probe/dt + m_target * dv_target/dt = T_engine
```

The tractor interaction cancels internally; the engines are what change total momentum.

## 3. Gameplay uses

The tractor field should become a genuine general tool rather than a single-purpose mining gimmick.

Near-term uses:

- pull loose mined fragments toward the probe;
- gather nearby debris without precision arm contact;
- bring a sample into manipulator reach;
- retrieve dropped components;
- move salvage toward a recycling or storage receiver;
- tow a disabled worker drone;
- pull cargo pods or fabrication stock;
- hold a drifting object near a work area;
- use a massive object as an anchor to pull the probe toward it;
- arrest or modify relative drift before grasping;
- reposition objects for cutting, drilling, repair, or construction.

Later uses:

- orbital construction handling;
- coordinated tug operations;
- towing damaged probes or stations;
- moving asteroid fragments into industrial processing lanes;
- cooperative multi-probe pulls on very large bodies;
- station-keeping around awkward cargo;
- momentum exchange and braking maneuvers;
- salvage after collisions or battles;
- debris-clearing and hazard mitigation.

## 4. Tool relationship to manipulators

Generation-1 should initially treat the tractor field as a tool/projector associated with the probe's physical work systems.

A sensible early presentation is a field-projector tool head or emitter that can be aimed using the same target-selection vocabulary as mining and manipulation. Later generations may evolve stronger ship-mounted, distributed, or multi-emitter tractor systems.

The manipulator remains necessary because the tractor field does not inherently provide:

- precise mechanical alignment;
- cutting;
- drilling;
- fastening;
- connector mating;
- component installation;
- fine repair work;
- guaranteed stable grasp;
- material processing.

A common loop should be:

```text
scan target
-> select target
-> tractor target into useful relative position
-> stabilize relative motion
-> reach/grasp with manipulator
-> mine / cut / repair / store / recycle
```

## 5. Range and field strength

A tractor system has real hardware limits.

Important parameters include:

- maximum effective surface-to-surface range;
- maximum field force;
- power draw;
- thermal load;
- field stability;
- target coupling quality;
- target composition/geometry effects where appropriate;
- aiming/tracking capability;
- number of simultaneous couplings;
- control bandwidth.

Generation-1 should begin with one modest field coupling at a time.

Requested field force above the installed hardware rating should clamp or fail safely rather than secretly produce more force.

Large future systems may increase field strength enormously, but progression should still require power generation, thermal rejection, structure, computation, and manufacturing capability.

## 6. Distance behavior

The first implementation uses a hard maximum surface-to-surface operating range so the mechanic is clear and deterministic.

Later prototypes may test field falloff, coupling efficiency, or power cost versus distance if those make play more interesting without obscuring the mechanic.

Range should be measured from physical surfaces rather than body centers. A large asteroid should not become impossible to couple merely because its center is far behind the visible surface.

## 7. Momentum-assisted towing

This is a signature skill expression for the tool.

### Static pull

Probe and target begin near rest.

The beam couples them and both accelerate toward one another. Which object visibly moves more depends on mass ratio.

### Pre-accelerated coupling

The probe accelerates before engaging the field.

When coupling begins, the target can acquire some of the probe's existing momentum while the probe slows or changes trajectory.

This can be useful when the probe does not have enough continuous thrust to produce the desired target acceleration quickly but can build velocity over time first.

### Powered tow

The probe keeps its engines firing while the tractor remains engaged.

The engines provide external force. The field transmits the resulting momentum exchange to the target.

This allows a sufficiently powerful probe to tow an object more massive than itself, but the result depends on:

- engine thrust;
- probe mass;
- target mass;
- beam force rating;
- tow direction;
- available propellant/energy;
- thermal capacity;
- time.

### Anchor maneuver

The player deliberately tractors an object much more massive than the probe.

The target barely moves while the probe is accelerated toward it.

This can become a useful emergency movement, approach, station-keeping, or low-propellant maneuver if the player understands the geometry and collision risk.

## 8. Failure and risk

The tool should have consequences.

Possible later failure modes include:

- field overload;
- thermal shutdown;
- insufficient power;
- target leaves range;
- tracking loss;
- coupling instability;
- excessive relative velocity;
- collision caused by pulling too aggressively;
- tractor force exceeding safe structural load on a fragile target;
- unstable multi-body interaction;
- towing a large mass without enough engine authority to control the resulting trajectory.

The first implementation does not need every failure mode, but the architecture must not assume the tractor field is consequence-free.

## 9. HUD and control requirements

Product Reality applies here exactly as it does for mining.

The player should be able to understand at a glance:

- selected tractor target;
- target estimated mass;
- probe mass;
- mass ratio;
- surface distance;
- field range;
- commanded field strength;
- maximum field strength;
- whether the target or probe is expected to move more;
- relative velocity;
- whether engines are adding or opposing towing momentum;
- beam power/thermal state once modeled;
- coupling state: READY / COUPLED / OUT OF RANGE / OVERLOAD / TRACK LOST.

A useful predictive HUD line might say:

```text
TARGET MASS 10,000 kg // PROBE MASS 2,500 kg // 4.0x HEAVIER
FIELD 1.0 kN // EXPECT PROBE TO ACCELERATE ~4x MORE THAN TARGET
```

or:

```text
POWERED TOW
ENGINE IMPULSE ADDS NET MOMENTUM TO COUPLED SYSTEM
```

This should teach the physics through play rather than requiring a manual.

## 10. AI / automation implications

Worker drones should eventually use tractor tools autonomously for material handling.

Examples:

- gather fragments after a miner breaks material loose;
- pull salvage into a recycler intake envelope;
- tow disabled machines to maintenance;
- keep cargo from drifting away;
- coordinate multiple tractors on a large object;
- use mass-ratio and engine-authority estimates before committing to a tow.

Fix_It should understand the same physics when recommending repairs or industrial changes. For example:

> Hauler-12 cannot safely tow the disabled refinery module with current field strength and remaining propulsion authority. Recommended: dispatch two additional tugs or install a stronger tractor projector.

The automation must never cheat by teleporting cargo that player-controlled machines are expected to move physically.

## 11. Evolution path

The tractor field fits Everward's Repair -> Replacement -> Evolution architecture.

Possible progression:

```text
Gen-1 single low-range field projector
-> improved tracking and force
-> lower power cost
-> better thermal efficiency
-> longer range
-> stronger emitter
-> multiple simultaneous couplings
-> distributed hull emitters
-> dedicated tug drone
-> coordinated tractor arrays
-> industrial mass-handling systems
-> extreme late-generation orbital/asteroid engineering
```

A future mining lineage may favor short-range high-force projectors.

A salvage lineage may favor many low-force couplings.

A construction lineage may favor precise vector control.

A tug lineage may pair very strong tractor systems with unusually powerful engines and thermal management.

## 12. Simulation contract

The authoritative simulation owns tractor consequences.

Presentation may draw beams, reticles, force vectors, predicted paths, and effects, but Unreal must not independently decide which body moved or by how much.

The initial engine-independent foundation is implemented in:

- `src/simulation/include/everward/simulation/tractor_field.hpp`
- `src/simulation/tests/tractor_field_tests.cpp`

The first automated tests lock in these rules:

1. lighter target moves more than the heavier probe;
2. heavier target pulls the lighter probe more strongly;
3. beam-only coupling conserves total momentum;
4. pre-existing probe momentum can transfer to the target without creating momentum;
5. engine thrust changes total coupled momentum by exactly the external engine impulse;
6. installed beam force is hardware-limited;
7. out-of-range targets fail without motion;
8. invalid physical states fail closed.

## 13. Development order

Do not let this new tool derail the immediate Phase-2 Product Reality gate.

Recommended order:

1. preserve reliable scan/mining/manipulator gameplay;
2. land the authoritative tractor-field physics foundation and tests;
3. expose one tractor target in the Phase-2 test environment;
4. add a visible control and coupling-state HUD;
5. let the player pull one loose debris/sample body;
6. verify both light-target and heavy-target behavior in Unreal;
7. connect tractor handling to storage/salvage/mining flow;
8. add engine-assisted towing telemetry;
9. later extend to drones and industrial logistics.

The mechanic is accepted as part of Everward's long-term mining, salvage, logistics, construction, and general probe-tool vocabulary.
