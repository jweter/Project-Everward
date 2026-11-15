extends Node3D

# Controlled Godot ENGINE FEASIBILITY proof. Mechanical truth is provisional
# GDScript and MUST be replaced with Everward's authoritative C++ core.
const ASTEROID_CENTER := Vector3(0.0, 0.0, -10.0)
const ASTEROID_RADIUS := 2.4
const TOOL_FORWARD_OFFSET := Vector3(0.9, -0.45, -2.1)
const MAX_STORAGE_KG := 30.0
const MINE_KG := 2.0
const REPAIR_COST_KG := 6.0

var probe: Node3D
var camera: Camera3D
var engine_light: OmniLight3D
var hud: CanvasLayer
var readout: Label
var tip: Label
var message: Label
var velocity := Vector3.ZERO
var energy_j := 950.0
var material_kg := 0.0
var ore_remaining_kg := 24.0
var propulsion_integrity := 0.28
var scanned := false
var auto_approach := false
var repairs_completed := 0
var selftest_errors := 0
var event_text := "BOOT: SURVIVAL POWER ONLINE. SCAN A NEARBY ASTEROID [T]."
var run_selftest := false
var run_benchmark := false
var benchmark_frames := 0
var benchmark_deltas_ms: Array[float] = []
var screenshot_saved := false

func _ready() -> void:
	run_selftest = OS.get_cmdline_user_args().has("--self-test")
	run_benchmark = OS.get_cmdline_user_args().has("--benchmark")
	_build_world()
	_build_probe()
	_build_camera()
	_build_hud()
	_update_hud()
	if run_selftest:
		_run_self_test()

func _material(c: Color, metal: float = 0.0, rough: float = 0.55) -> StandardMaterial3D:
	var m := StandardMaterial3D.new()
	m.albedo_color = c
	m.metallic = metal
	m.roughness = rough
	return m

func _add_mesh(parent: Node3D, mesh: Mesh, location: Vector3, sizing: Vector3, mat: Material) -> MeshInstance3D:
	var inst := MeshInstance3D.new()
	inst.mesh = mesh
	inst.position = location
	inst.scale = sizing
	inst.material_override = mat
	parent.add_child(inst)
	return inst

func _box(parent: Node3D, location: Vector3, sizing: Vector3, mat: Material) -> MeshInstance3D:
	return _add_mesh(parent, BoxMesh.new(), location, sizing, mat)

func _build_world() -> void:
	var env := WorldEnvironment.new()
	env.environment = Environment.new()
	env.environment.background_mode = Environment.BG_COLOR
	env.environment.background_color = Color(0.012, 0.022, 0.051)
	env.environment.ambient_light_source = Environment.AMBIENT_SOURCE_COLOR
	env.environment.ambient_light_color = Color(0.21, 0.29, 0.43)
	env.environment.ambient_light_energy = 0.45
	add_child(env)

	var sun := DirectionalLight3D.new()
	sun.rotation_degrees = Vector3(-42, 34, -12)
	sun.light_color = Color(0.82, 0.90, 1.0)
	sun.light_energy = 1.6
	add_child(sun)

	var asteroid := Node3D.new()
	asteroid.name = "ResourceAsteroid"
	asteroid.position = ASTEROID_CENTER
	add_child(asteroid)
	var rock_mesh := SphereMesh.new()
	rock_mesh.radius = ASTEROID_RADIUS
	rock_mesh.height = ASTEROID_RADIUS * 2.0
	rock_mesh.radial_segments = 16
	rock_mesh.rings = 8
	var rock := _add_mesh(asteroid, rock_mesh, Vector3.ZERO, Vector3(1.1, 0.74, 0.94), _material(Color(0.43, 0.51, 0.59), 0.10))
	rock.rotation_degrees = Vector3(12, 19, 7)
	var vein := _material(Color(0.22, 0.68, 0.83), 0.5, 0.28)
	for i in range(7):
		var fragment := _add_mesh(asteroid, SphereMesh.new(), Vector3(sin(float(i)*2.1)*1.3, cos(float(i)*3.3)*0.8, 1.4*sin(float(i)*1.6)), Vector3(0.22, 0.15, 0.37), vein)
		fragment.rotation_degrees = Vector3(i*22, i*11, 0)

	# Distant planet provides depth without complex volumetrics or high GPU cost.
	var planet := Node3D.new()
	planet.position = Vector3(19, 9, -57)
	add_child(planet)
	var planet_mesh := SphereMesh.new()
	planet_mesh.radius = 13.0
	planet_mesh.height = 26.0
	planet_mesh.radial_segments = 30
	planet_mesh.rings = 16
	_add_mesh(planet, planet_mesh, Vector3.ZERO, Vector3.ONE, _material(Color(0.20, 0.30, 0.51), 0.0, 0.9))
	# Deterministic distant stars. Decorative elements have no gameplay authority.
	var rng := RandomNumberGenerator.new()
	rng.seed = 4041526
	var star_mat := _material(Color(0.70, 0.81, 1.0))
	var star_mesh := SphereMesh.new()
	star_mesh.radius = 0.07
	star_mesh.height = 0.14
	for i in range(45):
		var pos := Vector3(rng.randf_range(-80, 80), rng.randf_range(-35, 50), rng.randf_range(-95, -38))
		_add_mesh(self, star_mesh, pos, Vector3.ONE, star_mat)

func _build_probe() -> void:
	probe = Node3D.new()
	probe.name = "Gen1PrimeProbe"
	probe.position = Vector3(0, 0, 8)
	add_child(probe)
	var alloy := _material(Color(0.56, 0.67, 0.74), 0.85, 0.35)
	var carbon := _material(Color(0.10, 0.16, 0.23), 0.35)
	var radiator := _material(Color(0.15, 0.38, 0.52), 0.75, 0.26)
	var copper := _material(Color(0.86, 0.52, 0.24), 0.6, 0.35)
	# Placeholder silhouette: tube, paired radiator wings, sensors and arm.
	_box(probe, Vector3.ZERO, Vector3(1.5, 1.1, 3.5), alloy)
	_box(probe, Vector3(0, 0, -1.7), Vector3(0.8, 0.8, 0.6), carbon)
	_box(probe, Vector3(-2.1, 0, 0), Vector3(2.5, 0.13, 1.9), radiator)
	_box(probe, Vector3(2.1, 0, 0), Vector3(2.5, 0.13, 1.9), radiator)
	_box(probe, Vector3(0, 0, 1.9), Vector3(1.05, 1.05, 0.5), copper)
	_box(probe, Vector3(0.75, -0.40, -0.7), Vector3(0.22, 0.26, 1.1), copper)
	_box(probe, Vector3(0.9, -0.55, -1.55), Vector3(0.17, 0.16, 0.85), alloy)
	_box(probe, TOOL_FORWARD_OFFSET, Vector3(0.35, 0.35, 0.3), copper)
	engine_light = OmniLight3D.new()
	engine_light.position = Vector3(0, 0, 2.4)
	engine_light.light_color = Color(0.18, 0.57, 1.0)
	engine_light.light_energy = 1.3
	engine_light.omni_range = 5.0
	probe.add_child(engine_light)

func _build_camera() -> void:
	camera = Camera3D.new()
	camera.current = true
	camera.fov = 65.0
	camera.position = probe.position + Vector3(5, 6, 13)
	add_child(camera)
	camera.look_at(probe.position + Vector3(0, 0, -3), Vector3.UP)

func _build_hud() -> void:
	hud = CanvasLayer.new()
	add_child(hud)
	var heading := Label.new()
	heading.position = Vector2(25, 20)
	heading.add_theme_font_size_override("font_size", 23)
	heading.add_theme_color_override("font_color", Color(0.64, 0.88, 1.0))
	heading.text = "EVERWARD   /   PRIME-001   •   ENGINE BAKEOFF"
	hud.add_child(heading)

	readout = Label.new()
	readout.position = Vector2(27, 67)
	readout.add_theme_font_size_override("font_size", 18)
	readout.add_theme_color_override("font_color", Color(0.85, 0.92, 0.98))
	hud.add_child(readout)

	message = Label.new()
	message.position = Vector2(27, 250)
	message.add_theme_font_size_override("font_size", 16)
	message.add_theme_color_override("font_color", Color(0.97, 0.77, 0.43))
	hud.add_child(message)

	tip = Label.new()
	tip.position = Vector2(27, 648)
	tip.add_theme_font_size_override("font_size", 14)
	tip.add_theme_color_override("font_color", Color(0.75, 0.85, 0.98))
	tip.text = "WASD/QE MOVE  •  SPACE STOP  •  T SCAN  •  Y APPROACH  •  F MINE  •  R FIX_IT  •  F5 SAVE  •  F6 LOAD  •  H HUD"
	hud.add_child(tip)

func _surface_distance_m() -> float:
	return maxf(0.0, (probe.position + TOOL_FORWARD_OFFSET).distance_to(ASTEROID_CENTER) - ASTEROID_RADIUS)

func _update_hud() -> void:
	if readout == null:
		return
	var repair_state := "DAMAGED" if propulsion_integrity < 0.8 else "RESTORED"
	readout.text = "ENGINE       %s  (%d%% integrity)\nENERGY       %.0f J\nCARGO        %.1f / %.1f kg regolith\nASTEROID     %s  |  %.1f kg remaining\nTOOL RANGE   %.2f m from surface  [<=1.15 m required]\nFIX_IT       %d repairs completed" % [repair_state, int(round(propulsion_integrity*100.0)), energy_j, material_kg, MAX_STORAGE_KG, "SCANNED" if scanned else "UNKNOWN", ore_remaining_kg, _surface_distance_m(), repairs_completed]
	message.text = event_text

func _process(delta: float) -> void:
	if run_selftest:
		return
	if run_benchmark:
		_run_benchmark_frame(delta)
		return
	var direction := Vector3.ZERO
	if Input.is_key_pressed(KEY_W): direction.z -= 1.0
	if Input.is_key_pressed(KEY_S): direction.z += 1.0
	if Input.is_key_pressed(KEY_A): direction.x -= 1.0
	if Input.is_key_pressed(KEY_D): direction.x += 1.0
	if Input.is_key_pressed(KEY_Q): direction.y -= 1.0
	if Input.is_key_pressed(KEY_E): direction.y += 1.0
	if Input.is_key_pressed(KEY_SPACE): velocity = Vector3.ZERO
	if direction.length_squared() > 0.0:
		auto_approach = false
		var speed := 3.2 + propulsion_integrity * 5.5
		velocity = velocity.move_toward(direction.normalized() * speed, delta * 17.0)
	elif not auto_approach:
		velocity = velocity.move_toward(Vector3.ZERO, delta * 7.0)

	if auto_approach:
		var stand_off := ASTEROID_CENTER + Vector3(0.0, 0.0, ASTEROID_RADIUS + 3.0)
		var to_goal := stand_off - probe.position
		if to_goal.length() < 0.05:
			auto_approach = false
			velocity = Vector3.ZERO
			event_text = "TARGET STAND-OFF REACHED. PRESS F TO MINE."
		else:
			velocity = to_goal.normalized() * minf(to_goal.length()/maxf(delta, 0.001), 3.0)
	probe.position += velocity * delta
	camera.position = camera.position.lerp(probe.position + Vector3(5, 6, 13), minf(1.0, delta * 4.0))
	camera.look_at(probe.position + Vector3(0, 0, -3), Vector3.UP)
	engine_light.light_energy = 1.0 if propulsion_integrity < 0.8 else 2.7
	_update_hud()

func _run_benchmark_frame(delta: float) -> void:
	benchmark_frames += 1
	if benchmark_frames == 20:
		_scan()
	if benchmark_frames == 100:
		probe.position = Vector3(0, 0, -4.6)
		_mine()
		_mine()
		_mine()
		_repair()
	if benchmark_frames > 60:
		benchmark_deltas_ms.append(delta * 1000.0)
	camera.position = probe.position + Vector3(5.0 + sin(float(benchmark_frames) * 0.006) * 2.0, 6.0, 13.0)
	camera.look_at(probe.position + Vector3(0, 0, -3), Vector3.UP)
	_update_hud()
	if benchmark_frames == 70 or benchmark_frames == 140:
		var filename := ProjectSettings.globalize_path("res://evidence/visual_%03d.png" % benchmark_frames)
		var image_result := get_viewport().get_texture().get_image().save_png(filename)
		print("BAKEOFF_SCREENSHOT:", filename, " RESULT:", image_result)
	if benchmark_frames >= 240:
		var sorted_times := benchmark_deltas_ms.duplicate()
		sorted_times.sort()
		var count := sorted_times.size()
		var total_ms := 0.0
		for ms in sorted_times:
			total_ms += ms
		var fps_estimate := float(count) * 1000.0 / maxf(1.0, total_ms)
		var p95_frame_ms: float = float(sorted_times[int(float(count - 1) * 0.95)])
		var p99_frame_ms: float = float(sorted_times[int(float(count - 1) * 0.99)])
		var result := {"engine":"Godot 4.7.2","status":"RENDER_SMOKE_NOT_COMPARABLE","frames":count,"warmup_frames":60,"mean_frame_based_fps":fps_estimate,"p95_frame_ms":p95_frame_ms,"p99_frame_ms":p99_frame_ms,"screenshot_before":"visual_070.png","screenshot_after":"visual_140.png","source":"provisional GDScript, NOT authoritative C++"}
		var file := FileAccess.open("res://evidence/godot_render_smoke.json", FileAccess.WRITE)
		if file != null:
			file.store_string(JSON.stringify(result, "\t"))
			file.close()
		print("BAKEOFF_RENDER_SMOKE:", JSON.stringify(result))
		get_tree().quit(0)

func _unhandled_key_input(event: InputEvent) -> void:
	if not event is InputEventKey or not event.pressed or event.echo:
		return
	match event.keycode:
		KEY_T: _scan()
		KEY_Y:
			if scanned:
				auto_approach = true
				event_text = "AUTO-APPROACH ENGAGED. MANUAL MOVEMENT TAKES CONTROL."
			else:
				event_text = "SCAN [T] BEFORE TARGETED APPROACH."
		KEY_F: _mine()
		KEY_R: _repair()
		KEY_F5: _save_game()
		KEY_F6: _load_game()
		KEY_H: hud.visible = not hud.visible
	_update_hud()

func _scan() -> void:
	if energy_j < 15.0:
		event_text = "SCAN FAILED: NOT ENOUGH POWER."
		return
	energy_j -= 15.0
	scanned = true
	event_text = "SCAN: regolith resource located. USE Y TO AUTO-APPROACH."

func _mine() -> void:
	if not scanned:
		event_text = "MINE FAILED: UNKNOWN TARGET. SCAN FIRST."
		return
	if _surface_distance_m() > 1.15:
		event_text = "MINE FAILED: TOOL IS %.1f METERS FROM SURFACE." % _surface_distance_m()
		return
	if ore_remaining_kg < MINE_KG or material_kg + MINE_KG > MAX_STORAGE_KG or energy_j < 30.0:
		event_text = "MINE BLOCKED: ORE, STORAGE OR POWER CONSTRAINT."
		return
	ore_remaining_kg -= MINE_KG
	material_kg += MINE_KG
	energy_j -= 30.0
	event_text = "EXTRACTED %.1f KG — STORAGE %.1f KG. REPAIR REQUIRES 6 KG." % [MINE_KG, material_kg]

func _repair() -> void:
	if propulsion_integrity >= 0.8:
		event_text = "FIX_IT: PROPULSION ALREADY RESTORED."
		return
	if material_kg < REPAIR_COST_KG or energy_j < 120.0:
		event_text = "FIX_IT: NEED 6 KG REGOLITH + 120 J ENERGY."
		return
	material_kg -= REPAIR_COST_KG
	energy_j -= 120.0
	propulsion_integrity = 0.86
	repairs_completed += 1
	event_text = "FIX_IT COMPLETE! PROPULSION RESTORED: 28% -> 86%."

func _save_game() -> void:
	var data := {"position":[probe.position.x,probe.position.y,probe.position.z],"energy_j":energy_j,"material_kg":material_kg,"ore_remaining_kg":ore_remaining_kg,"integrity":propulsion_integrity,"scanned":scanned,"repairs":repairs_completed}
	var file := FileAccess.open("user://bakeoff_save.json", FileAccess.WRITE)
	if file == null:
		event_text = "SAVE FAILED: FILE ACCESS ERROR."
		return
	file.store_string(JSON.stringify(data))
	file.close()
	event_text = "SAVED ENGINE-BAKEOFF PROGRESS."

func _load_game() -> void:
	if not FileAccess.file_exists("user://bakeoff_save.json"):
		event_text = "LOAD FAILED: NO SAVE FOUND."
		return
	var file := FileAccess.open("user://bakeoff_save.json", FileAccess.READ)
	var data = JSON.parse_string(file.get_as_text())
	if not data is Dictionary or not data.has("position"):
		event_text = "LOAD FAILED: INVALID SAVE."
		return
	probe.position = Vector3(float(data["position"][0]),float(data["position"][1]),float(data["position"][2]))
	energy_j = float(data["energy_j"])
	material_kg = float(data["material_kg"])
	ore_remaining_kg = float(data["ore_remaining_kg"])
	propulsion_integrity = float(data["integrity"])
	scanned = bool(data["scanned"])
	repairs_completed = int(data["repairs"])
	auto_approach = false
	velocity = Vector3.ZERO
	event_text = "SAVED STATE RESTORED."

func _check(condition: bool, description: String) -> void:
	if condition:
		print("BAKEOFF_TEST_PASS: ", description)
	else:
		selftest_errors += 1
		printerr("BAKEOFF_TEST_FAIL: ", description)

func _run_self_test() -> void:
	_check(not scanned and material_kg == 0.0 and propulsion_integrity < 0.3, "deterministic damaged awakening")
	_mine()
	_check(material_kg == 0.0, "mining requires a scan")
	_scan()
	_check(scanned and energy_j == 935.0, "scan consumes real energy")
	_mine()
	_check(material_kg == 0.0, "out-of-reach mining rejected")
	probe.position = Vector3(0.0, 0.0, -4.6)
	_check(_surface_distance_m() <= 1.15, "tool-tip collision range geometry")
	_mine()
	_mine()
	_mine()
	_check(material_kg == 6.0 and ore_remaining_kg == 18.0, "mining transfers physical mass")
	_repair()
	_check(material_kg == 0.0 and propulsion_integrity == 0.86 and repairs_completed == 1, "Fix_It consumes cargo and energy and restores integrity")
	_save_game()
	energy_j = 0.0
	propulsion_integrity = 0.01
	_load_game()
	_check(energy_j > 0.0 and propulsion_integrity == 0.86, "save/load roundtrip")
	print("BAKEOFF_SELFTEST_STATUS=", "PASS" if selftest_errors == 0 else "FAIL")
	print("BAKEOFF_SELFTEST_FAILURE_COUNT=", selftest_errors)
	get_tree().quit(0 if selftest_errors == 0 else 2)
