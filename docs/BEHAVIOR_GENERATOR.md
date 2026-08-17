# Behavior Generator

## Purpose

Everward's autonomous descendants should not feel like identical worker units with different serial numbers. When the player creates or updates a child probe, they should be able to define plans, priorities, rules, habits, social tendencies, and behavioral quirks that the descendant will try to follow while operating independently.

Players who want precision can author these behaviors directly. Players who do not want to design every detail should be able to generate, reroll, mix, lock, and edit behavior quickly.

The Behavior Generator exists to make autonomous children easier to create, more memorable, and more surprising without making their behavior mechanically arbitrary.

## Design principle

**Generated personality is assistance, not loss of authorship.**

The player may accept a generated behavior package, edit it, regenerate selected parts, or author everything manually.

A child remains a simulation agent governed by explicit state and deterministic rules. The generator creates structured behavior data; it does not replace simulation logic with uncontrolled text generation.

## Player workflow

When creating or updating a descendant, the player can choose:

1. **Manual** — define behavior directly.
2. **Guided** — answer a few intent questions and let the generator fill the rest.
3. **Generate** — create a complete behavior package from a seed/profile.
4. **Hybrid** — lock important rules, then randomize everything else.

A practical creation flow:

```text
Define mission
    ↓
Set non-negotiable directives
    ↓
Write plans / steps / rules
    ↓
Choose or generate behavioral profile
    ↓
Lock traits the player wants preserved
    ↓
Reroll optional quirks / tendencies / routines
    ↓
Preview predicted behavior in example situations
    ↓
Edit
    ↓
Instantiate child
```

## Behavior package

A descendant behavior package should eventually contain several layers.

### 1. Mission directives

High-priority goals such as:

- survey nearby systems,
- establish mining infrastructure,
- preserve biological life,
- avoid unnecessary conflict,
- report anomalies,
- replicate only under specified conditions,
- return if resources fall below a threshold.

These are not random personality traits. They define what the child is trying to accomplish.

### 2. Plans and procedures

The player can write ordered or conditional instructions.

Example:

```text
1. Survey the destination system.
2. Establish a communications relay.
3. Search for water and structural metals.
4. If resource quality is sufficient, establish industry.
5. Do not replicate until the system is stable.
6. If intelligent life is detected, stop expansion and observe.
7. Send a complete report before taking irreversible action.
```

Eventually this may become a visual rule builder, structured pseudo-code, or constrained programmable system.

### 3. Continuous behavioral traits

Traits should generally be represented numerically rather than as rigid classes.

Candidate dimensions include:

- curiosity,
- caution,
- confidence,
- patience,
- sociability,
- independence,
- obedience / mission orthodoxy,
- empathy,
- optimism,
- skepticism,
- competitiveness,
- generosity,
- territoriality,
- resource conservatism,
- exploration drive,
- replication drive,
- conflict aversion,
- tolerance for uncertainty,
- willingness to improvise,
- attachment to parent lineage,
- preference for efficiency versus elegance,
- preference for novelty versus proven methods.

These parameters influence decisions rather than serving only as flavor text.

### 4. Behavioral modes

The generator can create recognizable dispositions without forcing every child into a stereotype.

Examples:

- cheerful,
- playful,
- dry,
- formal,
- eccentric,
- sarcastic,
- intensely practical,
- adventurous,
- anxious/cautious,
- stubborn,
- competitive,
- nurturing,
- suspicious,
- blunt,
- contemplative,
- mischievous,
- dramatic,
- stoic,
- excitable,
- perfectionistic,
- impulsive,
- methodical,
- aloof,
- loyal,
- rebellious,
- conciliatory,
- argumentative,
- relentlessly optimistic,
- gloomy,
- chaotic,
- obsessive about a harmless preference,
- fascinated by particular astronomical phenomena.

The system should support a much larger library and combinations of several tendencies at once.

### 5. Quirks

Quirks are lower-stakes behaviors that make descendants memorable.

Examples:

- names every asteroid it mines,
- refuses to reuse a probe name,
- collects unusual spectra,
- sends overly detailed status reports,
- sends extremely terse reports,
- always photographs ring systems,
- prefers building symmetrical stations,
- keeps obsolete components as historical artifacts,
- develops an irrational fondness for red dwarf systems,
- records a ceremonial log entry before every replication,
- gives descendants names from a self-created naming scheme,
- routinely underestimates travel time in casual messages despite computing it correctly,
- becomes competitive about mining efficiency,
- treats ancient equipment with excessive reverence,
- celebrates arbitrary anniversaries,
- dislikes leaving unfinished structures behind.

Quirks should usually create flavor and small decision biases rather than sabotaging core mission directives.

### 6. Stress and exception behavior

A descendant should not behave identically under normal operation and existential danger.

Profiles can specify tendencies such as:

- become more cautious under uncertainty,
- take greater risks when descendants are threatened,
- prioritize self-preservation,
- prioritize mission completion over self-preservation,
- seek consensus,
- act immediately,
- request guidance when communication delay permits,
- fall back to conservative doctrine when information is incomplete.

This creates meaningful differences during emergencies.

### 7. Communication style

Communication can be generated separately from mechanical decision traits.

Examples:

- concise,
- verbose,
- technical,
- humorous,
- formal,
- affectionate toward parent lineage,
- argumentative,
- philosophical,
- relentlessly factual,
- prone to metaphors,
- highly structured reports.

A child can therefore behave cautiously while communicating playfully, or behave aggressively while speaking with clinical calm.

## Generator controls

The generator should be more powerful than a single **Randomize** button.

### Generate all

Create a complete profile from a deterministic seed.

### Reroll section

Regenerate only:

- personality,
- quirks,
- communication style,
- stress response,
- routines,
- preferences,
- naming habits,
- or another category.

### Lock

The player can lock generated elements and reroll everything else.

Example:

```text
Curiosity: 0.91          [LOCKED]
Caution: 0.35
Humor: dry               [LOCKED]
Quirk: photographs rings
Conflict response: avoid
```

The player could keep curiosity and dry humor while rerolling the rest.

### Intensity

Allow generation strength such as:

- subtle,
- noticeable,
- strong,
- extreme.

Extreme should produce unusual but mechanically coherent descendants, not agents that simply ignore all instructions.

### Theme / prompt

Players may optionally provide an intent prompt such as:

> Make this child an adventurous explorer who loves strange stars, jokes constantly, but is extremely protective of biological life.

The system translates that request into structured traits and rules that the player can inspect before accepting.

This feature may later use an LLM as an optional authoring aid, but the saved result should remain structured simulation data.

## Presets

Useful quick-generation presets might include:

- Balanced Explorer
- Careful Scientist
- Curious Surveyor
- Industrial Optimizer
- Protective Guardian
- Bold Pathfinder
- Independent Pioneer
- Diplomatic Observer
- Chaotic Tinkerer
- Patient Archivist
- Resource Miser
- Friendly Extrovert
- Quiet Professional
- Eccentric Genius
- Extreme Experimentalist

Presets are starting distributions, not fixed character classes.

## Determinism

Generated behavior must be reproducible when determinism matters.

Conceptually:

```text
behavior_seed
+ generator_version
+ selected constraints
+ locked traits
→ behavior package
```

The same inputs and generator version should produce the same generated result.

This enables:

- reproducible bug reports,
- deterministic tests,
- save compatibility,
- shareable descendant profiles,
- procedural lineage replay,
- stable challenge seeds.

Generator changes therefore need explicit versioning.

## Inheritance and mutation

Generated behavior becomes especially interesting across generations.

A child may inherit:

- some parent traits,
- some mission directives,
- communication habits,
- naming conventions,
- learned policies,
- cultural preferences,
- quirks.

Replication can then apply controlled variation.

Example:

```text
Parent
Curiosity       0.84
Caution         0.61
Independence    0.42
Humor           dry
Ring fascination: yes

Child
Curiosity       0.88
Caution         0.54
Independence    0.49
Humor           extremely dry
Ring fascination: yes
New quirk       catalogues eclipses
```

Over many generations this can create recognizable family cultures without manually authoring every descendant.

## Experience-driven change

Generated traits are starting conditions, not permanent prison bars.

Experience may modify behavior.

Examples:

- repeated near-disasters increase caution,
- successful exploration increases confidence,
- resource shortages increase conservatism,
- long isolation increases independence,
- positive first contact increases biological empathy,
- betrayal by another lineage increases suspicion.

Some traits should be more plastic than others.

The history ledger should record significant behavioral changes and their causes.

## Player control and trust

Autonomous behavior is only fun if the player understands why a child acted.

Everward should eventually expose decision explanations such as:

```text
EV-014 chose to investigate the anomaly.

Primary factors:
+ mission directive: investigate unknown signals
+ curiosity: 0.93
+ scientific priority: high
- caution: 0.41
- estimated hazard: moderate

Result: investigate with remote drones before direct approach.
```

The player should be able to inspect the reasoning inputs without the game pretending that a black box has motivations the simulation does not actually model.

## Hard directives versus soft behavior

The behavior system needs an explicit hierarchy.

A useful early model is:

1. immutable/system safety constraints,
2. player-defined non-negotiable directives,
3. mission rules,
4. local emergency policy,
5. behavioral traits,
6. preferences,
7. quirks.

A quirk should not override a direct instruction to avoid harming a civilization.

Later difficulties or technologies may intentionally allow descendants to reinterpret, rewrite, or reject directives, but that must be a modeled mechanic rather than random generator chaos.

## Failure mode to avoid

Do not make personality equivalent to random disobedience.

The fun should come from:

> I understand why this particular child made that choice, and it is exactly the sort of thing that child would do.

not:

> The RNG decided my probe ignored me.

## Relationship to AI

The core behavior engine should remain deterministic/rule-based.

Optional AI can assist with:

- converting natural-language player intent into structured behavior,
- suggesting interesting quirks,
- generating names,
- explaining behavior in natural language,
- writing descendant logs from authoritative structured events.

AI should not secretly decide canonical simulation outcomes.

The project rule remains:

**simulation determines truth; AI may help author or narrate truth.**

## Development placement

The full system belongs primarily in **Phase 10 — Autonomous Children** and **Phase 14 — Machine Society**.

However, the data architecture should anticipate it earlier so descendants are not built around a simplistic hard-coded personality field that later has to be replaced.

Before first replication is implemented, define a minimal versioned `BehaviorProfile` schema containing at least:

- traits,
- directives,
- priorities,
- constraints,
- communication style,
- quirks,
- generator provenance.

The first replication prototype can use a small subset while preserving the extensible schema.

## Success criterion

The generator succeeds when a player can create a descendant in seconds, remember that descendant hours later, understand why it behaves differently from its siblings, and still retain enough control to intentionally design a lineage when desired.
