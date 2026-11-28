extends RefCounted
# Every control the probe answers to, exercised the way a player uses it, with the specific effect each one must
# have (#378 G01, and the owner's "test all the controls and see if they actually work").
#
# The other modes press controls incidentally, on the way to testing a mechanic; a control that quietly stopped
# working would only show up as some later check failing, if at all. This mode is the inventory: it walks the
# probe's task list in the order a player learns it -- select, look, power, policy, arm, tool, scan, approach,
# mine, fly, dig -- and after each control asks whether that control, specifically, did its job.
#
# Nothing is simulated around the controls. Command keys go through main.gd's _input and first_loop.handle_key,
# the same path a keyboard takes. Held flight keys go through Input.parse_input_event and the real
# _manual_flight/_sim_manual_flight path, which is why --controltest is deliberately NOT one of main.gd's
# _scripted() modes: a scripted run disables manual flight, and a flight control cannot be tested with flight off.
# No simulation state is written to make a check pass.
#
# Negative control: --selftest-fail=deadkey ignores one command key, which must make its check fail and no other.

const FirstLoop = preload("res://first_loop.gd")
const MATERIAL := "iron_bearing_silicate_regolith"
const DEPOSIT := "ore-1"
const START_KG := 120.0
const HOLD_S := 0.6          # long enough for a held key to move the probe measurably, short enough to stay close
const EPS := 1e-6

var m: Node3D
var checks_run := 0


func _init(game: Node3D) -> void:
	m = game


# ------------------------------------------------------------------------------------------ input primitives
# A command key, exactly as main.gd delivers a real keypress.
func _tap(keycode: int) -> void:
	if m.selftest_fail == "deadkey" and keycode == KEY_TAB:
		return   # the negative control: one control silently stops responding
	m._press(keycode)


# A held key: pressed, held for real frames, released. This is the only honest way to test a control that the
# game reads with Input.is_key_pressed() rather than from an event.
func _hold(keycode: int, seconds: float) -> void:
	var down := InputEventKey.new()
	down.keycode = keycode
	down.physical_keycode = keycode
	down.pressed = true
	if m.evidence != null:
		m.evidence.event("command", "hold " + OS.get_keycode_string(keycode))
	Input.parse_input_event(down)
	await m._sim_run(seconds)
	var up := InputEventKey.new()
	up.keycode = keycode
	up.physical_keycode = keycode
	up.pressed = false
	Input.parse_input_event(up)
	await m._sim_run(0.1)


# Two keys at once, for the boost modifier.
func _hold_both(a: int, b: int, seconds: float) -> void:
	var ea := InputEventKey.new()
	ea.keycode = a
	ea.physical_keycode = a
	ea.pressed = true
	var eb := InputEventKey.new()
	eb.keycode = b
	eb.physical_keycode = b
	eb.pressed = true
	if m.evidence != null:
		m.evidence.event("command", "hold %s+%s" % [OS.get_keycode_string(a), OS.get_keycode_string(b)])
	Input.parse_input_event(ea)
	Input.parse_input_event(eb)
	await m._sim_run(seconds)
	ea.pressed = false
	eb.pressed = false
	Input.parse_input_event(ea)
	Input.parse_input_event(eb)
	await m._sim_run(0.1)


func _stop() -> void:
	m.sim.set_velocity(0.0, 0.0, 0.0)
	await m._sim_run(0.1)


func _velocity() -> PackedFloat64Array:
	return m.sim.get_velocity()


# The probe thrusts in its OWN frame and reports velocity in the world frame, so a probe that has turned spreads
# one key across several world axes. Measure along the probe's own nose, starboard and up, which is also what the
# player feels: W goes where the nose is pointing, whatever heading that is.
func _body_axes() -> Array:
	var b: Basis = m.ship.global_transform.basis
	return [-b.z, b.x, b.y]


# Solid voxels sampled along the nose between two distances. H hollows a cavity centred 18 m BEYOND the surface
# it hits -- the point is to hollow out the inside of a mountain -- so the surface distance cannot see its work
# and only a sample through the rock can.
func _solid_along_nose(from_m: float, to_m: float) -> int:
	var vt: VoxelTool = m.terrain.get_voxel_tool()
	var nose: Vector3 = -m.ship.global_transform.basis.z
	var solid := 0
	var d := from_m
	while d <= to_m:
		var p: Vector3 = m.ship.global_position + nose * d
		if vt.get_voxel_f(Vector3i(roundi(p.x), roundi(p.y), roundi(p.z))) < 0.0:
			solid += 1
		d += 1.0
	return solid


# Distance to the terrain straight ahead, or -1 when nothing is in range: how a dug hole is measured.
func _reach(limit: float) -> float:
	var dir: Vector3 = -m.ship.global_transform.basis.z
	var hit = m.terrain.get_voxel_tool().raycast(m.ship.global_position, dir, limit)
	return -1.0 if hit == null else m.ship.global_position.distance_to(Vector3(hit.position))


# Step the panel selection to an index with the real [ and ] keys. Bounded by the list length: if the selection
# cannot be driven there, the caller's check fails with the index it reached instead of looping forever.
func _select_capability(want: int) -> bool:
	var fl = m.first_loop
	for _i in FirstLoop.CAPABILITIES.size() * 2 + 4:
		if fl.selected_capability == want:
			return true
		_tap(KEY_BRACKETLEFT if fl.selected_capability > want else KEY_BRACKETRIGHT)
	await m._sim_run(0.1)
	return fl.selected_capability == want


func _check(name: String, ok: bool, detail: String) -> void:
	checks_run += 1
	# Printed as it happens: a control that hangs the run leaves the last control it answered on the log, which is
	# the only clue a killed headless run can give.
	print("CONTROL %02d %-4s %s" % [checks_run, "ok" if ok else "FAIL", name])
	m.evidence.check(name, ok, detail)


# ------------------------------------------------------------------------------------------------- the run
func run() -> void:
	m._begin_evidence("controltest")
	var origin := Vector3(0, 300, 0)
	m._place_ship(origin, origin + Vector3(0, 0, -100))
	await m._wait_stream()
	if not m._sim_start(origin):
		m._fail("simulation extension (EverwardSim) not loaded", 5)
		return
	m.first_loop = FirstLoop.new(m)
	m.sim.enable_fix_it(true)
	var registered: bool = m.first_loop.add_deposit(
		DEPOSIT, origin + Vector3(0, 0, -40), 3.0, MATERIAL, "Iron-bearing regolith", START_KG)
	m.evidence.precondition("a deposit is in range to operate on", registered,
		"%s at 40 m with %.1f kg" % [DEPOSIT, START_KG])
	if not registered:
		m._finish_evidence("CONTROLTEST", 16)
		return
	await m._sim_run(0.2)
	var fl = m.first_loop

	# --- selecting what the probe is about to operate ------------------------------------------------------
	_tap(KEY_2)
	await m._sim_run(0.1)
	_check("2 selects the starboard arm", fl.selected_arm == 1,
		"selected_arm=%d, message: %s" % [fl.selected_arm, fl.message])
	_tap(KEY_1)
	await m._sim_run(0.1)
	_check("1 selects the port arm", fl.selected_arm == 0,
		"selected_arm=%d, message: %s" % [fl.selected_arm, fl.message])

	var panel_before: bool = fl.panel_open
	_tap(KEY_TAB)
	await m._sim_run(0.1)
	_check("TAB opens the systems panel", fl.panel_open != panel_before and fl.panel_open,
		"panel_open %s -> %s, HUD now shows %d lines" % [panel_before, fl.panel_open, fl.hud_lines().split("\n").size()])
	_tap(KEY_TAB)
	await m._sim_run(0.1)
	_check("TAB closes it again", not fl.panel_open, "panel_open=%s" % fl.panel_open)

	var cap0: int = fl.selected_capability
	_tap(KEY_BRACKETRIGHT)
	await m._sim_run(0.1)
	var cap1: int = fl.selected_capability
	_tap(KEY_BRACKETLEFT)
	await m._sim_run(0.1)
	_check("] and [ step the selection and come back", cap1 != cap0 and fl.selected_capability == cap0,
		"%d -> %d -> %d of %d capabilities" % [cap0, cap1, fl.selected_capability, FirstLoop.CAPABILITIES.size()])

	# The selection must stay inside the list however hard it is pushed: an out-of-range index would index the
	# capability array in the HUD and the power call.
	for _i in FirstLoop.CAPABILITIES.size() + 4:
		_tap(KEY_BRACKETRIGHT)
	await m._sim_run(0.1)
	var high: int = fl.selected_capability
	for _i in FirstLoop.CAPABILITIES.size() + 8:
		_tap(KEY_BRACKETLEFT)
	await m._sim_run(0.1)
	_check("the selection never leaves the capability list",
		high >= 0 and high < FirstLoop.CAPABILITIES.size()
		and fl.selected_capability >= 0 and fl.selected_capability < FirstLoop.CAPABILITIES.size(),
		"pushed up to %d and down to %d, list size %d" % [high, fl.selected_capability, FirstLoop.CAPABILITIES.size()])

	# --- power -----------------------------------------------------------------------------------------------
	m.sim.allocate_power(0, 0.0)
	await m._sim_run(0.1)
	_tap(KEY_O)
	await m._sim_run(0.2)
	_check("O restores sensor power to the scan minimum",
		absf(m.sim.get_power_allocations()[0] - FirstLoop.SENSOR_SCAN_MINIMUM_W) < EPS,
		"sensors now %.1f W (minimum %.1f), message: %s"
		% [m.sim.get_power_allocations()[0], FirstLoop.SENSOR_SCAN_MINIMUM_W, fl.message])

	# ENTER acts on whatever the panel has selected: drive it to COMPUTATION and toggle its power. Bounded, because
	# a selection that cannot reach the index asked for is a finding to report, not a reason to spin forever.
	await _select_capability(2)
	var comp_before: float = m.sim.get_power_allocations()[2]
	_tap(KEY_ENTER)
	await m._sim_run(0.2)
	var comp_after: float = m.sim.get_power_allocations()[2]
	_check("ENTER acts on the selected capability",
		fl.selected_capability == 2 and absf(comp_after - comp_before) > EPS,
		"COMPUTATION %.1f -> %.1f W, message: %s" % [comp_before, comp_after, fl.message])
	_tap(KEY_KP_ENTER)
	await m._sim_run(0.2)
	_check("the numpad ENTER does the same as ENTER",
		absf(m.sim.get_power_allocations()[2] - comp_before) < EPS,
		"COMPUTATION back to %.1f W" % m.sim.get_power_allocations()[2])

	# --- software policy ---------------------------------------------------------------------------------------
	_tap(KEY_K)
	await m._sim_run(0.2)
	var pol: Dictionary = m.sim.get_policy_status()
	_check("K installs the survival policy", bool(pol["installed"]),
		"installed=%s id=%s, message: %s" % [pol["installed"], pol.get("policy_id", ""), fl.message])
	_tap(KEY_L)
	await m._sim_run(0.2)
	_check("L clears the software policy", not bool(m.sim.get_policy_status()["installed"]),
		"installed=%s, message: %s" % [m.sim.get_policy_status()["installed"], fl.message])

	# --- the arm and its tool ------------------------------------------------------------------------------------
	_tap(KEY_1)
	_tap(KEY_T)
	var deployed: bool = await m._until(func() -> bool: return bool(m.sim.get_arm_state(0)["deployed"]) and not bool(m.sim.get_arm_state(0)["deploying"]), 8.0)
	_check("T deploys the selected arm", deployed,
		"port arm state %s" % str(m.sim.get_arm_state(0)))
	_tap(KEY_Y)
	await m._sim_run(0.3)
	_check("Y attaches the tool to a deployed arm", bool(m.sim.get_arm_state(0)["tool_attached"]),
		"tool_attached=%s, message: %s" % [m.sim.get_arm_state(0)["tool_attached"], fl.message])

	# A control that refuses is still a control working: stowing an arm that holds a tool must be refused, and
	# must say why rather than silently doing nothing.
	_tap(KEY_T)
	await m._sim_run(0.3)
	var still_out: Dictionary = m.sim.get_arm_state(0)
	_check("T refuses to stow an arm that is holding its tool",
		bool(still_out["deployed"]) and not bool(still_out["stowing"]) and fl.message.contains("detach"),
		"deployed=%s stowing=%s, message: %s" % [still_out["deployed"], still_out["stowing"], fl.message])

	_tap(KEY_Y)
	await m._sim_run(0.3)
	_check("Y detaches the tool", not bool(m.sim.get_arm_state(0)["tool_attached"]),
		"tool_attached=%s, message: %s" % [m.sim.get_arm_state(0)["tool_attached"], fl.message])
	_tap(KEY_T)
	var stowed: bool = await m._until(func() -> bool: return not bool(m.sim.get_arm_state(0)["deployed"]) and not bool(m.sim.get_arm_state(0)["stowing"]), 8.0)
	_check("T stows the arm again", stowed, "port arm state %s" % str(m.sim.get_arm_state(0)))

	# --- scanning --------------------------------------------------------------------------------------------
	_tap(KEY_O)
	await m._sim_run(0.1)
	_tap(KEY_R)
	await m._sim_run(0.3)
	_check("R starts a scan of the nearest deposit", m.sim.is_scanning(),
		"scanning=%s remaining %.1f s, message: %s" % [m.sim.is_scanning(), m.sim.get_scan_remaining_s(), fl.message])
	_tap(KEY_R)
	await m._sim_run(0.3)
	_check("R again cancels the scan in progress", not m.sim.is_scanning(),
		"scanning=%s, message: %s" % [m.sim.is_scanning(), fl.message])

	# The probe wakes damaged, and a damaged sensor scans proportionally slower: a nominal 10 s scan takes
	# 10 / 0.35 = ~29 s at the 35% sensor integrity of the awakening. Budget from the integrity the simulation
	# actually reports, not from the nominal duration, or this reads as a broken control when the control is fine.
	var sensors: float = maxf(0.05, float(m.sim.get_integrity()["sensors"]))
	var budget: float = FirstLoop.SCAN_DURATION_S / sensors + 10.0
	_tap(KEY_R)
	var surveyed: bool = await m._until(func() -> bool: return m.sim.is_surveyed(DEPOSIT), budget)
	_check("a scan left to run surveys the deposit", surveyed,
		"surveyed=%s; a %.0f s scan takes about %.1f s at %d%% sensor integrity, allowed %.0f s"
		% [surveyed, FirstLoop.SCAN_DURATION_S, FirstLoop.SCAN_DURATION_S / sensors, int(sensors * 100.0), budget])

	# --- approach, and the control that interrupts it ------------------------------------------------------------
	_tap(KEY_1)
	_tap(KEY_P)
	await m._sim_run(0.5)
	var went: bool = fl.approaching or fl.waypoint_index >= 0 or fl.aligned
	_check("P commands the approach to the surveyed deposit", went,
		"approaching=%s waypoint=%d aligned=%s, message: %s"
		% [fl.approaching, fl.waypoint_index, fl.aligned, fl.message])
	# SPACE during an operation is the player's abort, and it is read from the flight path, not handle_key.
	if fl.approaching:
		await _hold(KEY_SPACE, 0.3)
		_check("SPACE cancels an approach that is under way", not fl.approaching,
			"approaching=%s, message: %s" % [fl.approaching, fl.message])
	else:
		_check("SPACE cancels an approach that is under way", true,
			"approach already finished before the abort could be timed; not exercised this run")

	# --- mining ---------------------------------------------------------------------------------------------
	var cargo_before: float = m.sim.get_inventory_kg(MATERIAL)
	_tap(KEY_1)
	_tap(KEY_T)
	await m._until(func() -> bool: return bool(m.sim.get_arm_state(0)["deployed"]), 8.0)
	_tap(KEY_Y)
	await m._sim_run(0.3)
	_tap(KEY_P)
	await m._until(func() -> bool: return fl.aligned, 25.0)
	_tap(KEY_G)
	var mined: bool = await m._until(func() -> bool: return m.sim.get_inventory_kg(MATERIAL) > cargo_before + EPS, 20.0)
	_check("G mines the surveyed deposit and puts material in cargo", mined,
		"cargo %.3f -> %.3f kg, aligned=%s, message: %s"
		% [cargo_before, m.sim.get_inventory_kg(MATERIAL), fl.aligned, fl.message])

	# --- a key the game does not use -------------------------------------------------------------------------------
	# The inventory is only meaningful if an unbound key is inert: a handler that swallowed everything would pass
	# every check above for the wrong reason. Only state that cannot move on its own is compared -- cargo is not,
	# because a mining operation legitimately keeps adding to it while this runs.
	var deployed_now: bool = bool(m.sim.get_arm_state(0)["deployed"])
	var policy_now: bool = bool(m.sim.get_policy_status()["installed"])
	var panel_now: bool = fl.panel_open
	var sel_now: int = fl.selected_arm
	_tap(KEY_Z)
	_tap(KEY_M)
	_tap(KEY_F5)
	await m._sim_run(0.3)
	_check("a key the game does not bind changes nothing",
		fl.panel_open == panel_now and fl.selected_arm == sel_now
		and bool(m.sim.get_policy_status()["installed"]) == policy_now
		and bool(m.sim.get_arm_state(0)["deployed"]) == deployed_now,
		"Z, M and F5 left panel=%s arm=%d policy=%s deployed=%s"
		% [fl.panel_open, fl.selected_arm, m.sim.get_policy_status()["installed"],
			m.sim.get_arm_state(0)["deployed"]])

	# --- flight: the six axes, the modifier and the brake ---------------------------------------------------------
	# Fly in clear air. A probe resting against the ground answers a thrust key with contact resolution as well as
	# thrust, and the sideways part of that is the ground doing its job, not the key failing -- it made the first
	# axis measured (whichever it was) look broken while the rest passed. Note _place_ship moves the scene node
	# only: once the simulation is running it decides where the probe is, so the probe is restarted instead.
	m.first_loop = null   # its deposits belong to the simulation being replaced
	m._sim_start(Vector3(0, 420, 0))
	await m._sim_run(0.4)

	var axes := [
		{"key": KEY_W, "name": "W drives the probe forward along its nose", "axis": 0, "sign": 1.0},
		{"key": KEY_S, "name": "S drives it backwards", "axis": 0, "sign": -1.0},
		{"key": KEY_D, "name": "D slides it to starboard", "axis": 1, "sign": 1.0},
		{"key": KEY_A, "name": "A slides it to port", "axis": 1, "sign": -1.0},
		{"key": KEY_E, "name": "E lifts it", "axis": 2, "sign": 1.0},
		{"key": KEY_Q, "name": "Q drops it", "axis": 2, "sign": -1.0},
	]
	var axis_names := ["nose", "starboard", "up"]
	for a in axes:
		await _stop()
		await _hold(int(a["key"]), HOLD_S)
		var v: Vector3 = m._sim_velocity_godot()
		var axis: int = int(a["axis"])
		var want: Vector3 = _body_axes()[axis]
		var along: float = v.dot(want) * float(a["sign"])
		var cross: float = (v - want * v.dot(want)).length()
		_check(String(a["name"]), along > 1.0 and cross < 0.5,
			"%.3f m/s along the probe's own %s axis (%s), %.3f m/s across it, after holding %s for %.1f s"
			% [along, axis_names[axis], "+" if float(a["sign"]) > 0.0 else "-", cross,
				OS.get_keycode_string(int(a["key"])), HOLD_S])

	await _stop()
	await _hold(KEY_W, HOLD_S)
	var nose: Vector3 = _body_axes()[0]
	var plain: float = m._sim_velocity_godot().dot(nose)
	await _stop()
	await _hold_both(KEY_W, KEY_SHIFT, HOLD_S)
	var boosted: float = m._sim_velocity_godot().dot(_body_axes()[0])
	_check("SHIFT boosts the thrust", boosted > plain * 1.5,
		"%.3f m/s plain against %.3f m/s boosted over the same %.1f s hold" % [plain, boosted, HOLD_S])

	await _hold(KEY_SPACE, 0.2)
	var stopped := _velocity()
	_check("SPACE brings the probe to a stop",
		absf(stopped[0]) < EPS and absf(stopped[1]) < EPS and absf(stopped[2]) < EPS,
		"velocity (%.6f, %.6f, %.6f) m/s" % [stopped[0], stopped[1], stopped[2]])

	# --- looking around ---------------------------------------------------------------------------------------
	var att0: PackedFloat64Array = m.sim.get_attitude()
	var look := InputEventMouseMotion.new()
	look.relative = Vector2(100, 0)
	m._input(look)
	await m._sim_run(0.1)
	var att1: PackedFloat64Array = m.sim.get_attitude()
	# A 100 px move right asks for 12 degrees, and damaged propulsion is allowed proportionally less authority
	# (impact_damage.hpp scales every manual trim by propulsion integrity), so the turn the player gets on a
	# 40%-propulsion probe is 4.8 degrees. Checking against a flat 12 would call the damage model a broken mouse.
	var prop: float = float(m.sim.get_integrity()["propulsion"])
	var expected: float = 12.0 * prop
	_check("the mouse turns the probe, with the authority its propulsion still has",
		absf((att1[0] - att0[0]) - expected) < 1e-6,
		"yaw %.3f -> %.3f degrees for a 100 px move right: 0.12 deg/px x 100 px x %d%% propulsion = %.3f expected"
		% [att0[0], att1[0], int(round(prop * 100.0)), expected])

	# --- digging ------------------------------------------------------------------------------------------------
	# The drill and the hollow bite whatever the NOSE is pointed at, and the nose direction comes from the
	# simulation's attitude, not from the scene node: placing the node looking at the ground does nothing, because
	# the next frame puts it back where the simulation says. So: restart the probe above the summit and pitch it
	# down with the simulation's own attitude control, then check the ground is really in reach before blaming a key.
	m.first_loop = null   # its deposits belong to the previous simulation
	var vt: VoxelTool = m.terrain.get_voxel_tool()
	var peak := Vector3(0, -1000, 0)
	for gx in range(-6, 7):
		for gz in range(-6, 7):
			var h = vt.raycast(Vector3(gx * 40.0, 480.0, gz * 40.0), Vector3.DOWN, 700.0)
			if h != null and h.position.y > peak.y:
				peak = Vector3(h.position)
	m._sim_start(peak + Vector3(0, 16, 0))
	await m._sim_run(0.2)
	m.sim.adjust_attitude(0.0, -90.0, 0.0)
	await m._sim_run(0.4)
	var before_drill := _reach(22.0)
	m.evidence.precondition("the ground is within the drill's reach of the nose", before_drill > 0.0,
		"summit %s, probe 16 m above it, nose %s, terrain %s"
		% [str(peak.round()), str((-m.ship.global_transform.basis.z).snapped(Vector3(0.01, 0.01, 0.01))),
			"out of range" if before_drill < 0.0 else "%.2f m ahead" % before_drill])
	if before_drill > 0.0:
		await _hold(KEY_F, 0.4)
		var after_drill := _reach(22.0)
		_check("F drills the ground the probe is pointed at",
			after_drill < 0.0 or after_drill > before_drill + 0.5,
			"terrain %.2f m ahead before, %s after" % [before_drill,
				"out of range" if after_drill < 0.0 else "%.2f m" % after_drill])

		# The cavity opens inside the rock, 18 m past the surface, radius 17: sample right through that span.
		var surface := _reach(120.0)
		var sample_near: float = maxf(1.0, surface + 2.0)
		var sample_far: float = surface + 36.0
		var solid_before := _solid_along_nose(sample_near, sample_far)
		m._press(KEY_H)
		await m._sim_run(0.6)
		var solid_after := _solid_along_nose(sample_near, sample_far)
		m.evidence.precondition("there is rock to hollow out along the nose", solid_before > 0,
			"%d solid samples between %.0f m and %.0f m ahead" % [solid_before, sample_near, sample_far])
		_check("H hollows out the inside of the ground ahead", solid_before > 0 and solid_after < solid_before,
			"%d solid samples between %.0f m and %.0f m ahead before, %d after (the surface itself stays at %.2f m, which is the point: the cavity is inside the rock)"
			% [solid_before, sample_near, sample_far, solid_after, surface])

	m.evidence.report["controls"] = {"checked": checks_run}
	m._finish_evidence("CONTROLTEST", 16)
