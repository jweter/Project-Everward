# ComfyUI Integration Plan

Last reviewed: 2026-08-20
Upstream: `Comfy-Org/ComfyUI`
Integration posture: **External local creative-generation backend, never linked into the proprietary Unreal runtime**

## Purpose

ComfyUI can provide a repeatable local node-based backend for concept art, texture ideation, environment studies, promotional images, storyboard frames, and selected production assets when provenance and commercial-use rights are clear.

## License boundary

ComfyUI itself is currently GPL-3.0. To preserve Everward's proprietary licensing posture, the preferred integration is process/service separation: run ComfyUI independently and exchange files/requests through documented external interfaces. Do not copy, statically link, or incorporate ComfyUI GPL source into Everward code without a separate legal/architecture decision.

Model, LoRA, VAE, ControlNet, custom-node, and other asset licenses are independent and may be more restrictive than ComfyUI itself. Every workflow intended to produce shipped assets must have a license manifest for every material model/node component.

## Target workflow

```text
Everward art brief / approved references
  -> versioned ComfyUI workflow
  -> local generation
  -> candidate output + generation metadata
  -> human art review/editing
  -> provenance record
  -> approved concept or production asset
  -> Unreal import pipeline if shippable
```

ComfyUI is an authoring tool. Unreal assets remain normal exported files; the game must run and build without ComfyUI installed.

## Phased plan

### Phase 0 - Environment and policy

- Run ComfyUI outside the Unreal repository/runtime.
- Define allowed model sources and commercial-use requirements.
- Create an asset-provenance template covering workflow, model IDs/versions, prompts, reference inputs, seed/settings, date, human edits, and output path.
- Prohibit unknown-license custom nodes/models from production workflows.

### Phase 1 - Concept-art pilot

Build 3-5 versioned workflows for low-risk non-shipped exploration:

- deep-space environment mood studies;
- probe silhouette/material exploration;
- stellar phenomena concepts;
- UI/background visual studies;
- marketing storyboard frames.

Evaluate consistency, controllability, GPU cost, and iteration speed.

### Phase 2 - Reproducible workflow storage

Store only safe workflow JSON/configuration and documentation in GitHub. Do not commit model weights or generated assets whose redistribution rights are unclear. Pin custom-node revisions where used.

### Phase 3 - Production-asset gate

An AI-assisted asset may enter a shippable Unreal content directory only after:

- commercial-use license review;
- provenance record completion;
- human modification/review where appropriate;
- visual originality/IP review;
- technical optimization and Unreal validation;
- explicit classification as `SHIPPABLE`, not merely `CONCEPT`.

### Phase 4 - Automation API

If useful, add a separate tooling script/service that submits approved workflows to local ComfyUI and collects outputs/metadata. Keep credentials/network exposure minimal and local by default.

## Security rules

- Bind local services to loopback/private interfaces unless there is a documented reason not to.
- Treat third-party custom nodes as executable code; review and pin them.
- Do not feed confidential, copyrighted, or provenance-unknown source art into production generation workflows.
- Do not auto-install arbitrary custom nodes from generated workflow files.

## Acceptance criteria

Adopt ComfyUI as a standard Everward authoring tool only if:

- workflows measurably improve art iteration;
- production workflows have complete model/node license inventories;
- outputs can be reproduced sufficiently for audit/debugging;
- Unreal builds remain entirely independent;
- concept vs shippable status is explicit;
- custom-node and model updates are controlled rather than floating.

## Non-goals

- making ComfyUI part of gameplay;
- embedding GPL code into proprietary game modules;
- automatically shipping generated output;
- assuming model licenses permit commercial use;
- using AI generation to replace required Unreal technical art workflows.

## Rollback

Because ComfyUI is external, removal means stopping generation and retaining already approved exported assets with their provenance. No Unreal code or save-data migration should be required.

## Agent rule

Agents may add tooling around external ComfyUI workflows, but must not add ComfyUI source to the Everward codebase, auto-approve generated assets, or bypass the IP/license/provenance gate in `docs/IP_AND_LICENSES.md`.
