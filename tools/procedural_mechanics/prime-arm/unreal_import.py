"""
Everward Mechanical Bridge v0.1 — opt-in test-only Unreal 5.8 GLB import.

Execute inside Unreal Editor's Python environment from a disposable test
checkout, NEVER as a normal launcher/startup side effect. The glTF importer
must be enabled and must yield 3 independently addressable StaticMeshes.
The importer cannot certify joint physics or Product Reality.
"""
import os
from pathlib import Path

import unreal


def main() -> None:
    if os.environ.get("EVERWARD_ARM_IMPORT_SAVE") != "1":
        raise RuntimeError(
            "Refusing to import into Unreal content. Set "
            "EVERWARD_ARM_IMPORT_SAVE=1 only in a disposable test checkout."
        )
    # Unreal's project path points at .../Project-Everward/unreal/.
    repo_root = Path(unreal.Paths.project_dir()).resolve().parent
    source = repo_root / "teardown-out" / "prime-arm-bridge" / "prime-arm-modules.glb"
    if not source.is_file():
        raise FileNotFoundError(
            f"No validated GLB at {source}. Run export_glb.mjs first."
        )
    destination = "/Game/MechanicalBridge/Pilot"
    task = unreal.AssetImportTask()
    task.filename = str(source)
    task.destination_path = destination
    task.automated = True
    task.save = True
    task.replace_existing = False
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
    paths = list(task.imported_object_paths or [])
    meshes = []
    for asset_path in paths:
        item = unreal.load_asset(asset_path)
        if isinstance(item, unreal.StaticMesh):
            meshes.append((str(asset_path), item))
    names = {mesh.get_name() for _, mesh in meshes}
    unreal.log(f"Everward mechanical bridge imported {len(meshes)} static meshes: {names}")
    if len(meshes) < 3:
        raise RuntimeError(
            "Import did not produce separate upper, forearm and tool "
            "StaticMesh assets. Do not enable the pawn overrides; inspect "
            "Interchange import settings or use a reviewed Blender split."
        )
    for path, mesh in meshes:
        # Displayed by Unreal in centimetres; the normalized module is
        # 1 metre long and is then scaled by the existing pawn components.
        bounds = mesh.get_bounds()
        unreal.log(f"Mechanical bridge test mesh {path}: bounds {bounds}")
    unreal.log_warning(
        "Mechanical bridge import is TEST ONLY. Set the matching three "
        "ProceduralArm visual assets on the test pawn, verify pivots and "
        "tool-tip contact in Unreal before claiming acceptance."
    )


if __name__ == "__main__":
    main()
