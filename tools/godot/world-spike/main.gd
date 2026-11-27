extends Node3D
# Everward Godot feasibility spike. Not game code: it answers "can Godot do this?" with a working scene.
#   * smooth voxel mountain (Voxel Tools) that can be drilled, hollowed and made to crumble
#   * raymarched cloud layer that parts around the ship's trail
#   * trees that sway, and shake/lean away when the ship flies near
# Run interactively:  godot --path project            (W/S thrust, mouse look, A/D/Q/E, LMB or F drill, H hollow, P shot)
# Scripted test:      godot --path project -- --autotest [--out=FOLDER]   (flies a route, writes screenshots + metrics.txt)

var OUT_DIR := ProjectSettings.globalize_path("user://spike_out")   # override with:  -- --autotest --out=D:/some/folder
const TRAIL_POINTS := 16
const TRAIL_LIFE := 9.0
const MAX_DEBRIS := 140

const Looks = preload("res://looks.gd")
const TreeBuilder = preload("res://tree_builder.gd")

var autotest := false
var lookshots := false
var look: Dictionary = {}
var look_name := ""
var env_ref: Environment
var sky_ref: ProceduralSkyMaterial
var sun_ref: DirectionalLight3D
var terrain_mat: ShaderMaterial
var explore := false
var selftest_input := false
var selftest_fail := ""
var terrain: VoxelTerrain
var ship: Node3D
var cam: Camera3D
var viewer: VoxelViewer
var cloud_mat: ShaderMaterial
var tree_mats: Array[ShaderMaterial] = []
var trail: Array[Vector4] = []          # xyz position, w birth time
var trail_clock := 0.0
var yaw := 0.0
var pitch := 0.0
var speed := 0.0
var debris: Array = []                  # [RigidBody3D, death_time]
var rock_mat: StandardMaterial3D
var dust_mat: StandardMaterial3D
var metrics: PackedStringArray = []
var edit_ms: PackedFloat32Array = []
var cavity_center := Vector3.ZERO
var shake := 0.0
var hud: Label
var shot_index := 0
var tree_spots: Array[Vector3] = []
# G2: when the Everward simulation extension is loaded and --simflight/--sim is given, the simulation moves the
# ship and owns contact. Simulation frame: x forward, y right, z up (Unreal convention); Godot: -z forward, x right,
# y up. A simulated point s maps to sim_origin + (s.y, s.z, -s.x); yaw is mirrored.
var sim = null
var sim_active := false
var sim_origin := Vector3.ZERO
var sim_contacts := 0
var simflight := false

func _ready() -> void:
	var args := OS.get_cmdline_user_args()
	autotest = "--autotest" in args
	explore = "--explore" in args
	lookshots = "--lookshots" in args
	simflight = "--simflight" in args
	for la in args:
		if la.begins_with("--look="):
			look_name = la.substr(7)
			if Looks.LOOKS.has(look_name):
				look = Looks.LOOKS[look_name]
			else:
				push_error("unknown look " + look_name + "; known: " + ", ".join(Looks.names()))
	selftest_input = "--selftest-input" in args
	for sa in args:
		if sa.begins_with("--selftest-fail="):
			selftest_fail = sa.substr(16)
	for a in args:
		if a.begins_with("--out="):
			OUT_DIR = a.substr(6)
	DirAccess.make_dir_recursive_absolute(OUT_DIR)
	_build_environment()
	_build_terrain()
	_build_ship()
	_build_clouds()
	_build_hud()
	_apply_look()
	rock_mat = StandardMaterial3D.new()
	rock_mat.albedo_color = Color(0.55, 0.5, 0.44)
	rock_mat.roughness = 0.95
	rock_mat.emission_enabled = true
	rock_mat.emission = Color(0.12, 0.11, 0.1)
	dust_mat = StandardMaterial3D.new()
	dust_mat.shading_mode = BaseMaterial3D.SHADING_MODE_UNSHADED
	dust_mat.transparency = BaseMaterial3D.TRANSPARENCY_ALPHA
	dust_mat.albedo_color = Color(0.66, 0.6, 0.53, 0.8)
	dust_mat.vertex_color_use_as_albedo = true
	dust_mat.billboard_mode = BaseMaterial3D.BILLBOARD_PARTICLES
	var puff := GradientTexture2D.new()
	var pg := Gradient.new()
	pg.set_color(0, Color(1, 1, 1, 1))
	pg.set_color(1, Color(1, 1, 1, 0))
	puff.gradient = pg
	puff.fill = GradientTexture2D.FILL_RADIAL
	puff.fill_from = Vector2(0.5, 0.5)
	puff.fill_to = Vector2(1.0, 0.5)
	puff.width = 64
	puff.height = 64
	dust_mat.albedo_texture = puff
	if simflight:
		_run_simflight.call_deferred()
	elif lookshots:
		seed(20261127)   # same trees and framings for every look, so they compare like for like
		_run_lookshots.call_deferred()
	elif autotest or explore:
		_run_autotest.call_deferred()
	else:
		Input.mouse_mode = Input.MOUSE_MODE_CAPTURED
		_start_interactive.call_deferred()
		if selftest_input:
			_selftest_input.call_deferred()

# ----------------------------------------------------------------------------------------- build
func _build_environment() -> void:
	var env := Environment.new()
	env.background_mode = Environment.BG_SKY
	var sky := Sky.new()
	var psm := ProceduralSkyMaterial.new()
	psm.sky_top_color = Color(0.16, 0.36, 0.7)
	psm.sky_horizon_color = Color(0.62, 0.74, 0.88)
	psm.ground_horizon_color = Color(0.62, 0.74, 0.88)
	psm.ground_bottom_color = Color(0.25, 0.3, 0.38)
	psm.sun_angle_max = 30.0
	sky.sky_material = psm
	env.sky = sky
	env.ambient_light_source = Environment.AMBIENT_SOURCE_SKY
	env.ambient_light_energy = 0.9
	env.tonemap_mode = Environment.TONE_MAPPER_ACES
	env.glow_enabled = true
	env.glow_intensity = 0.6
	env.fog_enabled = true
	env.fog_light_color = Color(0.66, 0.76, 0.9)
	env.fog_density = 0.0009
	env.fog_aerial_perspective = 0.5
	env_ref = env
	sky_ref = psm
	var we := WorldEnvironment.new()
	we.environment = env
	add_child(we)
	var sun := DirectionalLight3D.new()
	sun.rotation_degrees = Vector3(-38.0, -40.0, 0.0)
	sun.light_energy = 1.6
	sun.shadow_enabled = true
	sun.directional_shadow_mode = DirectionalLight3D.SHADOW_PARALLEL_2_SPLITS
	sun.directional_shadow_max_distance = 500.0
	sun_ref = sun
	add_child(sun)

func _build_terrain() -> void:
	terrain = VoxelTerrain.new()
	var gen := VoxelGeneratorNoise2D.new()
	var noise := FastNoiseLite.new()
	noise.noise_type = FastNoiseLite.TYPE_SIMPLEX_SMOOTH
	noise.fractal_type = FastNoiseLite.FRACTAL_RIDGED
	noise.fractal_octaves = 6
	noise.fractal_gain = 0.45
	noise.frequency = 0.0028
	noise.seed = 1234
	gen.noise = noise
	gen.channel = VoxelBuffer.CHANNEL_SDF
	gen.height_start = -10.0
	gen.height_range = 215.0
	terrain.generator = gen
	terrain.mesher = VoxelMesherTransvoxel.new()
	terrain.max_view_distance = 384
	terrain.bounds = AABB(Vector3(-640, -60, -640), Vector3(1280, 520, 1280))
	var mat := ShaderMaterial.new()
	mat.shader = load("res://terrain.gdshader")
	terrain.material_override = mat
	terrain_mat = mat
	add_child(terrain)

func _build_ship() -> void:
	ship = Node3D.new()
	ship.name = "Ship"
	ship.position = Vector3(0, 260, 380)
	add_child(ship)
	var body := MeshInstance3D.new()
	var cap := CapsuleMesh.new()
	cap.radius = 0.9
	cap.height = 4.6
	body.mesh = cap
	body.rotation_degrees = Vector3(90, 0, 0)
	var hull := StandardMaterial3D.new()
	hull.albedo_color = Color(0.7, 0.72, 0.78)
	hull.metallic = 0.8
	hull.roughness = 0.35
	body.material_override = hull
	ship.add_child(body)
	var wing := MeshInstance3D.new()
	var wb := BoxMesh.new()
	wb.size = Vector3(6.4, 0.16, 1.7)
	wing.mesh = wb
	wing.position = Vector3(0, 0, 0.6)
	wing.material_override = hull
	ship.add_child(wing)
	var glow := OmniLight3D.new()
	glow.light_color = Color(0.5, 0.75, 1.0)
	glow.light_energy = 2.0
	glow.omni_range = 14.0
	glow.position = Vector3(0, 0, 2.8)
	ship.add_child(glow)
	cam = Camera3D.new()
	cam.position = Vector3(0, 2.6, 9.5)
	cam.rotation_degrees = Vector3(-8, 0, 0)
	cam.far = 3000.0
	cam.fov = 70.0
	ship.add_child(cam)
	viewer = VoxelViewer.new()
	viewer.view_distance = 384
	viewer.requires_collisions = true
	ship.add_child(viewer)

func _build_clouds() -> void:
	var nt := NoiseTexture3D.new()
	nt.width = 128
	nt.height = 128
	nt.depth = 128
	nt.seamless = true
	var cn := FastNoiseLite.new()
	cn.noise_type = FastNoiseLite.TYPE_SIMPLEX_SMOOTH
	cn.fractal_type = FastNoiseLite.FRACTAL_FBM
	cn.fractal_octaves = 5
	cn.frequency = 0.035
	cn.seed = 77
	nt.noise = cn
	cloud_mat = ShaderMaterial.new()
	cloud_mat.shader = load("res://cloud.gdshader")
	cloud_mat.set_shader_parameter("noise_tex", nt)
	var q := MeshInstance3D.new()
	var quad := QuadMesh.new()
	quad.size = Vector2(2, 2)
	q.mesh = quad
	q.material_override = cloud_mat
	q.extra_cull_margin = 16384.0
	q.cast_shadow = GeometryInstance3D.SHADOW_CASTING_SETTING_OFF
	cam.add_child(q)
	_push_trail_uniform()

func _scripted() -> bool:
	# Any scripted run: no manual flight, no player input (they would move the camera between captures)
	return autotest or explore or lookshots or simflight

func _apply_look() -> void:
	# Design tokens -> scene: sky, fog, sun, clouds, terrain. Trees get theirs when they are created.
	var sd := sun_ref.global_transform.basis.z
	cloud_mat.set_shader_parameter("sun_dir", sd)
	if look.is_empty():
		return
	sky_ref.sky_top_color = look["sky_top"]
	sky_ref.sky_horizon_color = look["sky_horizon"]
	sky_ref.ground_horizon_color = look["sky_horizon"]
	sky_ref.ground_bottom_color = look["ground"]
	env_ref.ambient_light_energy = look["ambient_energy"]
	env_ref.fog_light_color = look["fog_color"]
	env_ref.fog_density = look["fog_density"]
	sun_ref.rotation_degrees = Vector3(look["sun_pitch"], look["sun_yaw"], 0.0)
	sun_ref.light_color = look["sun_color"]
	sun_ref.light_energy = look["sun_energy"]
	cloud_mat.set_shader_parameter("sun_dir", sun_ref.transform.basis.z)
	cloud_mat.set_shader_parameter("sun_color", look["cloud_lit"])
	cloud_mat.set_shader_parameter("ambient_color", look["cloud_shadow"])
	cloud_mat.set_shader_parameter("coverage", look["coverage"])
	for key in ["grass_a", "grass_b", "rock_a", "rock_b"]:
		terrain_mat.set_shader_parameter(key, look[key])
	terrain_mat.set_shader_parameter("snow_color", look["snow"])

func _apply_tree_look(mat: ShaderMaterial) -> void:
	if look.is_empty():
		return
	mat.set_shader_parameter("base_color", look["foliage_base"])
	mat.set_shader_parameter("tip_color", look["foliage_tip"])
	mat.set_shader_parameter("trunk_color", look["trunk"])
	mat.set_shader_parameter("autumn_color", look["autumn"])
	mat.set_shader_parameter("autumn_mix", look["autumn_mix"])

func _run_lookshots() -> void:
	# Same three framings for any --look, so presets can be compared side by side.
	var tag := look_name if look_name != "" else "default"
	_place_ship(Vector3(0, 300, 0), Vector3(0, 0, -100))
	await _wait_stream()
	var tool: VoxelTool = terrain.get_voxel_tool()
	var peak := Vector3(0, -1000, 0)
	for gx in range(-9, 10):
		for gz in range(-9, 10):
			var h = tool.raycast(Vector3(gx * 40.0, 480.0, gz * 40.0), Vector3.DOWN, 700.0)
			if h != null and h.position.y > peak.y:
				peak = Vector3(h.position)
	if _place_trees(700, 360.0, Vector3(peak.x, 0, peak.z)) == 0:
		_fail("no trees could be placed", 4)
		return
	# hide only the ship's meshes: the camera, cloud layer and terrain viewer are children of the ship
	for child in ship.get_children():
		if child is MeshInstance3D or child is Light3D:
			child.visible = false
	_place_ship(peak + Vector3(0, 130, 420), peak + Vector3(0, -40, 0))
	await _wait_stream()   # the ship is the streaming viewer; let the new area mesh before the shot
	await _shot("look_%s_A_overview" % tag)
	var spot := tree_spots[0]
	for sp in tree_spots:
		if sp.y > 20.0 and sp.y < 80.0:
			spot = sp
			break
	# forest view: stand the camera on the measured ground near the chosen tree, looking across the trees
	var cam_xz := spot + Vector3(45, 0, 70)
	var g = tool.raycast(Vector3(cam_xz.x, 480.0, cam_xz.z), Vector3.DOWN, 700.0)
	var ground_y: float = spot.y if g == null else float(g.position.y)
	_place_ship(Vector3(cam_xz.x, maxf(ground_y, spot.y) + 14.0, cam_xz.z), spot + Vector3(0, 5, 0))
	await _wait_stream()   # the ship is the streaming viewer; let the new area mesh before the shot
	# now that the area is loaded, make sure the camera is above the real ground, not inside a slope
	for i in 6:
		var under = tool.raycast(ship.global_position + Vector3(0, 300, 0), Vector3.DOWN, 400.0)
		if under == null or float(under.position.y) < ship.global_position.y - 10.0:
			break
		ship.global_position.y = float(under.position.y) + 14.0
		ship.look_at(spot + Vector3(0, 5, 0), Vector3.UP)
		await _wait_stream()
	await _shot("look_%s_B_forest" % tag)
	# close-up: from whichever of eight directions the ground is open (no higher than the tree's base),
	# stand 20 m out and 10 m up and look at mid-trunk, so the tree is seen against open ground or sky
	var best_dir := Vector3(1, 0, 0)
	var best_ground := 1e9
	for k in 8:
		var dir8 := Vector3(cos(k * TAU / 8.0), 0, sin(k * TAU / 8.0))
		var gp = tool.raycast(Vector3(spot.x, 480.0, spot.z) + dir8 * 20.0, Vector3.DOWN, 700.0)
		var gy8: float = 1e9 if gp == null else float(gp.position.y)
		if gy8 < best_ground:
			best_ground = gy8
			best_dir = dir8
	_place_ship(spot + best_dir * 20.0 + Vector3(0, maxf(10.0, best_ground - spot.y + 4.0), 0), spot + Vector3(0, 7, 0))
	await _wait_stream()
	await _shot("look_%s_D_trees_close" % tag)
	_place_ship(peak + Vector3(-90, -40, 260), peak + Vector3(0, 20, 0))
	await _wait_stream()   # the ship is the streaming viewer; let the new area mesh before the shot
	await _shot("look_%s_C_mountain" % tag)
	_flush_metrics()
	_quit_clean(0)

# ------------------------------------------------------------------------------- G2: simulation-driven flight
func _sim_start(origin: Vector3) -> bool:
	if not ClassDB.class_exists("EverwardSim"):
		return false
	sim = ClassDB.instantiate("EverwardSim")
	sim_origin = origin
	sim_active = true
	_sim_place_ship()
	return true

func _sim_to_godot(p: PackedFloat64Array) -> Vector3:
	return sim_origin + Vector3(p[1], p[2], -p[0])

func _godot_to_sim(g: Vector3) -> Vector3:
	var d := g - sim_origin
	return Vector3(-d.z, d.x, d.y)

func _sim_basis() -> Basis:
	var a: PackedFloat64Array = sim.get_attitude()
	return Basis(Vector3.UP, deg_to_rad(-a[0])) * Basis(Vector3.RIGHT, deg_to_rad(a[1])) * Basis(Vector3.FORWARD, deg_to_rad(a[2]))

func _sim_place_ship() -> void:
	ship.global_transform = Transform3D(_sim_basis(), _sim_to_godot(sim.get_position()))

func _sim_add_rock(id: String, godot_center: Vector3, radius: float) -> bool:
	var s := _godot_to_sim(godot_center)
	if not sim.add_sphere_body(id, s.x, s.y, s.z, radius):
		return false
	var mi := MeshInstance3D.new()
	var sm := SphereMesh.new()
	sm.radius = radius
	sm.height = radius * 2.0
	mi.mesh = sm
	mi.material_override = rock_mat
	add_child(mi)
	mi.global_position = godot_center
	return true

func _sim_step(delta: float) -> void:
	sim.advance_ticks(int(round(delta * 1000000.0)))
	_sim_place_ship()
	var n: int = sim.drain_contact_events()
	if n > 0:
		sim_contacts += n
		var c: Dictionary = sim.get_last_contact()
		_dust(_sim_to_godot(c["point"]), 30)
		shake = maxf(shake, 0.5)

func _sim_run(seconds: float) -> void:
	var t := 0.0
	while t < seconds:
		await get_tree().process_frame
		t += get_process_delta_time()

func _run_simflight() -> void:
	# G2 evidence: the simulation, not scripted movement or engine physics, moves the ship and reports contact.
	metrics.append("godot %s  renderer=%s" % [Engine.get_version_info()["string"], RenderingServer.get_video_adapter_name()])
	_place_ship(Vector3(0, 300, 0), Vector3(0, 0, -100))
	await _wait_stream()
	var tool: VoxelTool = terrain.get_voxel_tool()
	var peak := Vector3(0, -1000, 0)
	for gx in range(-9, 10):
		for gz in range(-9, 10):
			var h = tool.raycast(Vector3(gx * 40.0, 480.0, gz * 40.0), Vector3.DOWN, 700.0)
			if h != null and h.position.y > peak.y:
				peak = Vector3(h.position)
	_place_trees(700, 360.0, Vector3(peak.x, 0, peak.z))
	var origin := peak + Vector3(0, 70, 320)
	if not _sim_start(origin):
		_fail("simulation extension (EverwardSim) not loaded", 5)
		return
	var ok := true
	ok = ok and _sim_add_rock("boulder-1", origin + Vector3(0, 0, -70), 6.0)
	ok = ok and _sim_add_rock("boulder-2", origin + Vector3(25, -8, -110), 9.0)
	await _sim_run(0.5)
	await _shot("sim_01_start")
	sim.adjust_local_velocity(14.0, 0.0, 0.0)
	await _sim_run(2.5)
	await _shot("sim_02_approach")
	var waited := 0.0
	while sim_contacts == 0 and waited < 10.0:
		await _sim_run(0.25)
		waited += 0.25
	await _shot("sim_03_contact")
	var c: Dictionary = sim.get_last_contact()
	var pos_err: float = ship.global_position.distance_to(_sim_to_godot(sim.get_position()))
	metrics.append("sim contact events %d  body %s  normal_speed %.3f  ship-vs-sim position error %.6f m" % [sim_contacts, c["body_id"], c["normal_speed"], pos_err])
	# turn 90 degrees and fly on: the ship must move where its nose points
	sim.adjust_attitude(90.0, 0.0, 0.0)
	var v: PackedFloat64Array = sim.get_velocity()
	sim.adjust_local_velocity(10.0, 0.0, 0.0)
	var before := ship.global_position
	await _sim_run(2.0)
	await _shot("sim_04_turned")
	var moved := (ship.global_position - before).normalized()
	var nose := -ship.global_transform.basis.z
	var heading_dot := moved.dot(nose)
	metrics.append("after 90 deg turn: velocity before thrust %.3f %.3f %.3f; heading dot %.4f; contacts %d" % [v[0], v[1], v[2], heading_dot, sim_contacts])
	# roll check: after a +90 degree roll, thrust toward the probe's own right must move the ship along its displayed right
	sim.set_velocity(0.0, 0.0, 0.0)
	sim.adjust_attitude(0.0, 0.0, 90.0)
	sim.adjust_local_velocity(0.0, 8.0, 0.0)
	var before_roll := ship.global_position
	await _sim_run(1.5)
	var right_dot := (ship.global_position - before_roll).normalized().dot(ship.global_transform.basis.x)
	metrics.append("after +90 deg roll: thrust right moves along displayed right, dot %.4f" % right_dot)
	sim.set_velocity(0.0, 0.0, 0.0)
	sim.adjust_attitude(0.0, 0.0, -90.0)
	var passed: bool = ok and sim_contacts == 1 and c["body_id"] == "boulder-1" and pos_err < 0.001 and heading_dot > 0.99 and right_dot > 0.99
	metrics.append("SIMFLIGHT %s" % ("PASS" if passed else "FAIL"))
	_flush_metrics()
	print("SIMFLIGHT ", "PASS" if passed else "FAIL")
	_quit_clean(0 if passed else 6)

func _build_hud() -> void:
	var layer := CanvasLayer.new()
	hud = Label.new()
	hud.position = Vector2(12, 8)
	hud.add_theme_color_override("font_color", Color(1, 1, 1))
	hud.add_theme_color_override("font_outline_color", Color(0, 0, 0))
	hud.add_theme_constant_override("outline_size", 4)
	layer.add_child(hud)
	add_child(layer)

# --------------------------------------------------------------------------------------- per frame
func _process(delta: float) -> void:
	trail_clock += delta
	if sim_active:
		_sim_step(delta)
	if trail.is_empty() or trail_clock - trail[0].w > 0.22:
		trail.push_front(Vector4(ship.global_position.x, ship.global_position.y, ship.global_position.z, trail_clock))
		if trail.size() > TRAIL_POINTS:
			trail.pop_back()
	_push_trail_uniform()
	for m in tree_mats:
		m.set_shader_parameter("ship_pos", ship.global_position)
	_update_debris()
	if shake > 0.0:
		cam.h_offset = randf_range(-1.0, 1.0) * shake
		cam.v_offset = randf_range(-1.0, 1.0) * shake
		shake = maxf(0.0, shake - delta * 1.2)
	else:
		cam.h_offset = 0.0
		cam.v_offset = 0.0
	if not _scripted():
		_manual_flight(delta)
	hud.text = "FPS %d   alt %d m   speed %d m/s   debris %d" % [Engine.get_frames_per_second(), int(ship.global_position.y), int(speed), debris.size()]

func _push_trail_uniform() -> void:
	var arr := PackedVector4Array()
	arr.resize(TRAIL_POINTS)
	for i in TRAIL_POINTS:
		if i < trail.size():
			var age := trail_clock - trail[i].w
			var k := clampf(1.0 - age / TRAIL_LIFE, 0.0, 1.0)
			var r := 17.0 * sqrt(k)
			arr[i] = Vector4(trail[i].x, trail[i].y, trail[i].z, r if k > 0.0 else 0.0)
		else:
			arr[i] = Vector4(0, 0, 0, 0)
	cloud_mat.set_shader_parameter("trail", arr)
	var lo := Vector3(1e9, 1e9, 1e9)
	var hi := Vector3(-1e9, -1e9, -1e9)
	var any := false
	for v in arr:
		if v.w > 0.0:
			any = true
			lo = lo.min(Vector3(v.x, v.y, v.z))
			hi = hi.max(Vector3(v.x, v.y, v.z))
	if any:
		var c := (lo + hi) * 0.5
		cloud_mat.set_shader_parameter("trail_bound", Vector4(c.x, c.y, c.z, (hi - lo).length() * 0.5 + 17.0 * 2.3))
	else:
		cloud_mat.set_shader_parameter("trail_bound", Vector4(0, -99999.0, 0, 0))

func _manual_flight(delta: float) -> void:
	var thrust := 0.0
	if Input.is_key_pressed(KEY_W): thrust += 1.0
	if Input.is_key_pressed(KEY_S): thrust -= 0.6
	var boost := 2.6 if Input.is_key_pressed(KEY_SHIFT) else 1.0
	speed = lerpf(speed, thrust * 48.0 * boost, clampf(delta * 1.6, 0.0, 1.0))
	ship.rotation = Vector3(pitch, yaw, 0.0)
	var fwd := -ship.global_transform.basis.z
	var right := ship.global_transform.basis.x
	var vel := fwd * speed
	if Input.is_key_pressed(KEY_D): vel += right * 26.0
	if Input.is_key_pressed(KEY_A): vel -= right * 26.0
	if Input.is_key_pressed(KEY_E): vel += Vector3.UP * 26.0
	if Input.is_key_pressed(KEY_Q): vel -= Vector3.UP * 26.0
	var next := ship.global_position + vel * delta
	var t: VoxelTool = terrain.get_voxel_tool()
	var hit = t.raycast(ship.global_position, vel.normalized() if vel.length() > 0.1 else fwd, maxf(vel.length() * delta, 0.0) + 3.5)
	if hit == null:
		ship.global_position = next
	if Input.is_mouse_button_pressed(MOUSE_BUTTON_LEFT) or Input.is_key_pressed(KEY_F):
		_drill_step()

func _input(event: InputEvent) -> void:
	if _scripted():
		return
	if event is InputEventMouseMotion and Input.mouse_mode == Input.MOUSE_MODE_CAPTURED:
		yaw -= event.relative.x * 0.0025
		pitch = clampf(pitch - event.relative.y * 0.0025, -1.4, 1.4)
	elif event is InputEventMouseButton and event.pressed and Input.mouse_mode == Input.MOUSE_MODE_VISIBLE:
		Input.mouse_mode = Input.MOUSE_MODE_CAPTURED   # click the window to resume flying
	elif event is InputEventKey and event.pressed:
		if event.keycode == KEY_ESCAPE:
			Input.mouse_mode = Input.MOUSE_MODE_VISIBLE if Input.mouse_mode == Input.MOUSE_MODE_CAPTURED else Input.MOUSE_MODE_CAPTURED
		elif event.keycode == KEY_H:
			var hit = terrain.get_voxel_tool().raycast(ship.global_position, -ship.global_transform.basis.z, 120.0)
			var c: Vector3 = Vector3(hit.position) - ship.global_transform.basis.z * 18.0 if hit != null else ship.global_position - ship.global_transform.basis.z * 30.0
			hollow(c, 17.0)
		elif event.keycode == KEY_P:
			_save_shot("manual_%03d" % shot_index)
			shot_index += 1

# ----------------------------------------------------------------------------------- terrain edits
func carve(center: Vector3, radius: float) -> float:
	var t0 := Time.get_ticks_usec()
	var tool: VoxelTool = terrain.get_voxel_tool()
	tool.mode = VoxelTool.MODE_REMOVE
	tool.do_sphere(center, radius)
	var ms := (Time.get_ticks_usec() - t0) / 1000.0
	edit_ms.append(ms)
	return ms

func _drill_step() -> void:
	var dir := -ship.global_transform.basis.z
	var hit = terrain.get_voxel_tool().raycast(ship.global_position, dir, 22.0)
	if hit != null:
		var p: Vector3 = Vector3(hit.position) + dir * 2.0
		carve(p, 4.6)
		_spawn_debris(Vector3(hit.position), 2, -dir)
		_dust(Vector3(hit.position), 14)
		shake = maxf(shake, 0.05)

func hollow(center: Vector3, radius: float) -> void:
	cavity_center = center
	carve(center, radius)
	shake = 0.6
	_dust(center, 80)
	_spawn_debris(center + Vector3(0, radius * 0.5, 0), 26, Vector3.DOWN)
	_crumble(center, radius)

func _crumble(center: Vector3, radius: float) -> void:
	# Progressive collapse: the cavity ceiling lets go in pieces over a few seconds. Each piece is a real
	# voxel removal (the roof gets higher) plus falling rigid-body rubble and a dust puff.
	for k in 12:
		await get_tree().create_timer(0.35 + randf() * 0.25).timeout
		var base := center + Vector3(randf_range(-0.8, 0.8) * radius, 0.0, randf_range(-0.8, 0.8) * radius)
		var hit = terrain.get_voxel_tool().raycast(base, Vector3.UP, radius * 3.0)
		if hit == null:
			continue
		var r := randf_range(2.6, 4.8)
		carve(Vector3(hit.position) + Vector3.UP * (r * 0.6), r)
		_spawn_debris(Vector3(hit.position), 8, Vector3.DOWN)
		_dust(Vector3(hit.position), 26)
		shake = maxf(shake, 0.25)

func _spawn_debris(at: Vector3, count: int, dir: Vector3) -> void:
	for i in count:
		if debris.size() >= MAX_DEBRIS:
			var old = debris.pop_front()
			if is_instance_valid(old[0]): old[0].queue_free()
		var rb := RigidBody3D.new()
		var r := randf_range(0.35, 1.25)
		var cs := CollisionShape3D.new()
		var sh := SphereShape3D.new()
		sh.radius = r
		cs.shape = sh
		rb.add_child(cs)
		var mi := MeshInstance3D.new()
		var sm := SphereMesh.new()
		sm.radius = r
		sm.height = r * 2.0
		sm.radial_segments = 8
		sm.rings = 4
		mi.mesh = sm
		mi.material_override = rock_mat
		mi.scale = Vector3(randf_range(0.8, 1.3), randf_range(0.7, 1.1), randf_range(0.8, 1.3))
		rb.add_child(mi)
		add_child(rb)
		rb.global_position = at + Vector3(randf_range(-2, 2), randf_range(-1, 2), randf_range(-2, 2))
		rb.linear_velocity = dir * randf_range(2.0, 9.0) + Vector3(randf_range(-4, 4), randf_range(0, 5), randf_range(-4, 4))
		rb.angular_velocity = Vector3(randf_range(-4, 4), randf_range(-4, 4), randf_range(-4, 4))
		debris.append([rb, trail_clock + randf_range(5.0, 10.0)])

func _update_debris() -> void:
	for i in range(debris.size() - 1, -1, -1):
		var d = debris[i]
		if not is_instance_valid(d[0]) or trail_clock > d[1] or d[0].global_position.y < -80.0:
			if is_instance_valid(d[0]): d[0].queue_free()
			debris.remove_at(i)

func _dust(at: Vector3, amount: int) -> void:
	var p := CPUParticles3D.new()
	p.one_shot = true
	p.explosiveness = 0.95
	p.amount = amount
	p.lifetime = 3.2
	p.emission_shape = CPUParticles3D.EMISSION_SHAPE_SPHERE
	p.emission_sphere_radius = 2.5
	p.direction = Vector3.UP
	p.spread = 180.0
	p.initial_velocity_min = 1.5
	p.initial_velocity_max = 7.0
	p.gravity = Vector3(0, 0.6, 0)
	p.damping_min = 1.5
	p.damping_max = 3.0
	p.scale_amount_min = 3.0
	p.scale_amount_max = 7.0
	var q := QuadMesh.new()
	q.size = Vector2(1, 1)
	p.mesh = q
	p.material_override = dust_mat
	var grad := Gradient.new()
	grad.set_color(0, Color(1, 1, 1, 0.7))
	grad.set_color(1, Color(1, 1, 1, 0.0))
	p.color_ramp = grad
	add_child(p)
	p.global_position = at
	p.emitting = true
	get_tree().create_timer(4.0).timeout.connect(p.queue_free)

# ------------------------------------------------------------------------------------------ trees
func _place_trees(count: int = 700, area: float = 420.0, center: Vector3 = Vector3.ZERO) -> int:
	var tool: VoxelTool = terrain.get_voxel_tool()
	tool.set_raycast_normal_enabled(true)
	var transforms: Array[Transform3D] = []
	var tries := 0
	while transforms.size() < count and tries < count * 6:
		tries += 1
		var x := center.x + randf_range(-area, area)
		var z := center.z + randf_range(-area, area)
		var hit = tool.raycast(Vector3(x, 420.0, z), Vector3.DOWN, 600.0)
		if hit == null:
			continue
		var p: Vector3 = Vector3(hit.position)
		if hit.normal.y < 0.86 or p.y > 125.0 or p.y < 0.0:
			continue
		var s := randf_range(0.75, 1.25)
		var b := Basis(Vector3.UP, randf() * TAU).scaled(Vector3(s, s, s))
		transforms.append(Transform3D(b, p + Vector3(0, -0.3, 0)))
		tree_spots.append(p)
	if transforms.is_empty():
		return 0
	# Two species, two variants each (tree_builder.gd). Broadleaf in the low valleys, conifers higher up.
	var builder := TreeBuilder.new(4242)
	var variants := [builder.build_conifer(), builder.build_conifer(), builder.build_broadleaf(), builder.build_broadleaf()]
	var needle_tex := TreeBuilder.make_texture(TreeBuilder.NEEDLE, 11)
	var leaf_tex := TreeBuilder.make_texture(TreeBuilder.LEAF, 23)
	var groups: Array = [[], [], [], []]
	for xf in transforms:
		var broadleaf := xf.origin.y < 90.0 and randf() < 0.7
		groups[(2 if broadleaf else 0) + randi() % 2].append(xf)
	for v in 4:
		var list: Array = groups[v]
		if list.is_empty():
			continue
		var mm := MultiMesh.new()
		mm.transform_format = MultiMesh.TRANSFORM_3D
		mm.mesh = variants[v]
		mm.instance_count = list.size()
		for i in list.size():
			mm.set_instance_transform(i, list[i])
		var mmi := MultiMeshInstance3D.new()
		mmi.multimesh = mm
		mmi.visibility_range_end = 650.0
		mmi.visibility_range_end_margin = 60.0
		mmi.visibility_range_fade_mode = GeometryInstance3D.VISIBILITY_RANGE_FADE_SELF
		var mat := ShaderMaterial.new()
		mat.shader = load("res://tree.gdshader")
		mat.set_shader_parameter("leaf_tex", needle_tex if v < 2 else leaf_tex)
		mmi.material_override = mat
		tree_mats.append(mat)
		_apply_tree_look(mat)
		add_child(mmi)
	return transforms.size()

# ---------------------------------------------------------------------------------------- autotest
func _save_shot(name: String) -> void:
	if DisplayServer.get_name() == "headless":
		return   # no renderer in headless runs (used while the owner is at the laptop); logic checks still run
	var img := get_viewport().get_texture().get_image()
	img.save_png("%s/%s.png" % [OUT_DIR, name])

func _shot(name: String) -> void:
	await get_tree().process_frame
	await get_tree().process_frame
	await get_tree().create_timer(0.15).timeout
	_save_shot(name)
	metrics.append("shot %s fps=%d debris=%d" % [name, Engine.get_frames_per_second(), debris.size()])

func _fail(reason: String, code: int) -> void:
	# An incomplete route must not look like a completed feasibility test: record why and exit non-zero.
	metrics.append("FAILED: %s (exit %d)" % [reason, code])
	_flush_metrics()
	print("AUTOTEST FAILED: ", reason)
	_quit_clean(code)

func _quit_clean(code: int) -> void:
	# stop the voxel workers before quitting so the exit code is ours, not a shutdown crash
	terrain.queue_free()
	for i in 20:
		await get_tree().process_frame
	get_tree().quit(code)

func _flush_metrics() -> void:
	var f := FileAccess.open(OUT_DIR + "/metrics.txt", FileAccess.WRITE)
	for l in metrics:
		f.store_line(l)
	f.close()

func _place_ship(pos: Vector3, look: Vector3) -> void:
	ship.global_position = pos
	ship.look_at(look, Vector3.UP)
	yaw = ship.rotation.y
	pitch = ship.rotation.x

func _fly(to: Vector3, look: Vector3, secs: float) -> void:
	var from := ship.global_position
	var t := 0.0
	while t < secs:
		var dt := get_process_delta_time()
		t += dt
		ship.global_position = from.lerp(to, clampf(t / secs, 0.0, 1.0))
		ship.look_at(look, Vector3.UP)
		await get_tree().process_frame
	speed = from.distance_to(to) / secs

func _wait_terrain(at: Vector3) -> bool:
	var tool: VoxelTool = terrain.get_voxel_tool()
	for i in 600:
		await get_tree().create_timer(0.1).timeout
		var hit = tool.raycast(Vector3(at.x, 480.0, at.z), Vector3.DOWN, 700.0)
		if hit != null:
			return true
	return false

func _selftest_input() -> void:
	# Drives the real _input handler with synthetic events: Escape must release AND restore mouse capture,
	# and a click while released must recapture it.
	await get_tree().create_timer(1.0).timeout
	var ok := Input.mouse_mode == Input.MOUSE_MODE_CAPTURED
	var esc := InputEventKey.new()
	esc.keycode = KEY_ESCAPE
	esc.pressed = true
	_input(esc)
	ok = ok and Input.mouse_mode == Input.MOUSE_MODE_VISIBLE
	_input(esc)
	ok = ok and Input.mouse_mode == Input.MOUSE_MODE_CAPTURED
	_input(esc)
	var click := InputEventMouseButton.new()
	click.button_index = MOUSE_BUTTON_LEFT
	click.pressed = true
	_input(click)
	ok = ok and Input.mouse_mode == Input.MOUSE_MODE_CAPTURED
	print("SELFTEST_INPUT ", "PASS" if ok else "FAIL")
	get_tree().quit(0 if ok else 1)

func _start_interactive() -> void:
	await _wait_stream()
	_place_trees(700, 420.0, Vector3.ZERO)

func _wait_stream() -> void:
	# poll a coarse grid until the number of loaded ground hits stops growing
	var tool: VoxelTool = terrain.get_voxel_tool()
	var last := -1
	var stable := 0
	for i in 300:
		await get_tree().create_timer(0.5).timeout
		var n := 0
		for gx in range(-8, 9):
			for gz in range(-8, 9):
				if tool.raycast(Vector3(gx * 45.0, 480.0, gz * 45.0), Vector3.DOWN, 700.0) != null:
					n += 1
		if n == last:
			stable += 1
			if stable >= 4:
				break
		else:
			stable = 0
		last = n
	metrics.append("terrain streaming settled: %d/289 columns loaded" % last)

func _run_autotest() -> void:
	metrics.append("godot %s  renderer=%s" % [Engine.get_version_info()["string"], RenderingServer.get_video_adapter_name()])
	var t_start := Time.get_ticks_msec()
	_place_ship(Vector3(0, 300, 0), Vector3(0, 0, -100))
	var ok: bool = await _wait_terrain(Vector3(0, 0, 0))
	metrics.append("terrain streamed near origin: %s in %.1f s" % [ok, (Time.get_ticks_msec() - t_start) / 1000.0])
	if not ok:
		_fail("terrain did not stream in time", 2)
		return
	await _wait_stream()
	# survey the heightfield to find the mountain
	var tool: VoxelTool = terrain.get_voxel_tool()
	var peak := Vector3(0, -1000, 0)
	var lo := 9999.0
	for gx in range(-9, 10):
		for gz in range(-9, 10):
			var h = tool.raycast(Vector3(gx * 40.0, 480.0, gz * 40.0), Vector3.DOWN, 700.0)
			if h != null:
				if h.position.y > peak.y: peak = h.position
				lo = minf(lo, h.position.y)
	metrics.append("survey: peak %s low %.0f" % [str(peak), lo])
	print("PEAK ", peak, " LOW ", lo)
	if explore:
		_place_ship(peak + Vector3(0, 90, 360), peak)
		await _shot("explore_peak")
		_flush_metrics()
		get_tree().quit()
		return
	var trees: int = _place_trees(700, 360.0, Vector3(peak.x, 0, peak.z))
	metrics.append("trees placed: %d" % trees)
	if trees == 0:
		_fail("no trees could be placed", 4)
		return
	# 1. overview of the mountain from outside, above the cloud layer
	_place_ship(peak + Vector3(0, 130, 420), peak + Vector3(0, -40, 0))
	await _shot("01_overview")
	# 2. fly down through the cloud layer toward the mountain, trail carving the clouds
	await _fly(Vector3(peak.x - 80, 190, peak.z + 300), peak, 3.0)
	await _fly(Vector3(peak.x - 20, 150, peak.z + 120), peak, 4.0)
	await _shot("02_in_cloud")
	await _fly(Vector3(peak.x + 60, 175, peak.z + 60), peak + Vector3(0, 0, 200), 3.0)
	_place_ship(ship.global_position, ship.global_position + Vector3(0.4, -0.15, 1.0) * 100.0)
	await _shot("03_cloud_trail_behind")
	# 3. trees: a low tree, far then close. The shake is time-varying, so take two frames when close.
	var spot := Vector3.ZERO
	var best := 1e9
	for sp in tree_spots:
		if sp.y > 20.0 and sp.y < 80.0 and sp.distance_to(peak) < best:
			best = sp.distance_to(peak)
			spot = sp
	if spot == Vector3.ZERO and not tree_spots.is_empty():
		spot = tree_spots[0]
	metrics.append("tree test spot %s (of %d trees)" % [str(spot), tree_spots.size()])
	_place_ship(spot + Vector3(40, 16, 90), spot + Vector3(0, 6, 0))
	await _shot("04_trees_far")
	await _fly(spot + Vector3(6, 7, 14), spot + Vector3(0, 5, 0), 2.0)
	# control: identical pose with the shake switched off, then on; compose a side-by-side image
	for m in tree_mats:
		m.set_shader_parameter("shake_strength", 0.0)
	await _shot("05_trees_shake_off")
	if DisplayServer.get_name() == "headless":
		metrics.append("tree shake on/off: skipped (headless run has no renderer)")
	else:
		var off_img := get_viewport().get_texture().get_image()
		for m in tree_mats:
			m.set_shader_parameter("shake_strength", 8.0)
		await get_tree().create_timer(0.31).timeout
		await _shot("05_trees_shake_on")
		var on_img := get_viewport().get_texture().get_image()
		var w := on_img.get_width()
		var h := on_img.get_height()
		# measure: pixels that changed (colour distance > 0.06) in a window around the tree vs an equal control window
		var sp := cam.unproject_position(spot + Vector3(0, 5, 0))
		var win := 90
		var tree_rect := Rect2i(int(sp.x) - win, int(sp.y) - win, win * 2, win * 2).intersection(Rect2i(0, 40, w, h - 40))
		var ctrl_rect := Rect2i(w - 2 * win - 20, 60, win * 2, win * 2)
		var count_changed := func(r: Rect2i) -> Array:
			var n := 0
			var total := 0
			for y in range(r.position.y, r.end.y, 2):
				for x in range(r.position.x, r.end.x, 2):
					total += 1
					var ca := off_img.get_pixel(x, y)
					var cb := on_img.get_pixel(x, y)
					if absf(ca.r - cb.r) + absf(ca.g - cb.g) + absf(ca.b - cb.b) > 0.06:
						n += 1
			return [n, total]
		var tr: Array = count_changed.call(tree_rect)
		var cr: Array = count_changed.call(ctrl_rect)
		metrics.append("tree shake on/off: tree window %s changed %d/%d ; control window %s changed %d/%d" % [str(tree_rect), tr[0], tr[1], str(ctrl_rect), cr[0], cr[1]])
		var cmp := Image.create(w, h / 2, false, on_img.get_format())
		var off_s := off_img.duplicate()
		var on_s := on_img.duplicate()
		off_s.resize(w / 2, h / 2)
		on_s.resize(w / 2, h / 2)
		cmp.blit_rect(off_s, Rect2i(0, 0, w / 2, h / 2), Vector2i(0, 0))
		cmp.blit_rect(on_s, Rect2i(0, 0, w / 2, h / 2), Vector2i(w / 2, 0))
		cmp.save_png(OUT_DIR + "/05_compare_off_left_on_right.png")
	# 4. drill into the mountain face
	var face_h := peak.y * 0.5
	var origin := Vector3(peak.x + 330, face_h, peak.z)
	var fh = tool.raycast(origin, (peak - origin).normalized() * Vector3(1, 0, 1), 700.0)
	if selftest_fail == "face":
		fh = null
	if fh == null:
		_fail("no mountain face found", 3)
		return
	var P: Vector3 = fh.position
	var N: Vector3 = fh.normal if fh.normal.length() > 0.1 else Vector3(1, 0, 0)
	var inward := -Vector3(N.x, 0, N.z).normalized()
	metrics.append("drill start %s inward %s" % [str(P), str(inward)])
	_place_ship(P - inward * 28.0 + Vector3(0, 4, 0), P)
	await _shot("06_before_drill")
	var step_t := 0
	for k in 34:
		var c := P + inward * (k * 2.2)
		carve(c, 4.8)
		_spawn_debris(c - inward * 2.0, 1, -inward)
		if k % 4 == 0:
			_dust(c, 18)
		shake = maxf(shake, 0.08)
		ship.global_position = (P + inward * (k * 2.2 - 18.0)) + Vector3(0, 0.5, 0)
		ship.look_at(c + inward * 10.0, Vector3.UP)
		await get_tree().create_timer(0.12).timeout
		if k == 8:
			await _shot("07_drilling")
		if k == 24:
			await _shot("08_inside_tunnel")
	# 5. hollow out a chamber and let it crumble
	var end := P + inward * (34 * 2.2)
	hollow(end + inward * 8.0, 17.0)
	ship.global_position = end - inward * 6.0 + Vector3(0, 1.0, 0)
	ship.look_at(end + inward * 12.0 + Vector3(0, 3, 0), Vector3.UP)
	await _shot("09_hollow_start")
	await get_tree().create_timer(1.6).timeout
	await _shot("10_crumble_mid")
	await get_tree().create_timer(2.8).timeout
	await _shot("11_crumble_late")
	await get_tree().create_timer(2.5).timeout
	await _shot("12_aftermath")
	# fly out and look back at the opened mountain from outside
	_place_ship(P - inward * 60.0 + Vector3(0, 25, 40), P + inward * 30.0)
	await _shot("13_exterior_after")
	# metrics
	var avg := 0.0
	for v in edit_ms: avg += v
	if edit_ms.size() > 0: avg /= edit_ms.size()
	var mx := 0.0
	for v in edit_ms: mx = maxf(mx, v)
	metrics.append("voxel edits: %d  avg %.2f ms  max %.2f ms" % [edit_ms.size(), avg, mx])
	metrics.append("static memory MiB: %.0f   total run %.1f s" % [OS.get_static_memory_usage() / 1048576.0, (Time.get_ticks_msec() - t_start) / 1000.0])
	_flush_metrics()
	print("AUTOTEST DONE")
	get_tree().quit()
