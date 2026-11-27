extends RefCounted
# G3: the first gameplay loop on the Everward simulation -- scan, survey, auto-approach, arm path, mine -- with the
# controls and messages of the Unreal build (EverwardPlayerControllerMiningControls.cpp). Every decision (survey,
# staging pose, joint path, reach, extraction, storage) comes from the simulation through the EverwardSim binding;
# this script only reads input, steers toward the solved pose like the Unreal auto-approach, and draws the result.
#
# Keys: R scan the nearest deposit (R again cancels), 1/2 select port/starboard arm, T deploy/stow the selected arm,
# Y attach/detach its tool, P auto-approach (P or SPACE cancels), G mine.
# Power (#378 G09): K installs the GEN1 BASIC SURVIVAL policy, L clears it, O sets Sensors to the 50 W scan minimum.
# A refused scan shows the simulation's own explanation and recovery steps.
# Systems panel (#378 G11): Tab shows it; [ and ] move the selection with the simulation's bounded_selection rule
# whether or not the panel is open; selecting an arm makes it the arm T/Y/P act on; Enter on a power subsystem
# toggles it between 0 W and its operating minimum.
# Fix_It (#378 G02) runs inside the simulation on its own, as in the Unreal build: the probe wakes damaged, and Fix_It
# repairs in canonical order only while stored material and energy cover the next step. This script shows it.

const SCAN_RANGE_M := 250.0
const SCAN_DURATION_S := 10.0          # Unreal Phase2ScanDurationSeconds
const APPROACH_SPEED := 2.0            # Unreal AutoApproachSpeedMetersPerSecond
const APPROACH_TOLERANCE := 0.25       # Unreal AutoApproachStopToleranceMeters
const WAYPOINT_TOLERANCE_DEG := 1e-3
const WAYPOINT_TIMEOUT_S := 10.0
const ARM_NAMES := ["PORT", "STARBOARD"]
const SENSOR_SCAN_MINIMUM_W := 50.0     # ProbeRuntime::kGeneration1MinimumSensorPowerW
const COMPUTATION_MINIMUM_W := 25.0      # ProbeRuntime::kGeneration1MinimumPolicyComputationPowerW
const CAPABILITIES := ["SENSORS", "PROPULSION", "COMPUTATION", "THERMAL", "PORT ARM", "STARBOARD ARM"]

var m                                   # the world-spike main node
var deposits: Array[Dictionary] = []    # {id, material, name, center (Godot), radius, mesh, mat}
var selected_arm := 0
var op_arm := 0                         # the arm the active approach and alignment were solved for
var approaching := false
var staging := Vector3.ZERO             # simulation frame
var staging_angles := PackedFloat64Array()
var staging_gap := 0.0
var approach_target := ""
var waypoints: Array = []
var waypoint_index := -1
var waypoint_elapsed := 0.0
var aligned := false
var message := ""
var last_mine: Dictionary = {}
var fix_it_log: Array[String] = []
var panel_open := false
var selected_capability := 0
var unbounded_selection_for_test := false   # --hudtest --selftest-fail=unbounded: the old unclamped rule, must fail
var arm_meshes: Array = [[], []]        # per arm: 3 link cylinders + tool tip
var scan_beam: MeshInstance3D
var arm_mat: StandardMaterial3D
var tip_mat: StandardMaterial3D

func _init(main_node) -> void:
	m = main_node
	arm_mat = StandardMaterial3D.new()
	arm_mat.albedo_color = Color(0.86, 0.66, 0.2)
	arm_mat.metallic = 0.7
	arm_mat.roughness = 0.4
	tip_mat = StandardMaterial3D.new()
	tip_mat.albedo_color = Color(1.0, 0.45, 0.15)
	tip_mat.emission_enabled = true
	tip_mat.emission = Color(1.0, 0.4, 0.1)
	tip_mat.emission_energy_multiplier = 2.0
	for arm in 2:
		for i in 3:
			var link := MeshInstance3D.new()
			var cyl := CylinderMesh.new()
			cyl.top_radius = 0.12
			cyl.bottom_radius = 0.12
			cyl.height = 1.0
			link.mesh = cyl
			link.material_override = arm_mat
			m.add_child(link)
			arm_meshes[arm].append(link)
		var tip := MeshInstance3D.new()
		var ts := SphereMesh.new()
		ts.radius = 0.16
		ts.height = 0.32
		tip.mesh = ts
		tip.material_override = tip_mat
		m.add_child(tip)
		arm_meshes[arm].append(tip)
	scan_beam = MeshInstance3D.new()
	var beam := CylinderMesh.new()
	beam.top_radius = 0.05
	beam.bottom_radius = 0.35
	beam.height = 1.0
	scan_beam.mesh = beam
	var bm := StandardMaterial3D.new()
	bm.shading_mode = BaseMaterial3D.SHADING_MODE_UNSHADED
	bm.transparency = BaseMaterial3D.TRANSPARENCY_ALPHA
	bm.albedo_color = Color(0.4, 0.9, 1.0, 0.35)
	scan_beam.material_override = bm
	scan_beam.visible = false
	m.add_child(scan_beam)

# A deposit is a registered simulation body with a resource; the mesh is only its picture.
func add_deposit(id: String, center: Vector3, radius: float, material: String, display: String, kg: float) -> bool:
	var s: Vector3 = m._godot_to_sim(center)
	if not m.sim.add_deposit(id, s.x, s.y, s.z, radius, material, display, kg, 5.0):
		return false
	var mi := MeshInstance3D.new()
	var sm := SphereMesh.new()
	sm.radius = radius
	sm.height = radius * 2.0
	mi.mesh = sm
	var mat := StandardMaterial3D.new()
	mat.albedo_color = Color(0.42, 0.24, 0.16)
	mat.roughness = 0.85
	mat.emission_enabled = true
	mat.emission = Color(0.0, 0.0, 0.0)
	mi.material_override = mat
	m.add_child(mi)
	mi.global_position = center
	deposits.append({"id": id, "material": material, "name": display, "center": center, "radius": radius, "mesh": mi, "mat": mat})
	return true

func nearest_deposit() -> Dictionary:
	var best: Dictionary = {}
	var best_d := INF
	for d in deposits:
		var dist: float = m.ship.global_position.distance_to(d["center"]) - float(d["radius"])
		if dist < best_d:
			best_d = dist
			best = d
	return best if best_d <= SCAN_RANGE_M else {}

func busy() -> bool:
	return approaching or waypoint_index >= 0

func say(text: String) -> void:
	message = text
	m.metrics.append("first-loop: " + text)

# ------------------------------------------------------------------------------------------- controls
func handle_key(keycode: int) -> bool:
	match keycode:
		KEY_R:
			if m.sim.is_scanning():
				m.sim.cancel_scan()
				say("scan cancelled")
			else:
				var d := nearest_deposit()
				if d.is_empty():
					say("no deposit within %d m to scan" % int(SCAN_RANGE_M))
				elif m.sim.start_scan(d["id"], SCAN_DURATION_S):
					say("scanning " + String(d["name"]))
				else:
					say("scan refused: " + String(m.sim.get_last_scan_refusal()))
		KEY_1, KEY_2:
			selected_arm = 0 if keycode == KEY_1 else 1
			selected_capability = 4 + selected_arm   # the panel and the arm keys always agree
			say("%s arm selected" % ARM_NAMES[selected_arm])
		KEY_TAB:
			panel_open = not panel_open
			say("systems panel " + ("open" if panel_open else "closed"))
		KEY_BRACKETRIGHT, KEY_BRACKETLEFT:
			var step := 1 if keycode == KEY_BRACKETRIGHT else -1
			if unbounded_selection_for_test:
				selected_capability += step
			else:
				selected_capability = int(m.sim.bounded_selection(selected_capability, step, CAPABILITIES.size()))
			if selected_capability >= 4 and selected_capability < CAPABILITIES.size():
				selected_arm = selected_capability - 4
			say("%s selected" % (CAPABILITIES[selected_capability] if selected_capability >= 0 and selected_capability < CAPABILITIES.size() else "index %d (invalid)" % selected_capability))
		KEY_ENTER, KEY_KP_ENTER:
			act_on_selected()
		KEY_T:
			if m.sim.toggle_arm(selected_arm):
				var st: Dictionary = m.sim.get_arm_state(selected_arm)
				say("%s arm %s" % [ARM_NAMES[selected_arm], "deploying" if st["deploying"] else "stowing"])
				aligned = false
			else:
				say("%s arm: detach the tool before stowing" % ARM_NAMES[selected_arm])
		KEY_Y:
			var st: Dictionary = m.sim.get_arm_state(selected_arm)
			if st["tool_attached"]:
				say("%s tool %s" % [ARM_NAMES[selected_arm], "detached" if m.sim.detach_tool(selected_arm) else "detach refused"])
			else:
				say("%s tool %s" % [ARM_NAMES[selected_arm], "attached" if m.sim.attach_tool(selected_arm) else "needs a fully deployed arm"])
		KEY_K:
			say("GEN1 BASIC SURVIVAL policy %s" % ("installed" if m.sim.install_basic_survival_policy() else "refused"))
		KEY_L:
			say("software policy cleared" if m.sim.clear_policy() else "no software policy installed")
		KEY_O:
			var was_scanning: bool = m.sim.is_scanning()
			if m.sim.allocate_power(0, SENSOR_SCAN_MINIMUM_W):
				say("sensors power set to %d W" % int(SENSOR_SCAN_MINIMUM_W))
			else:
				say("sensor power change refused" + (" (scan was running)" if was_scanning else ""))
		KEY_P:
			toggle_approach()
		KEY_G:
			mine()
		_:
			return false
	return true

func act_on_selected() -> void:
	var minimums := [SENSOR_SCAN_MINIMUM_W, 0.0, COMPUTATION_MINIMUM_W, 0.0]
	if selected_capability >= 4:
		say("%s: use T to deploy or stow, Y for the tool" % CAPABILITIES[selected_capability])
		return
	var minimum: float = minimums[selected_capability]
	if minimum <= 0.0:
		say("%s has no operating minimum to restore" % CAPABILITIES[selected_capability])
		return
	var current: float = m.sim.get_power_allocations()[selected_capability]
	var target := 0.0 if current >= minimum else minimum
	if m.sim.allocate_power(selected_capability, target):
		say("%s power set to %d W" % [CAPABILITIES[selected_capability], int(target)])
	else:
		say("%s power change refused" % CAPABILITIES[selected_capability])

# The panel text: one line per capability with its live state, the selected one marked with ">".
func panel_lines() -> String:
	var p: PackedFloat64Array = m.sim.get_power_allocations()
	var i: Dictionary = m.sim.get_integrity()
	var integ := [i["sensors"], i["propulsion"], i["computation"], i["thermal"]]
	var out := "SYSTEMS  ([ ] select, Enter act, Tab close)"
	for k in CAPABILITIES.size():
		var state := ""
		if k < 4:
			state = "%d W  integrity %d%%" % [int(p[k]), int(round(float(integ[k]) * 100.0))]
		else:
			var st: Dictionary = m.sim.get_arm_state(k - 4)
			state = ("deployed" if st["deployed"] and not st["stowing"] else ("moving" if st["deploying"] or st["stowing"] else "stowed")) + ("  tool on" if st["tool_attached"] else "")
		out += "\n%s %s  %s" % [">" if k == selected_capability else " ", CAPABILITIES[k], state]
	return out

func toggle_approach() -> void:
	if approaching:
		cancel_approach("mining auto-approach cancelled")
		return
	var d := nearest_deposit()
	if d.is_empty():
		say("no deposit in range for auto-approach")
		return
	var plan: Dictionary = m.sim.plan_staging(selected_arm, d["id"])
	if not plan["has_solution"]:
		say("no collision-free mining pose for the %s arm at this attitude; rotate the probe and press P again" % ARM_NAMES[selected_arm])
		return
	var p: PackedFloat64Array = plan["position"]
	staging = Vector3(p[0], p[1], p[2])
	staging_angles = plan["angles"]
	staging_gap = plan["tool_gap_m"]
	op_arm = selected_arm   # selecting another arm later never redirects this operation
	approach_target = d["id"]
	approaching = true
	aligned = false
	waypoint_index = -1
	say("mining auto-approach engaged; SPACE or P cancels")

func cancel_approach(text: String) -> void:
	approaching = false
	m.sim.set_velocity(0.0, 0.0, 0.0)
	say(text)

func mine() -> void:
	var d := nearest_deposit()
	if d.is_empty():
		say("no deposit in range")
		return
	last_mine = m.sim.mine_deposit(d["id"])
	if last_mine["accepted"]:
		say("mined %.1f kg %s with the %s arm (tool gap %.2f m); %.1f kg left" % [last_mine["extracted_kg"], d["name"], ARM_NAMES[int(last_mine["arm"])], last_mine["tool_gap_m"], last_mine["remaining_kg"]])
		m._dust(d["center"], 40)
	else:
		say("mining refused: " + String(last_mine["reason"]))

# ------------------------------------------------------------------------------------------- per frame
func process(delta: float) -> void:
	for line in m.sim.drain_fix_it_events():
		fix_it_log.append(line)
		m.metrics.append("Fix_It: " + line)
	if approaching:
		var p: PackedFloat64Array = m.sim.get_position()
		var to := staging - Vector3(p[0], p[1], p[2])
		var dist := to.length()
		if dist <= APPROACH_TOLERANCE:
			approaching = false
			m.sim.set_velocity(0.0, 0.0, 0.0)
			_align()
		else:
			var spd := minf(APPROACH_SPEED, maxf(0.25, dist))
			var v := to / dist * spd
			if not m.sim.set_velocity(v.x, v.y, v.z):
				cancel_approach("auto-approach stopped: velocity refused")
	if waypoint_index >= 0:
		_follow(delta)
	_draw()

func _align() -> void:
	var st: Dictionary = m.sim.get_arm_state(op_arm)
	if not st["deployed"] or st["deploying"] or st["stowing"] or not st["tool_attached"]:
		say("mining pose reached; fully deploy the %s arm and attach its tool, then press P to align it" % ARM_NAMES[op_arm])
		return
	var path: Dictionary = m.sim.plan_arm_path(op_arm, staging_angles[0], staging_angles[1], staging_angles[2])
	if not path["ok"]:
		say("no collision-free joint path for the %s arm to the mining pose; rotate the probe and press P again" % ARM_NAMES[op_arm])
		return
	waypoints = path["waypoints"]
	waypoint_index = 0
	_command_waypoint()
	say("mining pose reached: aligning %s arm (planned tool-tip gap %.2f m)" % [ARM_NAMES[op_arm], staging_gap])

func _command_waypoint() -> void:
	waypoint_elapsed = 0.0
	if waypoint_index >= waypoints.size():
		waypoint_index = -1
		aligned = true
		say("%s arm aligned; press G to mine" % ARM_NAMES[op_arm])
		return
	var w: PackedFloat64Array = waypoints[waypoint_index]
	m.sim.set_joint_targets(op_arm, w[0], w[1], w[2])

func _follow(delta: float) -> void:
	var w: PackedFloat64Array = waypoints[waypoint_index]
	var a: PackedFloat64Array = m.sim.get_arm_state(op_arm)["angles"]
	if absf(a[0] - w[0]) < WAYPOINT_TOLERANCE_DEG and absf(a[1] - w[1]) < WAYPOINT_TOLERANCE_DEG and absf(a[2] - w[2]) < WAYPOINT_TOLERANCE_DEG:
		waypoint_index += 1
		_command_waypoint()
		return
	waypoint_elapsed += delta
	if waypoint_elapsed > WAYPOINT_TIMEOUT_S:
		waypoint_index = -1
		say("arm alignment stalled (a joint frame was refused); press P to retry")

func _draw() -> void:
	for arm in 2:
		var pts: Array = m.sim.get_arm_points(arm)
		var meshes: Array = arm_meshes[arm]
		for i in 3:
			_segment(meshes[i], m._sim_to_godot(pts[i]), m._sim_to_godot(pts[i + 1]))
		(meshes[3] as MeshInstance3D).global_position = m._sim_to_godot(pts[3])
	var scanning: bool = m.sim.is_scanning()
	scan_beam.visible = scanning
	if scanning:
		for d in deposits:
			if d["id"] == m.sim.get_scan_target():
				_segment(scan_beam, d["center"], m.ship.global_position)
	for d in deposits:
		var mat: StandardMaterial3D = d["mat"]
		mat.emission = Color(0.9, 0.45, 0.1) * 0.6 if m.sim.is_surveyed(d["id"]) else Color(0, 0, 0)

func _segment(mi: MeshInstance3D, a: Vector3, b: Vector3) -> void:
	var dir := b - a
	var length := dir.length()
	if length < 1e-4:
		mi.visible = false
		return
	mi.visible = true
	var y := dir / length
	var x := y.cross(Vector3.UP)
	if x.length() < 1e-3:
		x = y.cross(Vector3.RIGHT)
	x = x.normalized()
	var z := x.cross(y)
	mi.global_transform = Transform3D(Basis(x, y * length, z), (a + b) * 0.5)

func hud_lines() -> String:
	var d := nearest_deposit()
	var target := "no deposit in range"
	if not d.is_empty():
		var state := "SURVEYED" if m.sim.is_surveyed(d["id"]) else "unsurveyed"
		if m.sim.is_scanning():
			state = "scanning %.1f s" % m.sim.get_scan_remaining_s()
		target = "%s  %.0f m  %s  %.1f kg left" % [d["name"], m.ship.global_position.distance_to(d["center"]) - float(d["radius"]), state, m.sim.get_world_deposit_remaining_kg(d["id"])]
	var st: Dictionary = m.sim.get_arm_state(selected_arm)
	var arm_state := "stowed"
	if st["deploying"]:
		arm_state = "deploying %d%%" % int(float(st["deployment_fraction"]) * 100.0)
	elif st["stowing"]:
		arm_state = "stowing %d%%" % int(float(st["deployment_fraction"]) * 100.0)
	elif st["deployed"]:
		arm_state = "deployed"
	var cargo := ""
	for dd in deposits:
		var kg: float = m.sim.get_inventory_kg(dd["material"])
		if kg > 0.0:
			cargo += "  %s %.1f kg" % [dd["name"], kg]
	if panel_open:
		return panel_lines() + "\n" + message
	return "TARGET %s\nARM %s %s  tool %s%s%s\nCARGO%s  storage %.1f / %.0f kg\n%s\n%s" % [
		target, ARM_NAMES[selected_arm], arm_state, "on" if st["tool_attached"] else "off",
		"  APPROACHING" if approaching else "", "  ALIGNED" if aligned else "",
		cargo if cargo != "" else " empty", m.sim.get_storage_used_kg(), m.sim.get_storage_capacity_kg(), fix_it_line() + "\n" + power_line(), message]

# Power and policy, with the simulation's compact recovery hint whenever scanning is blocked (G09).
func power_line() -> String:
	var p: PackedFloat64Array = m.sim.get_power_allocations()
	var pol: Dictionary = m.sim.get_policy_status()
	var g: Dictionary = m.sim.get_scan_power_guidance()
	var line := "POWER SENS %d W (scan %d) PROP %d COMP %d THERM %d // POLICY %s" % [int(p[0]), int(SENSOR_SCAN_MINIMUM_W), int(p[1]), int(p[2]), int(p[3]), String(pol["policy_id"]) if pol["installed"] else "none"]
	if g["state"] != "ready":
		line += " // " + String(g["compact"])
	return line

# The Unreal FixItRuntimeActor summary format, plus the integrity of all four subsystems.
func fix_it_line() -> String:
	var f: Dictionary = m.sim.get_fix_it_status()
	if not f["enabled"]:
		return "FIX_IT // OFF"
	var i: Dictionary = m.sim.get_integrity()
	var health := "COMP %d%% THERM %d%% SENS %d%% PROP %d%%" % [int(round(float(i["computation"]) * 100.0)), int(round(float(i["thermal"]) * 100.0)), int(round(float(i["sensors"]) * 100.0)), int(round(float(i["propulsion"]) * 100.0))]
	match String(f["state"]):
		"repairing":
			return "FIX_IT // REPAIR %s // %d%% // %.2f/%.2f KG // %s" % [f["subsystem"], int(float(f["progress"]) * 100.0), f["material_consumed_kg"], f["material_required_kg"], health]
		"waiting":
			return "FIX_IT // PRIORITY %s // WAITING: %.2f KG + %.0f KJ // STORED %.2f KG // %s" % [f["subsystem"], f["material_required_kg"], float(f["energy_required_j"]) / 1000.0, m.sim.get_storage_used_kg(), health]
		_:
			return "FIX_IT // MONITOR // %s" % health
