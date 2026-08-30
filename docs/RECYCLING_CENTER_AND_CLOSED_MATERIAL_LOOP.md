# Recycling Center and Closed Material Loop

## Canonical status

Everward uses a physically grounded material economy. Matter does not disappear when a machine, component, tool, or structure becomes obsolete. Salvage and recycling are first-class industrial systems, and a **physical recycling center/facility** is part of the intended infrastructure progression.

This doctrine extends `FIX_IT_UPGRADE_AND_DRONE_ECOLOGY.md` and the recycling requirements already present in `ROADMAP.md`.

## Core principle

> Nothing useful is casually wasted.

Every recoverable kilogram should remain materially accountable where practical. Failed hardware may become spare parts, repaired machinery, repurposed infrastructure, recyclable feedstock, or tracked low-grade waste/scrap.

The recycling system should support the long-term Everward fantasy of a machine civilization that increasingly closes its own material loops.

## Physical recycling center

The recycling center is not a menu button that deletes an object and refunds resources. It is a physical industrial facility with real inputs, processing limits, storage, energy requirements, tools, throughput, and outputs.

A mature recycling center may contain or coordinate:

- receiving and quarantine area for damaged or unknown hardware;
- inspection and identification systems;
- disassembly equipment;
- reusable-component sorting and storage;
- cutting, crushing, shredding, or size-reduction machinery where appropriate;
- material separation systems;
- furnaces, remelting, chemical processing, or other refining steps appropriate to the material;
- contamination handling;
- feedstock storage;
- low-grade scrap/waste storage;
- fabrication-compatible output streams;
- repair/reuse staging areas;
- autonomous maintenance and worker-drone interfaces.

The exact industrial process can become more sophisticated as technology improves, but recycling must remain constrained by physics, known materials, energy, tooling, and processing capability.

## Worker-drone interaction

Worker drones should make the recycling center part of the physical logistics network.

Relevant roles include:

- **gatherer/hauler drones** recovering scrap, failed parts, and defunct machines and transporting them to the facility;
- **scanner/inspection drones** identifying composition, contamination, damage, and reusable components;
- **repair/maintenance drones** determining whether hardware should be repaired rather than destroyed for feedstock;
- **fabrication/construction drones** disassembling large assemblies and using recovered material in new construction;
- **mining/industrial drones** assisting with heavy cutting, crushing, or material handling when useful;
- **defense/environmental drones** protecting remote salvage operations or hazardous recycling facilities where conditions require it.

A damaged worker drone may therefore eventually be recovered by another worker, transported home, inspected, repaired, cannibalized, or fully recycled.

## Decision hierarchy

Recycling should not automatically be the first answer. `Fix_It` should evaluate the highest-value use of failed or obsolete hardware:

```text
inspect
-> can it be repaired economically?
-> can it be reused as-is?
-> can it be repurposed?
-> are components worth salvaging intact?
-> recycle remaining material
-> track unrecoverable / low-grade residue
```

This keeps replacement, repair, reuse, salvage, and recycling connected rather than treating them as separate game systems.

## Material accounting

The intended loop is:

```text
resource deposit
-> mining / gathering
-> transport
-> refining
-> fabrication
-> machine / component / structure
-> use and wear
-> repair / reuse / replacement
-> salvage
-> recycling center
-> recovered components + recovered feedstock + tracked residue
-> fabrication again
```

The game should avoid magical 100% recycling unless a sufficiently advanced technology and material process justifies it. Different materials and assemblies may have different recovery efficiencies, contamination penalties, energy costs, and processing requirements.

## Progression

Early game recycling may be crude: manual disassembly with the probe's manipulator, separation of obviously reusable parts, and simple metal recovery.

Mid-game recycling becomes dedicated infrastructure with autonomous collection, sorting, disassembly, material identification, and refining.

Late-game machine civilization may operate regional reclamation networks capable of dismantling obsolete fleets, factories, structures, and abandoned installations while preserving historically or strategically important artifacts.

`Fix_It` should be able to recommend recycling infrastructure when waste accumulation, material scarcity, obsolete equipment, salvage distance, or replacement demand makes reclamation more valuable than continued virgin extraction.

## Design requirement

The player should eventually be able to look at an old, damaged, or obsolete machine and have several meaningful choices:

- keep it operating;
- repair it;
- upgrade it;
- repurpose it;
- salvage useful components;
- send it to the recycling center;
- preserve it because of historical significance.

That supports Everward's larger rule: **matter has history**. The industrial civilization grows not only by mining new material, but by continually reusing the physical legacy of what it previously built.