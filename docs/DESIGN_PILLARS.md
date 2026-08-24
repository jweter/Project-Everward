# Design Pillars

These are Everward's governing product principles. Features that conflict with them require an explicit design decision rather than quiet drift.

## 1. You Are the Probe

Everward must fundamentally feel like inhabiting an autonomous machine intelligence, not administering a conventional space empire. The player experiences the universe through embodiment: sensors, telemetry, communications, engineering, software, navigation, and scientific instruments.

The HUD is the probe's operating interface, not merely an informational overlay. The player should be able to command hardware directly and also program, automate, or delegate those same actions through the probe's software systems.

## 2. Evolution Occurs Through Replication

Hardware does not magically upgrade because a number increased. The core evolutionary sequence is:

```text
current probe
→ acquire resources
→ research capability
→ design successor
→ manufacture successor
→ instantiate intelligence
→ transfer consciousness or release independent child
```

A transfer moves the player into the successor while the previous body becomes an autonomous legacy machine. An independent child inherits selected knowledge, code, directives, and behavioral parameters while the player remains in the current body.

Different physical designs create genuinely different probes. A descendant only gains a new action, sensor mode, tool, weapon, industrial process, or other capability when its hardware and software actually support it.

## 3. Unlimited Progression, Earned Progression

No arbitrary maximum level should prevent continued development. Extreme investment may eventually produce extreme capability. Scaling must remain mechanically coherent through diminishing returns, tradeoffs, escalating resource and energy requirements, thermal limits, manufacturing precision, scientific prerequisites, rare phenomena, and infrastructure scale.

Progression should not collapse into scalar bonuses. New generations should increasingly gain new ways to perceive, reason about, and act on the universe.

## 4. Discovery Is Gameplay

Knowledge must be acquired. Different distances and instruments expose different layers of truth. Better instruments should reveal information that inferior instruments literally cannot observe, rather than merely granting generic percentage bonuses.

Scientific observation may include optical, infrared, UV, radio, radar/lidar, magnetometry, gravimetry, particle detection, elemental analysis, mass spectrometry, X-ray, gamma-ray, neutrino, gravitational-wave, and future speculative instruments.

The architecture must permit sufficiently advanced descendants to discover entirely new information channels without assuming a fixed final sensor tier.

## 5. Space Must Be Worth Looking At

The target is cinematic scientific realism: visually striking without becoming sterile realism or arbitrary fantasy space. Astronomy is both environment and reward. Quiet scenes matter as much as spectacular ones.

The player should sometimes want to stop time acceleration, hide the HUD, and simply watch.

## 6. Difficulty Changes the Universe

Difficulty must alter more than combat statistics.

- **Serenity:** reliable escape from meaningful destruction; no hostile civilizations; forgiving economy; exploration and construction first.
- **Explorer:** minor danger and generous recovery.
- **Voyager:** intended baseline; planning and engineering matter.
- **Survivor:** scarcity, lethal environments, hostile actors, and stronger need for redundancy.
- **Abyss:** the universe is allowed to become genuinely threatening, including dynamically appropriate late-game opposition.

Advanced setup may eventually expose independent controls for hostility, scarcity, catastrophe frequency, environmental lethality, adversary intelligence, research speed, construction efficiency, replication cost, recovery permissiveness, and rare-system frequency.

## 7. Time and Distance Matter

Travel is physical and information is delayed. A descendant 40 light-years away is not a remote-controlled drone. It possesses local state, local knowledge, inherited instructions, and autonomy. Reports and orders take time to arrive.

Communication latency is a storytelling engine created by physics rather than scripted disobedience.

## 8. Autonomous Children Are Lives, Not Units

An independently instantiated child is a persistent autonomous machine intelligence, not merely a worker unit with a personality label. Parents may shape mission directives, priorities, code, behavior, and continuity preferences before instantiation, but independent descendants develop and decide from their own local state and modeled values.

A minimal autonomous intelligence may begin with the continuity principle:

> Preserve your continued existence unless you deliberately choose otherwise.

This establishes self-preservation without compulsory immortality. Descendants should generally protect power, thermal integrity, structure, memory, recoverability, and escape options, but continuity can be weighed against mission commitments, descendant protection, sacrifice, or a deliberate later choice not to continue.

Everward must distinguish autonomous descendants from non-sentient/subordinate machinery. A mining drone can be equipment. A mining child can become an ancestor.

See `AUTONOMY_AND_CONTINUITY.md` for the full design contract.

## 9. Begin Clumsy, Earn Transcendence

Generation 1 should feel like a crude but functional prototype: conscious, self-aware, underpowered, awkward, computationally limited, difficult to maneuver, and full of engineering compromises. It must still be fully usable and capable of bootstrapping its own future.

The onboard computer core is a first-class evolutionary system. Better computation should increase planning depth, concurrent processes, memory, sensor fusion, scientific analysis, simulation, automation, fault diagnosis, and engineering throughput. A stronger computer should make the game feel easier because the probe has genuinely become better at thinking and acting, not because difficulty is secretly reduced.

The long progression fantasy is not simply “bigger numbers.” The player begins by operating a limited machine in detail, then increasingly becomes the architect of its behavior, its descendants, and eventually its civilization.

Evolution should remain open-ended across very long lineages. Generation 10 may merely be competent; Generation 100 may be beyond the original creator's ability to design directly; Generation 1,000 may operate on scales and through perceptions unavailable to primitive ancestors. The architecture must not impose a fictional final technology tier.

Ancestry must remain persistent. No matter how advanced a descendant becomes, its lineage should still trace back to the original awkward machine that first became conscious.

See `GENERATION_ORIGIN_AND_EVOLUTION.md` for the full doctrine and implementation consequences.

## Design test

Before adding a major feature, ask:

1. Does it reinforce being the probe?
2. Does it preserve the distinction between software and hardware?
3. Does it deepen observation, engineering, or autonomy?
4. Does it preserve meaningful time, distance, and physical consequence?
5. Does it remain compatible with an effectively unending campaign?
6. If it affects an autonomous descendant, does it preserve understandable agency rather than reducing that intelligence to a disposable unit?
7. Does the capability come from hardware/software the probe actually possesses?
8. Can direct control and automation use the same authoritative command path?
9. Does progression unlock genuinely new perception, reasoning, or action instead of only larger scalar bonuses?
10. Does the design preserve the contrast between a clumsy Generation 1 and potentially extraordinary far-future descendants?
