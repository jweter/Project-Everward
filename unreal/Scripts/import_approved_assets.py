"""Import Everward-approved third-party assets into Unreal.

Run this script inside the Unreal Python environment after regenerating
assets/pipeline/unreal_import_manifest.json. It verifies the registry hashes
before asking Unreal to import anything.
"""

from __future__ import annotations

import hashlib
import json
from pathlib import Path

import unreal


REPO_ROOT = Path(__file__).resolve().parents[2]
MANIFEST = REPO_ROOT / "assets" / "pipeline" / "unreal_import_manifest.json"
RESULTS = REPO_ROOT / "artifacts" / "asset-scout" / "unreal_import_results.json"


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for chunk in iter(lambda: handle.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def main() -> None:
    payload = json.loads(MANIFEST.read_text(encoding="utf-8"))
    if payload.get("schema_version") != 1:
        raise RuntimeError("Unsupported Everward asset import manifest schema")

    results = []
    asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
    for item in payload.get("imports", []):
        source = REPO_ROOT / item["source"]
        result = {
            "asset_id": item["asset_id"],
            "source": item["source"],
            "destination": item["destination"],
            "status": "pending",
            "imported_object_paths": [],
        }
        if not source.is_file():
            result["status"] = "missing_source"
            results.append(result)
            continue
        actual_hash = sha256(source)
        if actual_hash != item["sha256"]:
            result["status"] = "hash_mismatch"
            result["actual_sha256"] = actual_hash
            results.append(result)
            continue

        task = unreal.AssetImportTask()
        task.filename = str(source)
        task.destination_path = item["destination"]
        task.automated = True
        task.save = True
        task.replace_existing = False
        task.replace_existing_settings = False
        asset_tools.import_asset_tasks([task])

        imported = list(task.imported_object_paths)
        result["imported_object_paths"] = imported
        result["status"] = "imported" if imported else "unreal_import_failed"
        results.append(result)

    RESULTS.parent.mkdir(parents=True, exist_ok=True)
    RESULTS.write_text(
        json.dumps({"schema_version": 1, "results": results}, indent=2, sort_keys=True) + "\n",
        encoding="utf-8",
    )

    failures = [result for result in results if result["status"] != "imported"]
    if failures:
        raise RuntimeError(
            f"Everward asset import completed with {len(failures)} failure(s); see {RESULTS}"
        )
    unreal.log(f"Everward asset import succeeded for {len(results)} source file(s)")


if __name__ == "__main__":
    main()
