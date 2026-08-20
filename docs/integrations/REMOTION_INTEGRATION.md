# Remotion Integration Plan

Last reviewed: 2026-08-20
Upstream: `remotion-dev/remotion`
Integration posture: **Separate programmable media-composition tool for trailers/devlogs; never an Unreal runtime dependency**

## Purpose

Remotion can provide a deterministic, code-driven layer for assembling Everward trailers, development updates, social clips, comparisons, captions, overlays, and repeatable presentation formats from captured game footage and approved media assets.

Its value is repeatability: once a video template is coded, future releases can feed new screenshots, gameplay clips, metrics, text, and audio into the same composition pipeline.

## License gate

Remotion uses its own license rather than a standard open-source license. As of this review, individuals and eligible small organizations may use it under the free license, while larger for-profit organizations require a company license. The project has also announced licensing changes around Remotion 5.0.

Before every commercial production rollout or major upgrade:

- review the current Remotion license text;
- confirm the user/business remains eligible for the selected license tier;
- record the Remotion version;
- configure the required license declaration/key where the installed version requires it;
- separately review licenses for fonts, music, stock media, codecs, and third-party packages.

Do not assume today's free-license eligibility is permanent.

## Architectural boundary

Remotion should live in a separate media-tool directory/repository or external tooling workspace, not inside Unreal modules.

```text
Everward packaged build / capture session
  -> approved screenshots + gameplay clips + metadata
  -> Remotion composition
      -> title cards
      -> captions
      -> transitions
      -> overlays
      -> audio mix references
  -> rendered MP4/WebM/etc.
  -> human review
  -> publish
```

Everward must build and ship with no Node/React/Remotion dependency.

## Initial use cases

- 15-30 second teaser template;
- milestone/progress devlog template;
- before/after visual comparison clips;
- feature highlight reels;
- vertical social-media variants;
- automated caption/title-card insertion;
- build/version watermark or provenance overlays for internal test media.

## Phased plan

### Phase 0 - Separate tooling skeleton

Create an external media workspace with:

- pinned Node/Remotion versions;
- one minimal composition;
- documented render command;
- local input/output directories;
- no Unreal runtime coupling;
- no committed credentials.

### Phase 1 - Teaser template

Build one deterministic teaser composition accepting:

- project title/build label;
- 3-8 short gameplay/video clips;
- optional stills;
- narration/audio track;
- captions;
- target aspect ratio and duration.

### Phase 2 - Asset manifest

Drive compositions from a small manifest rather than hard-coded paths. Record:

- input path/ID;
- source build/version;
- rights/provenance status;
- caption/text;
- trim points;
- intended placement;
- output target.

### Phase 3 - Automated variants

Generate horizontal and vertical outputs from the same approved content where practical. Keep final framing human-reviewed because gameplay/UI crops may require manual decisions.

### Phase 4 - Optional upstream automation

Allow ViMax or another approved storyboard system to prepare a candidate manifest, but Remotion remains the deterministic compositor. Human approval occurs before final publish.

### Phase 5 - CI/build integration only if justified

If automated media generation becomes valuable, use a separate workflow/job. Do not make video rendering a required game CI check or block normal Unreal builds.

## Acceptance criteria

Standardize Remotion only if:

- one template can be reused across multiple Everward updates;
- media rendering is deterministic enough for repeat use;
- license eligibility is documented;
- the game repository/runtime remains independent;
- source-media provenance is preserved;
- failed media renders cannot affect the game build;
- human review remains the publication gate.

## Non-goals

- rendering in-game cinematics at runtime;
- adding React/Node dependencies to Unreal gameplay modules;
- automatic publication without review;
- storing copyrighted music or stock media without redistribution rights;
- relying on Remotion-specific formats as the only archive of source footage.

## Rollback

Stop using the Remotion tooling and retain ordinary source footage, manifests, and rendered media. No Unreal migration is required.

## Agent rule

Agents may create or update external Remotion templates, but must re-check licensing on material version changes, keep the tool outside the game runtime, preserve input-media provenance, and never auto-publish unless repository policy is explicitly changed.
