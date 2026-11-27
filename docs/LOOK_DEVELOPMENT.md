# Everward look development

How the owner's reference images become the look of the game. This works like a UI design system: define the smallest pieces first (tokens), build materials from tokens, assets from materials, and scenes from assets. Every scene then inherits one consistent look instead of being tuned by hand.

## Where the references live

| Set | Where | Why |
|---|---|---|
| Prime Probe canon (9 sheets) | `assets/reference/probe/gen1-prime` (in the repo) | Owner's own design sheets; hash-checked by `tools/validate_reference_assets.py` |
| Builder Bot concept | `assets/reference/builder-bot` (in the repo) | Owner-supplied, exploratory |
| Mood images, owner photos, exploded-view sheet | **Outside the repo**, owner's machine: `Documents\Everward-References` with `manifest.json` (SHA-256) and `palettes.json` | This repository is **public**. The mood set includes third-party wallpapers from Reddit whose artists and licences are unknown, and the exploded-view sheet carries a NASA/JPL logo it should not be published with. Only values derived from them (colours, lighting numbers) are committed. |

The owner re-sent the nine canon sheets on 2026-11-27; they matched the stored files (mean pixel difference 0.2-0.4), so nothing was re-added.

## Layer 1: tokens (atoms)

Palettes were sampled from each mood image by median-cut quantisation to six colours, after cropping the Reddit frame. They live in `tools/godot/world-spike/looks.gd`.

**Planet looks** (wired into the spike now):

| Look | Built from | Sky | Light | Fog | Ground and plants |
|---|---|---|---|---|---|
| `ember_dusk` | owner photo of a low sun under an altocumulus deck | slate top `#3c4458`, horizon `#ae7456` | warm `#ffb27a`, sun 9 degrees up | `#78594f` | dark grass `#313d1e`, rock `#2d2521` |
| `moonrise` | red moon over misty conifers; moon over a storm sea | teal `#0e2a33`, rose horizon `#ab8376` | moonlight `#f0b39c`, low energy | dense teal `#25555f` | near-black conifers `#020a0b` |
| `storm_ridge` | grass ridge in a break of storm light | `#3a3f3a`, bright gap `#c7bba9` | strong warm-white `#fff0d4` | olive grey `#838172` | wet black rock `#141814`, lit grass `#8fae3a` |
| `autumn_mist` | misty autumn lake; owner photo of an overcast marsh | white-grey `#c3ccd7` | soft `#fff4ea` | thick `#d0d8e1` | dark spruce `#12171b` with 45% orange larches `#d2601e` |

**Space tokens** (defined, not yet used; for space and black-hole scenes): `accretion_gold`, `accretion_ash`, `lensed_white`, `nebula_red`, `nebula_teal_ember`, `vortex_blue`.

**Probe materials** come from canon sheet 07: machined dark alloy, bead-blasted panels, dark ceramic tile, carbon fibre, radiator mesh, coated optics with purple/green sheen, heat-tinted engine metal, hardened steel and bronze accents, with clean operational wear and no rust.

## Layer 2: materials (molecules)

Shaders read tokens through uniforms, so a look changes colour without changing code: `terrain.gdshader` (grass, rock, snow), `tree.gdshader` (foliage, trunk, autumn mix), `cloud.gdshader` (lit and shadow colour, coverage, sun direction). Still to build: the probe's PBR material set from sheet 07, water, and the space shaders.

## Layer 3 and up: assets, then scenes

Assets (probe modules, trees, rocks, the Builder Bot) are built only from Layer 2 materials. Scenes pick one look. Interface screens (HUD, menus) get their own design system from the same tokens. The suggested route is a design system made in Claude's design tool from these palettes plus HUD screenshots the owner likes, then a skill that points to it, so every new screen starts from the system.

## Evidence so far

`godot --path tools/godot/world-spike -- --lookshots --look=NAME --out=FOLDER` renders the same three framings (overview above the clouds, forest, mountain) with seeded tree placement. All five runs (default plus four looks) exit 0 with zero script errors, at about 55-60 FPS on the dev laptop's integrated GPU.

Visually, ember dusk and moonrise carry their references' colour mood clearly, and autumn mist shows orange trees in fog. Not yet done:
- the cloud raymarch is grainy at this step count;
- the trees and ship are placeholders;
- there is no water, so the lake references are not represented;
- the probe has no materials from sheet 07;
- the looks have not been judged by the owner or on a real GPU.
