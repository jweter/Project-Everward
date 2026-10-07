# Assets

Everward's tracked asset tree distinguishes original/reference material from third-party production inputs.

Every third-party asset must have known provenance, explicit commercial-use evidence, redistribution-compatible terms for this public repository, passing technical QA, and a machine-readable registry entry before production promotion.

Use the controlled pipeline documented in `docs/ASSET_ACQUISITION_PIPELINE.md`:

`search -> license verdict -> ignored staging -> technical QA -> promotion -> registry -> Unreal import`

Canonical files:

- `assets/pipeline/policy.json` — fail-closed licensing and technical policy;
- `assets/staging/` — ignored quarantine for downloaded candidates;
- `assets/third_party/asset_registry.json` — promoted third-party provenance authority;
- `assets/pipeline/unreal_import_manifest.json` — generated Unreal import jobs;
- `assets/reference/` — concept/reference material, not automatically cleared for shipping.

Do not commit a discovered asset directly and do not treat concept/reference assets as automatically cleared for shipping.
