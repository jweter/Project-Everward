# Everward Future Platform Vision

## Status

This document records a **long-term product and architecture direction**, not a commitment for the first playable build, vertical slice, Early Access, or 1.0.

Everward remains **PC-first**, with Unreal Engine as the production presentation target. The purpose of this document is to preserve a future possibility that fits the game's existing architecture and its evolution from direct machine embodiment into civilization-scale command.

## Core platform idea

Everward should not become two unrelated games simply because the player moves between a PC and a phone.

The preferred future model is:

> **One Everward campaign. One authoritative simulation and persistent history. Multiple interfaces appropriate to different devices.**

A future mobile experience should therefore be conceived primarily as a **shared-state command client** for the same Everward universe rather than as a simplified, disconnected mobile spin-off.

## PC remains the complete embodied experience

The PC game is the canonical full-fidelity Everward experience.

It is where the player directly inhabits the probe and receives the full physical and cinematic presentation:

- direct probe movement and attitude control,
- manipulator and mining-tool operation,
- local physical interaction,
- scanning and scientific observation,
- detailed astronomical environments,
- high-fidelity machinery and industry,
- Unreal rendering, lighting, particles, and materials,
- extreme astronomy,
- photo mode,
- detailed fabrication, repair, replacement, and construction feedback,
- full local HUD and spatial sensorium.

Everward should **not** reduce the PC visual or simulation target merely so the exact same presentation can run natively on a phone.

## Mobile should become a command interface, not a compromised PC port

As Everward grows, the player's control naturally moves through four scales:

1. **Direct control** — operate the current probe and its tools.
2. **Task control** — assign concrete work to individual machines.
3. **Operational control** — manage groups, routes, stockpiles, maintenance, production, and local industrial networks.
4. **Doctrine control** — define intent and policy for large machine populations and distant descendants.

The later three layers are especially well suited to phones and tablets.

A future mobile client could allow the player to:

- inspect probe telemetry, energy, heat, storage, and damage,
- review `Fix_It` diagnoses and ranked recommendations,
- approve or reprioritize repairs and replacements,
- queue fabrication and construction,
- inspect storage, refinery, fabrication, and recycling status,
- assign mining, hauling, repair, construction, and scanner tasks,
- set stockpile and production targets,
- manage worker groups and operating areas,
- edit doctrines and automation policies,
- inspect descendants and machine genealogy,
- review research and discoveries,
- read delayed communications from distant probes,
- issue orders that respect communication latency,
- inspect system and galaxy maps,
- review alerts, bottlenecks, history, and significant events.

The mobile interface should emphasize **intent, information, and command** rather than attempting to reproduce every local 3D interaction.

## The same machine should remain the same machine

A machine must not become a different abstract object merely because the player views it from mobile.

For example, on PC a nearby mining drone may be represented with:

- articulated joints,
- physical contact,
- tool motion,
- cargo transfer,
- detailed damage,
- lighting and particles.

On mobile the same persistent entity might be represented as:

```text
MINER-014
Task: Mining Ni/Fe asteroid A-77
Cargo: 71%
Condition: 93%
Power reserve: 64%
ETA to unload: 18 min
```

The presentation changes. The authoritative identity, history, material state, task, and consequences do not.

This principle aligns directly with Everward's existing requirement that simulation truth remain separate from rendering and UI presentation.

## Shared campaign and continuity

If this platform direction is eventually implemented, the desired experience is continuity across devices.

Example:

```text
PC
-> personally scan and mine an asteroid
-> establish an industrial task
-> save/synchronize campaign state

PHONE
-> inspect production while away from the PC
-> review Fix_It recommendation
-> approve two additional haulers
-> change repair priority
-> read a delayed message from a child probe

PC
-> return to the same universe
-> continue direct embodied play
```

This should feel like **returning to the same existence**, not starting a second game.

Any eventual synchronization, hosting, authentication, offline behavior, conflict resolution, and security design must preserve simulation authority and deterministic history.

## Mobile-native visual scope

A native mobile client may still contain visually rich 3D and animated interfaces where useful, such as:

- rotating probe and component inspection,
- system maps,
- orbital diagrams,
- trajectory visualization,
- descendant and lineage visualization,
- industrial network views,
- simplified local scenes,
- station/factory previews,
- astronomical discovery presentations.

However, native mobile rendering is not required to reproduce the complete PC Unreal presentation pipeline.

## Streaming is a separate future option

A second future path could allow the full PC game to be used on a phone or tablet through remote/streamed play.

That creates three distinct experiences without creating three different simulations:

1. **PC native** — complete Everward.
2. **Mobile native command client** — strategic/operational/doctrine interaction with the same campaign.
3. **Streamed PC experience** — full game presentation on a mobile device when the player wants direct control remotely.

Streaming should remain independent from the native mobile-client design.

## Why Everward is unusually compatible with this model

Many games become harder to translate to mobile as they become more complex.

Everward may become **more naturally compatible** with mobile interaction as a campaign matures.

Early game questions are physical:

> Move my arm.
>
> Scan this object.
>
> Mine this deposit.

Late-game questions increasingly become systemic:

> Why has System 42 nickel throughput fallen?
>
> Should Generation-17 scanners replace Generation-12 units?
>
> What does Fix_It recommend rebuilding next?
>
> Which descendant lineage has stopped reporting?
>
> Should Exploration Doctrine 4 permit autonomous replication?

Those are excellent tablet/phone interactions because Everward's civilization-scale gameplay is fundamentally about **telemetry, hierarchy, delayed information, policy, prioritization, and intent**.

## Architectural implications we should preserve now

This future capability is **not a reason to build mobile networking now**.

It is a reason to avoid unnecessary architectural dead ends.

The project should continue preserving:

- engine-independent authoritative simulation truth,
- presentation/UI separation,
- stable persistent entity identifiers,
- versioned save/state schemas,
- explicit commands rather than UI-owned mechanical mutation,
- deterministic event history where practical,
- hierarchical control structures,
- machine genealogy and persistent identity,
- structured telemetry,
- explicit communication messages and latency,
- simulation level of detail for remote populations,
- APIs/boundaries that could later expose safe subsets of campaign state and commands.

No mobile-specific production work is required during the current phases unless a future accepted architecture decision explicitly schedules it.

## Product rule

The future platform objective is:

> **Expand where the player can interact with Everward without shrinking what Everward is.**

PC remains the place for maximum embodiment and spectacle.

Mobile may eventually become the place for persistent connection to the civilization the player created.

## Brand language — provisional, not legally cleared

Two lines currently express complementary parts of the project identity:

> **Everward — there is always farther.**

and

> **Always expanding and evolving.**

The first expresses exploration and the endless horizon.

The second expresses the machine, the lineage, the industry, the software, the hardware, the civilization, and the project itself: Everward does not merely travel outward; it continually changes what it is capable of becoming.

A possible stylized marketing form is:

> **Always expanding. Always evolving.**

This wording is recorded as **provisional brand/tagline language only**. It is not a representation that the phrase is available, registrable, or cleared for trademark use. Formal name/tagline clearance should occur before commercial branding is locked.

## Scope boundary

This document does **not** add mobile development to the current execution plan.

Priority remains:

```text
One Probe
-> reliable scanning and mining
-> material accounting
-> Fix_It repair
-> replacement / recycling
-> storage and worker machines
-> industrial bootstrap
-> successor engineering
-> first replication
-> interstellar expansion
```

The mobile/shared-platform vision is valuable precisely because it can grow naturally from the simulation and control architecture Everward already needs for the PC game.
