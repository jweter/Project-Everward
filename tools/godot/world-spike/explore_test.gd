extends RefCounted
# Seeded, reproducible, game-command exploratory acceptance test.
# Always drives main.gd's real key handler; never edits simulation state to fabricate success.
# Input trace and all failures are written to evidence.gd's report.json.

const FirstLoop = preload("res://first_loop.gd")
const MATERIAL := "iron_bearing_silicate_regolith"
const DEPOSIT := "ore-1"
const START_KG := 120.0
const MAX_STEPS := 64
const ACTIONS := [
	KEY_R, KEY_G, KEY_T, KEY_Y, KEY_1, KEY_2, KEY_K, KEY_L, KEY_O,
	KEY_TAB, KEY_BRACKETLEFT, KEY_BRACKETRIGHT
]

var m: Node3D

func _init(game: Node3D) -> void:
	m = game

func _stock() -> Dictionary:
	var repair: Dictionary = m.sim.get_fix_it_status()
	return {
		"deposit": float(m.sim.get_world_deposit_remaining_kg(DEPOSIT)),
		"cargo": float(m.sim.get_inventory_kg(MATERIAL)),
		"consumed": float(repair.get("total_material_consumed_kg", -1.0)),
		"storage": float(m.sim.get_storage_used_kg()),
		"capacity": float(m.sim.get_storage_capacity_kg()),
		"energy": float(m.sim.get_stored_energy_j()),
		"tick": int(m.sim.get_tick())
	}

func _invariant(before: Dictionary, after: Dictionary) -> bool:
	var mass_ok: bool = (
		is_finite(after["deposit"]) and is_finite(after["cargo"]) and
		is_finite(after["consumed"]) and
		absf(after["deposit"] + after["cargo"] + after["consumed"] - START_KG) < 1e-6
	)
	var bounds_ok: bool = (
		after["deposit"] >= -1e-8 and after["cargo"] >= -1e-8 and
		after["consumed"] >= -1e-8 and after["storage"] >= -1e-8 and
		after["storage"] <= after["capacity"] + 1e-8 and
		absf(after["storage"] - after["cargo"]) < 1e-8 and
		is_finite(after["energy"]) and after["energy"] >= -1e-8
	)
	var monotonic: bool = (
		after["tick"] >= before["tick"] and
		after["deposit"] <= before["deposit"] + 1e-8 and
		after["consumed"] >= before["consumed"] - 1e-8
	)
	var health: Dictionary = m.sim.get_integrity()
	var valid_health := true
	for subsystem in ["sensors", "propulsion", "computation", "thermal"]:
		var v: float = float(health[subsystem])
		valid_health = valid_health and is_finite(v) and v >= 0.0 and v <= 1.0
	return mass_ok and bounds_ok and monotonic and valid_health

func run(test_seed: int, steps: int) -> void:
	m._begin_evidence("exploretest")
	m.evidence.report["exploration"] = {"seed": test_seed, "steps_requested": steps, "actions": []}
	m.evidence.precondition("seed and bounded action count",
		test_seed >= 0 and steps >= 1 and steps <= MAX_STEPS,
		"seed=%d steps=%d, allowed 1..%d" % [test_seed, steps, MAX_STEPS])
	if test_seed < 0 or steps < 1 or steps > MAX_STEPS:
		m._finish_evidence("EXPLORETEST", 15)
		return

	# Same scene and simulation-authorized starting scenario used by the regular gameplay tests.
	var origin := Vector3(0, 300, 0)
	m._place_ship(origin, origin + Vector3(0, 0, -100))
	await m._wait_stream()
	if not m._sim_start(origin):
		m._fail("simulation extension (EverwardSim) not loaded", 5)
		return
	m.first_loop = FirstLoop.new(m)
	m.sim.enable_fix_it(true)
	m.sim_contact_log.clear()
	var registered: bool = m.first_loop.add_deposit(
		DEPOSIT, origin + Vector3(0, 0, -40), 3.0, MATERIAL, "Iron-bearing regolith", START_KG)
	m.evidence.precondition("exploration resource registered", registered,
		"%s at 40 m, %.1f kg" % [DEPOSIT, START_KG])
	if not registered:
		m._finish_evidence("EXPLORETEST", 15)
		return
	await m._sim_run(0.2)

	# Invalid and interrupted operations are intentional; only broken mechanical invariants fail.
	# Each seed deterministically selects the same sequence and sim delay, making failures replayable.
	var rng := RandomNumberGenerator.new()
	rng.seed = test_seed
	var trace: Array = m.evidence.report["exploration"]["actions"]
	var valid := true
	for i in steps:
		var keycode: int = ACTIONS[rng.randi_range(0, ACTIONS.size() - 1)]
		var wait_s := float(rng.randi_range(1, 3)) * 0.1
		var before := _stock()
		m._press(keycode)
		await m._sim_run(wait_s)
		var after := _stock()
		var ok := _invariant(before, after)
		# The negative control corrupts ONLY the test verdict, never game state or cargo.
		if m.selftest_fail == "explore-invariant" and i == 2:
			ok = false
		var name := OS.get_keycode_string(keycode)
		trace.append({"index": i, "key": name, "wait_s": wait_s, "tick": after["tick"],
			"deposit_kg": after["deposit"], "cargo_kg": after["cargo"],
			"repair_consumed_kg": after["consumed"], "ok": ok,
			"message": String(m.first_loop.message)})
		m.evidence.event("explore", "seed=%d step=%d key=%s ok=%s" % [test_seed, i, name, ok])
		m.evidence.check("seed %d action %02d %s conserves state" % [test_seed, i, name], ok,
			"deposit %.3f cargo %.3f Fix_It %.3f kg, storage %.3f kg, tick %d, message: %s" %
			[after["deposit"], after["cargo"], after["consumed"], after["storage"],
				after["tick"], m.first_loop.message])
		valid = valid and ok

	# Recover using player commands, not a fresh simulation or direct inventory mutation.
	# An arbitrary prelude may have left the Systems panel open or a scan in progress.
	if m.first_loop.panel_open:
		m._press(KEY_TAB)
	m._press(KEY_L)
	m._press(KEY_O)
	if m.sim.is_scanning():
		m._press(KEY_R)
	if not m.sim.is_surveyed(DEPOSIT):
		m._press(KEY_R)
	var surveyed: bool = await m._until(
		func() -> bool: return m.sim.is_surveyed(DEPOSIT), 45.0)
	m.evidence.check("recovery scans deposit after seeded input", surveyed,
		"surveyed=%s; %s" % [surveyed, m.first_loop.message])
	m._press(KEY_1)
	await m._until(func() -> bool:
		var arm: Dictionary = m.sim.get_arm_state(0)
		return not arm["deploying"] and not arm["stowing"], 6.0)
	if not bool(m.sim.get_arm_state(0)["deployed"]):
		m._press(KEY_T)
	await m._until(func() -> bool:
		var arm: Dictionary = m.sim.get_arm_state(0)
		return arm["deployed"] and not arm["deploying"], 6.0)
	if not bool(m.sim.get_arm_state(0)["tool_attached"]):
		m._press(KEY_Y)
	var arm_ready: bool = bool(m.sim.get_arm_state(0)["deployed"]) and (
		bool(m.sim.get_arm_state(0)["tool_attached"]))
	m.evidence.check("recovery arms real tool", arm_ready, m.first_loop.message)

	# Interrupt a legal approach, then resume it; check player recoverability.
	if surveyed and arm_ready:
		m._press(KEY_P)
		var engaged: bool = m.first_loop.approaching
		if engaged:
			await m._sim_run(0.3)
			m._press(KEY_P)
		var canceled: bool = engaged and not m.first_loop.approaching
		m.evidence.check("interrupt and cancel a player approach", canceled, m.first_loop.message)
		m._press(KEY_P)
		var settled: bool = await m._until(func() -> bool:
			return m.first_loop.aligned or (
				not m.first_loop.busy() and not m.first_loop.approaching), 100.0)
		var aligned: bool = m.first_loop.aligned
		var before_mine: Dictionary = _stock()
		m._press(KEY_G)
		await m._sim_run(0.2)
		var after_mine: Dictionary = _stock()
		var accepted: bool = bool(m.first_loop.last_mine.get("accepted", false))
		m.evidence.check("seeded recovery returns to real mining",
			settled and aligned and accepted and after_mine["cargo"] > before_mine["cargo"],
			"settled=%s aligned=%s accepted=%s, cargo +%.3f kg; %s" %
			[settled, aligned, accepted, after_mine["cargo"] - before_mine["cargo"], m.first_loop.message])
		m.evidence.check("seeded mining maintains independent material balance",
			_invariant(before_mine, after_mine), "deposit %.3f + cargo %.3f + repair %.3f = %.3f" %
			[after_mine["deposit"], after_mine["cargo"], after_mine["consumed"], START_KG])
	else:
		m.evidence.check("seeded recovery reaches mining", false,
			"seed=%d, surveyed=%s, arm_ready=%s" % [test_seed, surveyed, arm_ready])

	m.evidence.check("no hull collision during exploratory recovery",
		m.sim_contact_log.is_empty(), "contact bodies: %s" % [m.sim_contact_log])
	await m._sim_run(0.2)
	m.evidence.check("HUD reflects the selected arm and resources",
		m.hud.text.contains("ARM PORT") and m.hud.text.contains("CARGO") and
		m.hud.text.contains("Iron-bearing regolith") and m.hud.text.contains("FIX_IT //"),
		m.hud.text.replace("\n", " | "))
	m.evidence.report["exploration"]["actions_recorded"] = trace.size()
	m.evidence.report["exploration"]["invariants_all_passed"] = valid
	m._finish_evidence("EXPLORETEST", 15)
