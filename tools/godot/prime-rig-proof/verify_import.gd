extends SceneTree

const EXPECTED_PIVOTS: Array[String] = [
    "EW_port_ShoulderPivot",
    "EW_port_ElbowPivot",
    "EW_port_WristPivot",
    "EW_port_AuthoritativeToolTip",
    "EW_starboard_ShoulderPivot",
    "EW_starboard_ElbowPivot",
    "EW_starboard_WristPivot",
    "EW_starboard_AuthoritativeToolTip"
]
const EXPECTED_MESHES: Array[String] = [
    "EW_Arm_Upper", "EW_Arm_Forearm", "EW_Arm_MiningTool"
]

func _initialize() -> void:
    call_deferred("_verify")

func _visit(n: Node, names: Dictionary, mesh_count: Array[int]) -> void:
    names[n.name] = true
    if n is MeshInstance3D:
        mesh_count[0] += 1
    for child in n.get_children():
        _visit(child, names, mesh_count)

func _verify() -> void:
    var missing: Array[String] = []
    var assembly: PackedScene = load("res://assets/prime-arm-assembly.glb")
    var modules: PackedScene = load("res://assets/prime-arm-modules.glb")
    if assembly == null or modules == null:
        printerr("EVERWARD_GLB_IMPORT_FAIL: packed scene unavailable after Godot import")
        quit(2)
        return
    var arm_root: Node = assembly.instantiate()
    var module_root: Node = modules.instantiate()
    var assembly_names: Dictionary = {}
    var module_names: Dictionary = {}
    var count: Array[int] = [0]
    _visit(arm_root, assembly_names, count)
    var assembly_mesh_count: int = count[0]
    count[0] = 0
    _visit(module_root, module_names, count)
    var module_mesh_count: int = count[0]
    for expected in EXPECTED_PIVOTS:
        if not assembly_names.has(expected):
            missing.append(expected)
    for expected in EXPECTED_MESHES:
        if not module_names.has(expected):
            missing.append(expected)
    print("EVERWARD_GLB_IMPORT_ASSEMBLY_MESH_COUNT=", assembly_mesh_count)
    print("EVERWARD_GLB_IMPORT_MODULE_MESH_COUNT=", module_mesh_count)
    print("EVERWARD_GLB_IMPORT_ASSEMBLY_NODE_NAMES=", JSON.stringify(assembly_names.keys()))
    print("EVERWARD_GLB_IMPORT_MODULE_NODE_NAMES=", JSON.stringify(module_names.keys()))
    print("EVERWARD_GLB_IMPORT_MISSING=", JSON.stringify(missing))
    var passed: bool = missing.is_empty() and assembly_mesh_count >= 6 and module_mesh_count >= 3
    print("EVERWARD_GLB_IMPORT_STATUS=", "PASS" if passed else "FAIL")
    arm_root.free()
    module_root.free()
    quit(0 if passed else 2)
