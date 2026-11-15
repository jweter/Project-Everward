extends SceneTree

# A native Godot Jolt fixture. This verifies local collision-query plumbing
# only. Everward's C++ simulation remains responsible for every game outcome.
var failed: int = 0

func _check(ok: bool, label: String) -> void:
    if ok:
        print("EVERWARD_JOLT_PASS: ", label)
    else:
        failed += 1
        printerr("EVERWARD_JOLT_FAIL: ", label)

func _initialize() -> void:
    call_deferred("_run_fixture")

func _run_fixture() -> void:
    var configured: String = str(ProjectSettings.get_setting("physics/3d/physics_engine", "UNKNOWN"))
    print("EVERWARD_JOLT_ENGINE_CONFIG=", configured)
    _check(configured == "Jolt Physics", "explicit Jolt engine configuration")

    var world_root := Node3D.new()
    world_root.name = "LocalCollisionFixture"
    get_root().add_child(world_root)

    var target := StaticBody3D.new()
    target.name = "ResourceTarget_Static"
    target.collision_layer = 2
    target.collision_mask = 0
    var target_collider := CollisionShape3D.new()
    var target_sphere := SphereShape3D.new()
    target_sphere.radius = 2.0
    target_collider.shape = target_sphere
    target.add_child(target_collider)
    world_root.add_child(target)

    var debris := RigidBody3D.new()
    debris.name = "ZeroGravityDebris"
    debris.mass = 9.0
    debris.gravity_scale = 0.0
    debris.linear_damp_mode = RigidBody3D.DAMP_MODE_REPLACE
    debris.angular_damp_mode = RigidBody3D.DAMP_MODE_REPLACE
    debris.linear_damp = 0.0
    debris.angular_damp = 0.0
    debris.continuous_cd = true
    debris.collision_layer = 4
    debris.collision_mask = 2
    debris.position = Vector3(0, 0, 11)
    var debris_collider := CollisionShape3D.new()
    var debris_sphere := SphereShape3D.new()
    debris_sphere.radius = 0.4
    debris_collider.shape = debris_sphere
    debris.add_child(debris_collider)
    world_root.add_child(debris)

    _check(debris.continuous_cd, "fast debris CCD explicitly enabled")
    _check(debris.gravity_scale == 0.0 and debris.linear_damp == 0.0 and debris.angular_damp == 0.0, "zero-g debris has no unintended drag")
    _check(debris.collision_layer == 4 and debris.collision_mask == 2, "purpose-specific collision layers and masks")

    await physics_frame
    await physics_frame

    var state: PhysicsDirectSpaceState3D = get_root().world_3d.direct_space_state
    var ray: PhysicsRayQueryParameters3D = PhysicsRayQueryParameters3D.create(Vector3(0, 0, 7), Vector3(0, 0, -7), 2)
    var hit: Dictionary = state.intersect_ray(ray)
    _check(hit.has("collider") and hit.get("collider") == target, "sensor ray detects intended resource body")
    if hit.has("position"):
        _check(absf((hit["position"] as Vector3).z - 2.0) < 0.05, "sensor ray locates surface, not center")

    var blocked_mask: PhysicsRayQueryParameters3D = PhysicsRayQueryParameters3D.create(Vector3(0, 0, 7), Vector3(0, 0, -7), 8)
    _check(state.intersect_ray(blocked_mask).is_empty(), "unrelated collision layer excluded from query")

    var tool_tip := SphereShape3D.new()
    tool_tip.radius = 0.5
    var overlap_query := PhysicsShapeQueryParameters3D.new()
    overlap_query.shape = tool_tip
    overlap_query.transform = Transform3D(Basis.IDENTITY, Vector3(0, 0, 2.4))
    overlap_query.collision_mask = 2
    overlap_query.collide_with_bodies = true
    var contacts: Array[Dictionary] = state.intersect_shape(overlap_query, 8)
    var hit_target := false
    for c in contacts:
        if c.get("collider") == target:
            hit_target = true
    _check(hit_target, "tool-tip compound-independent shape query detects true contact")

    # A high-speed 16 m step would tunnel through a 4 m target if only
    # checking endpoint overlap. Explicit swept shape queries must detect it.
    var sweep_query := PhysicsShapeQueryParameters3D.new()
    sweep_query.shape = tool_tip
    sweep_query.transform = Transform3D(Basis.IDENTITY, Vector3(0, 0, 8))
    sweep_query.motion = Vector3(0, 0, -16)
    sweep_query.collision_mask = 2
    sweep_query.collide_with_bodies = true
    var sweep_fraction: PackedFloat32Array = state.cast_motion(sweep_query)
    _check(sweep_fraction.size() == 2, "swept shape returns safe and unsafe fractions")
    if sweep_fraction.size() == 2:
        _check(sweep_fraction[0] > 0.30 and sweep_fraction[0] < 0.40, "fast sweep stops before tunneling through asteroid")
        _check(sweep_fraction[1] >= sweep_fraction[0], "sweep fraction ordering is physically valid")

    _check(get_root().world_3d != null, "dedicated local-space physics world available")
    print("EVERWARD_JOLT_LOCAL_QUERY_STATUS=", "PASS" if failed == 0 else "FAIL")
    print("EVERWARD_JOLT_FAILURE_COUNT=", failed)
    world_root.queue_free()
    quit(0 if failed == 0 else 2)
