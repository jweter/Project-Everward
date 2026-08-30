# Mining, Materials, Replication, and Evolution Doctrine

This document records accepted player/product direction from the August 30, 2026 mining-control Product Reality discussion. It exists so these decisions do not depend on conversation history and so human, Codex, Claude, and scheduled development work can recover the intended game loop directly from the repository.

The master phase sequence remains in [`ROADMAP.md`](ROADMAP.md). This document is the canonical detailed doctrine for how scanning, manipulation, mining, storage, replication, recycling, repair, replacement, and evolution connect.

## 1. Product Reality comes before hidden capability

A capability that exists in code but cannot be seen, understood, or intentionally used by the player is not sufficient.

The current controls/HUD direction was accepted once the manipulator controls became visible and understandable. Preserve that presentation standard and improve it rather than regressing to hidden controls or tiny diagnostic text.

Required UX direction:

- primary controls and current tasks must be discoverable without reading source code or commit history;
- HUD text used for normal play must be human-readable at ordinary play distance and resolution;
- important controls must not be clipped, off-screen, or effectively invisible;
- the player must be able to tell what mode they are in, what the probe is doing, and what action is available next;
- diagnostic density is allowed, but it cannot make the game unusable as a player-facing interface;
- the HUD is the probe operating system, not a developer debug overlay.

A build that technically supports an action but gives the player no practical way to discover or operate it is still incomplete in Product Reality.

## 2. Manipulator arms need an actual economic purpose

If the probe can move articulated arms, those arms must lead to physical work.

The intended near-term progression is not "animate arm joints for their own sake." It is:

```text
see target
-> select target
-> scan target
-> identify useful material
-> approach / position probe
-> reach with manipulator
-> mine, cut, drill, grasp, or collect
-> physically acquire material
-> move material into storage or a receiving system
-> use that material for repair, fabrication, construction, or replication
```

The mining/manipulator arm is therefore a bridge from embodiment to industry.

Minimum consequences:

- the arm must eventually be able to collect material, not only touch scenery;
- tools and end effectors must have physical purposes;
- reach, alignment, joint limits, collision, force/tool capability, and storage access should matter;
- resource acquisition must change authoritative inventory/material state;
- no invisible resource teleportation should replace the physical interaction loop.

## 3. The scanner exists to make action possible

Scanning is not a detached minigame and is not merely a progress bar or generic science score.

Its practical purpose is to answer questions the machine needs in order to survive and expand:

- What is this object?
- What material is present?
- Is it useful?
- Can the current tool extract it?
- Is it worth the energy/time/risk to acquire?
- What can this material repair, replace, fabricate, or help construct?

The scanner therefore participates directly in the core expansion loop:

```text
scan
-> understand material
-> gather / mine
-> store / process
-> fabricate / repair
-> build improved systems and additional probes
```

Better scanners should reveal qualitatively better information and unlock better decisions, not merely increase a percentage bonus.

## 4. The strategic purpose of gathering is replication

Resource collection must point toward something larger than inventory accumulation.

The long-term purpose is to create a self-sustaining machine lineage and eventually a fleet/industrial network.

The first major proof is:

> The original probe can use what it learns and gathers to create another functioning machine.

That makes scanning, mining, storage, processing, fabrication, research, and repair parts of one coherent loop rather than disconnected systems.

The first successful replication should feel like a major threshold: the player has turned a stranded single machine into the beginning of an expanding lineage.

## 5. Specialization is expected: scanner, miner, gatherer, and beyond

Descendants and constructed units should be able to specialize physically rather than all remaining copies of one universal chassis.

Early understandable roles may include:

- scanner / survey probe;
- miner / extraction probe;
- gatherer / material-handling probe;
- hauler / transport probe;
- storage or depot infrastructure;
- processor / refinery;
- fabricator / constructor.

These are roles and evolutionary outcomes, not rigid character classes.

A lineage may combine roles in one body, split them across multiple machines, or evolve into forms not anticipated by the original Generation-1 probe.

Specialization must follow the existing adjacent-generation doctrine: a machine becomes a better scanner, miner, gatherer, or fabricator because its hardware, software, materials, computation, and manufacturing capability physically support that change.

## 6. Material logistics must become spatial and physical

A growing fleet needs somewhere for material to go.

Acceptable early patterns include:

```text
worker probe -> return material to parent probe
```

and later:

```text
worker probes -> shared material store / depot -> processing / fabrication / construction
```

The project should support construction of increasingly large storage and industrial infrastructure as throughput grows.

Material storage must remain physical and capacity-constrained enough to preserve engineering meaning. Large stores, depots, processors, and factories should exist in the world rather than becoming an abstract global inventory.

This is the beginning of the transition from one machine to a physically distributed industrial system.

## 7. Nothing useful should be wasted: recycling is a first-class industrial capability

Everward should strongly prefer a closed material cycle.

Recovered hardware, damaged parts, obsolete components, fabrication scrap, and defunct probes should be recyclable into useful source material when physically and chemically reasonable.

Design direction:

- salvage damaged or obsolete components;
- disassemble units that are no longer worth maintaining;
- recover usable subassemblies where appropriate;
- separate/reprocess source materials;
- feed recovered material back into repair, replacement, fabrication, and new construction;
- do not make discarded generations simply vanish from the world.

The target fantasy is not disposable robots. It is a machine civilization that understands matter well enough to reuse what it has already paid to extract and manufacture.

Perfect 100% recovery is not required where physics, contamination, entropy, or process losses make that implausible, but unnecessary gameplay waste should not be the default design.

## 8. Canonical progression: Repair -> Replacement -> Evolution

The machine-development arc should explicitly progress through three stages.

### Stage 1 — Repair

The original machine is damaged and initially survives by restoring existing hardware.

Repair means:

- diagnose what is wrong;
- obtain material and energy;
- restore an existing component toward useful operation;
- prioritize capability before perfection;
- gradually bring the whole machine back online.

This is the canonical damaged-awakening loop already defined in [`STARTER_AWAKENING_AND_SELF_REPAIR.md`](STARTER_AWAKENING_AND_SELF_REPAIR.md).

### Stage 2 — Replacement

Eventually, repair is no longer the best engineering answer.

The player should be able to fabricate and install a replacement component when the original is too damaged, inefficient, obsolete, or limiting.

Replacement introduces stronger consequences than repair:

- component manufacture;
- installation/removal logistics;
- compatibility and interfaces;
- calibration;
- mass/power/thermal changes;
- potential reuse or recycling of the removed component.

Replacement is the bridge between keeping the original probe alive and deliberately redesigning what it is.

### Stage 3 — Evolution

Once the player can design, fabricate, and replace systems, the same industrial base supports successor design and generational evolution.

Evolution means the lineage intentionally produces a machine that is not merely restored to its original specification.

```text
repair what exists
-> replace what no longer serves
-> improve or add capabilities
-> build successor
-> specialize descendants
-> expand fleet and industry
```

This progression should be visible in mechanics and not exist only as lore.

## 9. Generation-1 origin seed: the secret "Fix-It" program

Accepted origin direction: Generation 1 begins as a failed or underperforming probe project that was nevertheless launched. An intern secretly added a small "Fix-It" program before launch.

That program is the seed of the later self-repair/evolution behavior.

It should not begin as magical omnipotent self-improvement. Its early expression is incremental and practical: diagnose faults, use whatever power/material/manipulator capability remains, repair enough of the machine to regain another capability, reassess, and continue.

The narrative/mechanical significance is that a modest unauthorized recovery routine becomes the seed from which the machine eventually learns to:

```text
fix itself
-> replace parts
-> redesign parts
-> design successors
-> build descendants
-> create a machine lineage
```

The exact authored backstory can be refined later, but the causal relationship between the humble Fix-It seed and the enormous later evolutionary arc should be preserved.

## 10. Replication changes the scale of play

The project should intentionally evolve from direct operation of one probe toward orchestration of many machines.

Early game:

- the player manually scans, flies, positions, manipulates, mines, repairs, and manages power.

Middle progression:

- automation reduces repetitive sequences;
- specialized descendants perform mining, gathering, scanning, transport, processing, and fabrication;
- the parent or shared infrastructure becomes a coordination/material hub.

Later progression:

- fleets and distributed industry can operate from goals and policies rather than constant joint-by-joint micromanagement;
- local knowledge, communication latency, hardware, computation, and autonomy still constrain behavior;
- descendants may diverge physically and behaviorally.

The player should feel the scale increase because capability was built, not because the game silently replaced the physical simulation with an abstract strategy layer.

## 11. Development consequences

These decisions change what counts as a complete feature.

### Current Phase 2 / early Phase 3

- Keep the accepted large, readable manipulator/control presentation.
- Finish Product Reality legibility; normal play cannot rely on tiny debug text.
- Manipulator controls must lead into selecting, reaching, grasping, and physically moving targets.
- Scanner results must expose material/useful-action information.

### Phase 3 — One Star System

- Scanning must support material/resource decisions.
- Science must produce actionable knowledge about what can be acquired and why it matters.

### Phase 4 — Industrial Bootstrap

- Mining must physically acquire material.
- Storage must receive and track it.
- Processing/fabrication must transform it into useful stock/components.
- Repair must consume real resources.
- Replacement must follow repair as an explicit capability.
- Recycling/salvage must be part of the industrial model rather than postponed as decorative polish.

### Phase 5–6 — Engineering and First Replication

- The first successor must be built from the resource/industry loop the player established.
- Successor design may specialize toward scanner, miner, gatherer, industrial, computational, or hybrid roles.
- The first replication is the payoff for the entire scan -> mine -> process -> fabricate chain.

### Phase 10+ — Autonomous Children / Machine Society

- Fleet roles should emerge from real hardware and autonomy.
- Material return, depots, processing centers, construction nodes, and recycling infrastructure should allow distributed industry.
- Defunct machines remain part of material history and can be salvaged/recycled rather than disappearing.

## 12. Acceptance tests for future design work

When a new scanning, mining, storage, repair, fabrication, replication, or fleet feature is proposed, ask:

1. Can the player see and understand the control or state?
2. Does the scanner reveal information that changes a decision?
3. Does the manipulator physically do useful work?
4. Does mining actually transfer material into authoritative storage?
5. Can the acquired material be used for repair, replacement, fabrication, or construction?
6. Does the feature move the player closer to first replication or a stronger self-sustaining industry?
7. Can later probes specialize this capability physically?
8. Where does gathered material go when multiple machines exist?
9. Can obsolete/damaged material be salvaged or recycled instead of simply deleted?
10. Does the system fit the Repair -> Replacement -> Evolution progression?
11. Does automation reduce repetitive operation without bypassing the physical mechanics?
12. Does the result still feel like a machine civilization made out of real components and matter rather than abstract resource counters?

If the answer to these questions is no, the feature should be treated as incomplete or as a design conflict rather than quietly accepted.
