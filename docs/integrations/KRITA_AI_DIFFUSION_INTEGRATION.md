# Krita AI Diffusion Integration Plan

Last reviewed: 2026-08-20
Upstream: `Acly/krita-ai-diffusion`
Integration posture: **External artist-in-the-loop authoring tool; reference and production aid, not runtime dependency**

## Purpose

Krita AI Diffusion provides an artist-controlled interface for inpainting, outpainting, reference-guided generation, ControlNet-style conditioning, and ComfyUI-backed workflows. For Everward its value is not autonomous asset generation; it is faster human-directed iteration and cleanup.

## License boundary

The plugin is currently GPL-3.0 and uses ComfyUI as a backend. Keep Krita and the plugin outside the proprietary Everward runtime and source tree. Exchange only ordinary authored/exported assets and provenance records.

Model and backend licenses remain separate. A permissively licensed plugin does not confer rights to model output, and GPL licensing of the plugin does not make every exported image GPL; nevertheless, production use still requires provenance and model/license review.

## Recommended use cases

- paint-over and refinement of approved concept art;
- controlled inpainting of composition defects;
- environment and probe design exploration;
- rough texture/reference studies;
- marketing composition iteration;
- creating references for human 3D/technical art work.

## Target workflow

```text
art brief
  -> Krita source document
  -> human drawing/paint/edit
  -> optional AI Diffusion operation
  -> human selection/refinement
  -> provenance record
  -> concept or shippable review gate
  -> exported image/reference
```

Keep layered `.kra` working files local or in the repository only when source/reference licensing permits redistribution.

## Phased plan

### Phase 0 - Tooling policy

Define:

- approved model families;
- allowed reference-image sources;
- production vs concept-only workflows;
- metadata/provenance fields;
- storage rules for layered source files and generated intermediates.

### Phase 1 - Concept pilot

Use the plugin for a limited set of concept tasks and measure whether it improves control compared with raw prompt-only generation. Favor tasks where human selection and paint-over are central.

### Phase 2 - Reusable art recipes

Document a small number of repeatable workflows for:

- space environment expansion;
- probe/structure paint-over;
- material variation;
- controlled background replacement;
- composition repair.

Do not copy upstream plugin code into Everward to recreate these capabilities.

### Phase 3 - Production gate

Before any AI-assisted result becomes shipped game/marketing art:

- verify all source/reference rights;
- verify model commercial-use rights;
- archive relevant tool/model/version information;
- identify meaningful human edits;
- perform originality/trademark/franchise review;
- optimize/export through normal art production standards.

### Phase 4 - Team/reproducibility support

If the workflow becomes important, document installation versions, ComfyUI backend expectations, approved model inventory, and example non-proprietary workflow files so it can be recreated without making it a build dependency.

## Acceptance criteria

Standardize Krita AI Diffusion only if:

- it materially improves artist control or iteration speed;
- production provenance can be recorded consistently;
- no Everward build/runtime dependency is introduced;
- the art workflow remains usable in plain Krita when AI tooling is unavailable;
- model/reference licensing is explicit.

## Non-goals

- bulk-autogenerating final game art without review;
- replacing Unreal materials, Niagara, modeling, or technical art;
- shipping the Krita plugin with the game;
- using unlicensed franchise art as generation reference;
- treating AI edits as provenance-free because a human clicked the button.

## Rollback

The project should retain normal image/source assets. Removing Krita AI Diffusion affects only the authoring workflow; approved exports and Unreal content remain valid subject to their recorded provenance.

## Agent rule

Agents may document or automate setup and asset metadata, but cannot declare AI-assisted artwork shippable, ingest unknown-license reference art, or add GPL plugin source into proprietary Everward modules.
