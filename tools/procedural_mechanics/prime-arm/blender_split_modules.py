"""
Optional, safeguarded Blender 4.x split/export fallback.

Run in a disposable Blender *background* instance only:
  blender --background --factory-startup --python blender_split_modules.py -- \
    teardown-out/prime-arm-bridge/prime-arm-modules.glb \
    teardown-out/prime-arm-bridge/fbx

Blender import should convert glTF Y-up module geometry into Blender Z-up.
This script refuses to run inside any saved .blend file, requires exactly the
three normalized module meshes, and writes one FBX per module. Verify the
resulting pivots/axes in Unreal before acceptance. Never run as a startup hook.
"""
from pathlib import Path
import sys

import bpy


EXPECTED = {"EW_Arm_Upper", "EW_Arm_Forearm", "EW_Arm_MiningTool"}


def arguments():
    if "--" not in sys.argv:
        raise RuntimeError("Supply input GLB and output directory after --")
    values = sys.argv[sys.argv.index("--") + 1 :]
    if len(values) != 2:
        raise RuntimeError("usage: blender ... -- <input.glb> <output-directory>")
    source, target = Path(values[0]).resolve(), Path(values[1]).resolve()
    if not source.is_file() or source.suffix.lower() != ".glb":
        raise FileNotFoundError(source)
    if target == source.parent.parent or target == Path.cwd():
        raise RuntimeError("Destination must be a separate, explicit output folder")
    return source, target


def main():
    if bpy.data.filepath:
        raise RuntimeError("Refusing to modify an open or saved Blender project")
    source, target = arguments()
    # In the requested factory-startup background process, removing the
    # default cube/camera affects only the isolated temporary scene.
    bpy.ops.object.select_all(action="SELECT")
    bpy.ops.object.delete(use_global=False)
    bpy.ops.import_scene.gltf(filepath=str(source))
    mesh_objs = [o for o in bpy.data.objects if o.type == "MESH"]
    matched = {o.name: o for o in mesh_objs if o.name in EXPECTED}
    if set(matched) != EXPECTED or len(mesh_objs) != 3:
        raise RuntimeError(
            f"Expected only 3 separately named, unit-normalized parts, "
            f"found {[o.name for o in mesh_objs]}; importer must be reviewed"
        )
    target.mkdir(parents=True, exist_ok=True)
    for name in sorted(EXPECTED):
        mesh = matched[name]
        # Preserve evaluated world transform when detaching any importer
        # correction root. Blender's exporter then applies the expected
        # +X-forward/+Z-up convention for the next Unreal import stage.
        m = mesh.matrix_world.copy()
        mesh.parent = None
        mesh.matrix_world = m
        # Reject secretly rescaled/translated mesh pivots: otherwise the
        # existing Unreal component offset would cause visible contact drift.
        if any(abs(v) > 1e-4 for v in mesh.location):
            raise RuntimeError(f"Non-zero module origin: {name} {tuple(mesh.location)}")
        bpy.ops.object.select_all(action="DESELECT")
        mesh.select_set(True)
        bpy.context.view_layer.objects.active = mesh
        out = target / f"{name}.fbx"
        bpy.ops.export_scene.fbx(
            filepath=str(out),
            use_selection=True,
            object_types={"MESH"},
            axis_forward="X", axis_up="Z",
            apply_unit_scale=True,
            bake_anim=False,
            add_leaf_bones=False,
        )
        if not out.is_file() or out.stat().st_size == 0:
            raise RuntimeError(f"FBX export absent: {out}")
        print(f"Everward module export: {name} -> {out}")
    print("Three FBX modules exported; Unreal pivot/scale tests remain REQUIRED")


if __name__ == "__main__":
    main()
