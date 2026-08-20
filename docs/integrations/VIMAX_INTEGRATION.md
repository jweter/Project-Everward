# ViMax Integration Plan

Last reviewed: 2026-08-20
Upstream: `HKUDS/ViMax`
Integration posture: **External media-production tool and architectural reference; no direct game-runtime dependency**

## Purpose

ViMax is useful to Everward in two distinct ways:

1. as an optional external pipeline for trailers, devlogs, concept reels, and promotional storyboards; and
2. as a reference for persistent agent-loop concepts such as planning, execution, artifact checkpoints, resume behavior, and human revision.

These two uses must remain separated. Media tooling must not silently become game-AI architecture, and game-agent design must not import ViMax application code merely because its workflow is conceptually useful.

## License boundary

ViMax is currently MIT licensed. Even with a permissive code license, every external image/video/audio/model provider used by a ViMax workflow has independent terms. Generated marketing assets must satisfy the same Everward provenance rules as other AI-assisted assets.

## Media-production architecture

```text
Everward build captures / approved art
  -> marketing brief
  -> ViMax storyboard/agent workflow
  -> generated or assembled shot candidates
  -> human review
  -> optional Remotion/final compositor
  -> provenance + rights review
  -> published media
```

The game repository should store reusable prompts/templates/configuration only when safe. Large generated media should follow the project's artifact/media storage policy rather than inflate source history.

## Agent-architecture research boundary

ViMax may be studied for patterns such as:

- goal decomposition;
- persistent task/session state;
- artifact checkpoints;
- resumable runs;
- planner/executor separation;
- human revision loops;
- provider abstraction.

Everward must re-express any useful patterns in its own Unreal/game-domain architecture and terminology. Avoid importing ViMax classes, schemas, agent prompts, or assumptions directly into probe simulation code unless a separate technical evaluation proves a concrete need.

## Phased plan

### Phase 0 - Source review

Document which ViMax ideas are:

- media-production capabilities;
- generic software architecture patterns;
- irrelevant to Everward.

Do not treat the repository as a drop-in game AI framework.

### Phase 1 - Trailer pilot

Use approved Everward screenshots/video captures and non-shippable concept material to produce one short internal teaser prototype. Evaluate:

- storyboard quality;
- shot consistency;
- manual correction burden;
- model/API cost;
- reproducibility;
- provenance completeness.

### Phase 2 - Repeatable marketing template

If useful, define a stable trailer/devlog recipe:

- input asset manifest;
- target aspect ratio/duration;
- storyboard stages;
- narration/caption policy;
- human approval points;
- final output handoff to Remotion or another compositor.

### Phase 3 - Agent-pattern research note

Extract only the most relevant patterns for Everward's probe/descendant systems. Candidate mapping:

```text
probe goal
  -> planner
  -> prioritized tasks
  -> execution
  -> observation
  -> persistent state
  -> replan
  -> child instruction/mutation boundary
```

Any actual game implementation belongs in Everward architecture documents and Unreal code, not in this integration layer.

### Phase 4 - Production hardening

For media workflows that remain useful:

- pin ViMax version;
- track provider/model versions;
- store non-secret configuration;
- separate credentials;
- archive generation provenance;
- require human publish approval;
- ensure workflows fail safely when providers are unavailable.

## Acceptance criteria

Use ViMax routinely only if:

- it reduces trailer/devlog production effort;
- outputs remain human-reviewed;
- provider/model licensing is traceable;
- media tooling remains external to Unreal;
- useful agent ideas can be implemented independently in Everward without source coupling;
- marketing automation never publishes automatically without an explicit future policy change.

## Non-goals

- making ViMax a gameplay dependency;
- using a media agent as the probe simulation engine;
- importing third-party prompts/agent state as canonical game behavior;
- auto-publishing promotional content without review;
- feeding copyrighted source media into generation without rights.

## Rollback

Stop using ViMax and retain only approved exported media and independent Everward architecture. No game code, save data, or Unreal build should depend on ViMax.

## Agent rule

Agents may study ViMax and build external tooling around it. They must preserve conceptual separation between **reference architecture**, **marketing pipeline**, and **game runtime**, and must follow `docs/IP_AND_LICENSES.md` for every generated asset.
