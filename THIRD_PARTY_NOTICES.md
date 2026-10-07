# Third-Party Notices

Everward is proprietary. Third-party components remain governed by their own terms.

This file is the human-readable shipping-notice index. The detailed working provenance record lives in `docs/IP_AND_LICENSES.md` and should eventually be backed by machine-readable dependency/asset manifests.

## Current shipping components

None recorded yet. Everward is in pre-production; Unreal Engine is the accepted production direction, but no third-party asset has yet been admitted to the shipping-component list.

## Development-only tooling

| Component | Revision | Purpose | License | Shipped |
|---|---|---|---|---|
| `arielshad/3d-asset-server` | `5914a8fd280b79d00fc6b0783c7e7d7b6affd654` | External asset search/download development service, installed locally by script rather than vendored into Everward | Apache-2.0 | No |

The development tool's Apache-2.0 license applies to that software only. Models, textures, materials, HDRIs, and other content discovered through it retain their own licenses and must pass Everward's asset provenance gate independently.

## Required record for each shipped component

Record, at minimum:

| Field | Required information |
|---|---|
| Component | Name and version/revision |
| Type | Library, engine, plugin, asset, font, audio, data, shader, tool output, etc. |
| Source | Canonical source/vendor |
| Copyright owner | As stated by the supplier |
| License | SPDX identifier where available, otherwise exact license name |
| Commercial use | Confirmed yes/no/conditional |
| Redistribution | Conditions for including it in builds |
| Attribution | Text/location required by the license |
| Modification | Any notice obligations for modified versions |
| Evidence | Saved license file, vendor terms, receipt, or other provenance record |
| Used in shipping build | Yes/no |

## Rule

No third-party material should enter a shipping build until its provenance and redistribution rights are known. Reference or concept material is not automatically cleared for production use.
