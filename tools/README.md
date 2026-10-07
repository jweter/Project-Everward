# Tools

Developer and offline tooling belongs here.

Potential uses include astronomy analysis, procedural-generation inspection, save migration utilities, data conversion, profiling, visualization, content validation, benchmark harnesses, and test-fixture generation.

Python or other scripting languages may be used here without implying they become shipping runtime dependencies.

## Foundation validation

`check_foundation.py` is the dependency-free repository constitution check used by GitHub Actions.

Run it locally from the repository root with:

```text
python tools/check_foundation.py
```

This validates required project-foundation files, detects unresolved merge-conflict markers in text source, and checks that project documentation is non-empty. It is intentionally runnable without GitHub Actions so platform/account failures can be distinguished from project test failures.

`test_check_foundation.py` gives every guard clause in `check_foundation.py` direct regression coverage against isolated fixture directories, rather than relying on the ambient repository always happening to pass. Run it locally with:

```text
python -m unittest discover -s tools -p 'test_*.py' -v
```

## Third-party asset acquisition

`everward_asset_scout.py` is the dependency-free Everward gate between external asset discovery and tracked production content. It classifies licensing, stages approved downloads into ignored quarantine, performs local safety/importability QA, promotes passing assets into the provenance registry, and generates Unreal import jobs.

Set up the tested local `3d-asset-server` revision on Windows with:

```powershell
powershell -ExecutionPolicy Bypass -File tools/Setup_Everward_Asset_Server.ps1
```

Then, for example:

```text
python tools/everward_asset_scout.py search "asteroid rock" --type model
python tools/everward_asset_scout.py inspect polyhaven:ASSET_ID
python tools/everward_asset_scout.py stage polyhaven:ASSET_ID --format gltf --resolution 2k
python tools/everward_asset_scout.py qa assets/staging/polyhaven-asset-id
python tools/everward_asset_scout.py promote assets/staging/polyhaven-asset-id
python tools/everward_asset_scout.py validate
```

See `docs/ASSET_ACQUISITION_PIPELINE.md` for the non-bypassable licensing/provenance rules and Unreal handoff.
