# Everward Asset Scout and Acquisition Pipeline

## Purpose

Everward may use third-party 3D models, PBR materials, textures, HDRIs, and supporting environment assets, but it cannot trade commercial IP discipline for speed.

The Asset Scout pipeline gives autonomous development a controlled route from **"we need an asset"** to **"this file is allowed, inspected, registered, and ready for Unreal"**:

`search -> license verdict -> ignored staging -> technical QA -> promotion -> provenance registry -> Unreal import`

Search results are not production assets. A file does not become production-eligible merely because it is free or downloadable.

## Current integration

Everward integrates with `arielshad/3d-asset-server` through its HTTP `/v1` API.

The compatibility point currently tested by Everward is:

`5914a8fd280b79d00fc6b0783c7e7d7b6affd654`

The upstream server is development tooling; it is not a shipping runtime dependency.

The default Everward endpoint is deliberately local:

`http://127.0.0.1:8787`

This avoids making production development depend on an unpinned public service. The public server can still be supplied explicitly with `--server https://3d.shep.bot` when appropriate.

## Install the pinned local server on Windows

From the Everward repository:

```powershell
powershell -ExecutionPolicy Bypass -File tools/Setup_Everward_Asset_Server.ps1
```

Then start the server in a separate terminal using the command printed by the setup script.

The setup script clones the upstream project under ignored `.tools/`, checks out the tested commit, runs `npm ci`, and builds it. Node.js 20+ is required.

## Search

```text
python tools/everward_asset_scout.py search "carbon composite spacecraft panel" --type material
python tools/everward_asset_scout.py search "asteroid rock" --type model
python tools/everward_asset_scout.py search "deep space star field" --type hdri
```

Each result receives one of three Everward verdicts:

- `approved` — may proceed automatically to staging;
- `review` — a human/legal/IP decision is required before the policy can be expanded;
- `rejected` — cannot proceed under the current policy.

The initial automatic allowlist is intentionally narrow: directly downloadable, explicitly commercial-use CC0 assets from approved providers with no attribution requirement. A model claiming "royalty free," "free," or "CC0" on a provider outside the allowlist does **not** bypass review.

## Stage

```text
python tools/everward_asset_scout.py stage polyhaven:ASSET_ID --format gltf --resolution 2k
```

Staging writes into `assets/staging/`, which is ignored by Git. The staged directory contains:

- `payload/` — downloaded/extracted source files;
- `provenance.json` — asset ID, source, provider, server compatibility point, license evidence, requested format/resolution, and licensing verdict.

Archive extraction rejects path traversal, symlinks, and oversized expansion.

## Technical QA

```text
python tools/everward_asset_scout.py qa assets/staging/polyhaven-asset-id
```

The dependency-free gate currently checks:

- forbidden executable/script payloads;
- per-file size limits;
- recognized Unreal-importable file presence;
- SHA-256 for every file;
- glTF 2.x validity at the JSON/header level;
- missing external glTF buffers/images;
- GLB header/version/declared-length consistency.

This gate is intentionally conservative. It is not a substitute for the Unreal import/build/performance gate. Polygon count, texture memory, material count, collision, LOD/Nanite suitability, shader cost, visual quality, and gameplay fit remain later Unreal/Product Reality evidence.

## Promote

```text
python tools/everward_asset_scout.py promote assets/staging/polyhaven-asset-id
```

Promotion only succeeds when:

1. current policy still auto-approves the license/provider combination;
2. staging provenance recorded the same approved verdict;
3. `qa.json` passes.

Promotion copies the source package into:

`assets/third_party/<asset-key>/`

and updates the canonical machine-readable registry:

`assets/third_party/asset_registry.json`

The registry records provenance, license, source URL, hashes, repository paths, and intended Unreal destination.

It also regenerates:

`assets/pipeline/unreal_import_manifest.json`

## Unreal import

`unreal/Scripts/import_approved_assets.py` consumes the import manifest inside Unreal's Python environment.

Before Unreal imports a file, the script recomputes SHA-256 and refuses hash drift. Imported assets go under `/Game/ThirdParty/<asset-key>`.

Import is **not** the final production gate. An imported asset still needs appropriate Unreal-side checks for the role it will play, including scale/units, normals/tangents, materials, texture settings, collision, LOD/Nanite strategy, performance, art direction, and packaging behavior.

## CI invariant

Foundation CI runs:

```text
python tools/everward_asset_scout.py validate
```

This fails if the asset registry is malformed or the Unreal import manifest no longer matches it. Unit tests exercise licensing verdicts, archive traversal protection, glTF dependency checks, dangerous-payload rejection, hashing, promotion, and stale-manifest detection without requiring network access.

## Agent rule

Autonomous agents may search widely, but they may not:

- commit a discovered third-party asset directly;
- bypass an `everward_verdict=review`;
- interpret "free" as commercial/redistribution permission;
- use a search-provider summary as a substitute for the recorded asset license;
- replace the canonical Prime Generation-1 probe design with generic marketplace art;
- promote material that fails QA;
- silently modify the allowlist to get a desired result.

If a useful asset lands in `review`, the correct outcome is a review request containing the exact listing, provider, license evidence, intended use, redistribution implications, and proposed policy decision.

## Everward-specific scope

This system is most useful now for supporting visual content around proven gameplay: asteroid/regolith assets, materials, industrial surfaces, debris, structural/truss references, machinery/support components, HDRIs, and similar environment production inputs.

The Prime probe remains a canonical Everward design. Asset Scout may supply references or supporting components, but generic found assets must not redefine the player's body merely because they are convenient.

## Future gates

The next useful extensions after real assets begin flowing through the pipeline are:

1. Blender/Assimp or Unreal-derived mesh statistics;
2. automatic poly/material/texture-memory budgets;
3. Nanite/LOD suitability report;
4. scale/unit and pivot checks;
5. collision-complexity checks;
6. automated Unreal import smoke run on the unattended Windows worker;
7. representative-scene screenshot/performance comparison;
8. attribution/notice generation for licenses intentionally admitted beyond the initial CC0-only path.

Those extensions should build on the existing registry rather than creating a second asset-tracking system.
