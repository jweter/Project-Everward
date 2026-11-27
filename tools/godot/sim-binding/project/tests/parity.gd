extends SceneTree
# Godot side of the G1 parity check: drives the real Everward simulation through the GDExtension and
# prints exactly the lines the native reference (parity_scenario.hpp) prints. The two outputs must match.

func _init() -> void:
	if not ClassDB.class_exists("EverwardSim"):
		print("PARITY_ERROR EverwardSim class not registered (extension did not load)")
		quit(2)
		return
	var sim = ClassDB.instantiate("EverwardSim")
	sim.set_velocity(1.5, 0.0, -0.5)
	sim.advance_ticks(600000)
	var p: PackedFloat64Array = sim.get_position()
	print("tick %d pos %.9f %.9f %.9f" % [sim.get_tick(), p[0], p[1], p[2]])
	sim.setup_bootstrap_deposit(250.0)
	for i in 6:
		var r: Dictionary = sim.mine_once()
		print("mine %d accepted %d extracted %.9f remaining %.9f" % [i, 1 if r["accepted"] else 0, r["extracted_kg"], r["remaining_kg"]])
	print("after6 inventory %.9f extracted %.9f remaining %.9f" % [sim.get_inventory_kg("iron_regolith"), sim.get_extracted_kg(), sim.get_deposit_remaining_kg()])
	var save: String = sim.save_json()
	for i in 4:
		sim.mine_once()
	print("after10 inventory %.9f remaining %.9f" % [sim.get_inventory_kg("iron_regolith"), sim.get_deposit_remaining_kg()])
	print("load %d" % (1 if sim.load_json(save) else 0))
	print("loaded tick %d inventory %.9f extracted %.9f remaining %.9f" % [sim.get_tick(), sim.get_inventory_kg("iron_regolith"), sim.get_extracted_kg(), sim.get_deposit_remaining_kg()])
	for i in 2:
		sim.mine_once()
	print("final inventory %.9f extracted %.9f remaining %.9f" % [sim.get_inventory_kg("iron_regolith"), sim.get_extracted_kg(), sim.get_deposit_remaining_kg()])
	print("bad load %d" % (1 if sim.load_json("{not json") else 0))
	print("after bad load inventory %.9f" % sim.get_inventory_kg("iron_regolith"))

	# Review regressions (#437), same steps as parity_scenario.hpp
	var cap = ClassDB.instantiate("EverwardSim")
	cap.setup_bootstrap_deposit(600.0)
	var accepted := 0
	for i in 200:
		if not cap.mine_once()["accepted"]:
			break
		accepted += 1
	print("capacity accepted %d storage %.9f of %.9f remaining %.9f" % [accepted, cap.get_storage_used_kg(), cap.get_storage_capacity_kg(), cap.get_deposit_remaining_kg()])
	var pre = ClassDB.instantiate("EverwardSim")
	var before_deposit: String = pre.save_json()
	pre.setup_bootstrap_deposit(250.0)
	pre.mine_once()
	var pl := 1 if pre.load_json(before_deposit) else 0
	var hd := 1 if pre.has_deposit() else 0
	var inv: float = pre.get_inventory_kg("iron_regolith")
	var rem: float = pre.get_deposit_remaining_kg()
	var ext: float = pre.get_extracted_kg()
	var after := 1 if pre.mine_once()["accepted"] else 0
	print("predeposit load %d has_deposit %d inventory %.9f remaining %.9f extracted %.9f mine_after %d" % [pl, hd, inv, rem, ext, after])
	var bad: String = sim.save_json()
	var key := "\"mass_kg\": "
	var at := bad.find(key)
	if at != -1:
		var start := at + key.length()
		var stop := start
		while stop < bad.length() and not (bad[stop] in [",", "\n", "}"]):
			stop += 1
		bad = bad.substr(0, start) + "-1" + bad.substr(stop)
	print("invalid mass load %d inventory %.9f" % [1 if sim.load_json(bad) else 0, sim.get_inventory_kg("iron_regolith")])
	var cmd = ClassDB.instantiate("EverwardSim")
	var adv := 1 if cmd.advance_ticks(-1) else 0
	var vel := 1 if cmd.set_velocity(NAN, 0.0, 0.0) else 0
	var dep := 1 if cmd.setup_bootstrap_deposit(-1.0) else 0
	var mined := 1 if cmd.mine_once()["accepted"] else 0
	print("invalid commands advance %d velocity %d deposit %d mine %d tick %d" % [adv, vel, dep, mined, cmd.get_tick()])

	var fly = ClassDB.instantiate("EverwardSim")
	var att := 1 if fly.adjust_attitude(30.0, 10.0, 0.0) else 0
	var a: PackedFloat64Array = fly.get_attitude()
	print("attitude ok %d yaw %.9f pitch %.9f roll %.9f" % [att, a[0], a[1], a[2]])
	fly.adjust_attitude(-30.0, -10.0, 0.0)
	var thr := 1 if fly.adjust_local_velocity(10.0, 0.0, 0.0) else 0
	var v: PackedFloat64Array = fly.get_velocity()
	print("thrust ok %d velocity %.9f %.9f %.9f" % [thr, v[0], v[1], v[2]])
	var body := 1 if fly.add_sphere_body("rock-1", 30.0, 0.0, 0.0, 3.0) else 0
	var bad_body := 1 if fly.add_sphere_body("", 0.0, 0.0, 0.0, 1.0) else 0
	var contacts := 0
	for step in 50:
		fly.advance_ticks(100000)
		contacts += fly.drain_contact_events()
	var c: Dictionary = fly.get_last_contact()
	var fp: PackedFloat64Array = fly.get_position()
	print("flight body %d bad_body %d contacts %d has %d id %s normal_speed %.9f pos %.9f %.9f %.9f" % [body, bad_body, contacts, 1 if c["has_contact"] else 0, c["body_id"], c["normal_speed"], fp[0], fp[1], fp[2]])

	var mv = ClassDB.instantiate("EverwardSim")
	mv.add_sphere_body("rock-far", 100.0, 0.0, 0.0, 3.0)
	var moved := 1 if mv.move_sphere_body("rock-far", 30.0, 0.0, 0.0) else 0
	var moved_unknown := 1 if mv.move_sphere_body("nope", 1.0, 0.0, 0.0) else 0
	mv.adjust_local_velocity(10.0, 0.0, 0.0)
	var mv_contacts := 0
	for step in 50:
		mv.advance_ticks(100000)
		mv_contacts += mv.drain_contact_events()
	var mp: PackedFloat64Array = mv.get_position()
	print("moved body %d unknown %d contacts %d pos %.9f" % [moved, moved_unknown, mv_contacts, mp[0]])

	var ore := "ore-1"
	var mat := "iron_bearing_silicate_regolith"
	var fl = ClassDB.instantiate("EverwardSim")
	var fl_dep := 1 if fl.add_deposit(ore, 40.0, 0.0, 0.0, 2.0, mat, "Iron-bearing regolith", 120.0, 5.0) else 0
	var dup := 1 if fl.add_deposit(ore, 0.0, 0.0, 0.0, 1.0, mat, "", 1.0, 1.0) else 0
	var early: Dictionary = fl.mine_deposit(ore)
	print("deposit %d dup %d early %d reason %s" % [fl_dep, dup, 1 if early["accepted"] else 0, early["reason"]])
	var scan := 1 if fl.start_scan(ore, 10.0) else 0
	for step in 105:
		fl.advance_ticks(100000)
	print("scan %d surveyed %d scanning %d" % [scan, 1 if fl.is_surveyed(ore) else 0, 1 if fl.is_scanning() else 0])
	var rescan := 1 if fl.start_scan(ore, 10.0) else 0
	fl.advance_ticks(1000000)
	var cancelled := 1 if fl.cancel_scan() else 0
	print("rescan %d cancelled %d surveyed %d" % [rescan, cancelled, 1 if fl.is_surveyed(ore) else 0])
	var joint_early := 1 if fl.set_joint_targets(0, 10.0, 10.0, 0.0) else 0
	var toggle := 1 if fl.toggle_arm(0) else 0
	for step in 22:
		fl.advance_ticks(100000)
	var tool := 1 if fl.attach_tool(0) else 0
	var arm: Dictionary = fl.get_arm_state(0)
	print("joint_early %d toggle %d tool %d deployed %d accepts %d fraction %.9f" % [joint_early, toggle, tool, 1 if arm["deployed"] else 0, 1 if arm["accepts_joint_commands"] else 0, arm["deployment_fraction"]])
	var far: Dictionary = fl.mine_deposit(ore)
	print("far %d gap %.9f reason %s" % [1 if far["accepted"] else 0, far["tool_gap_m"], far["reason"]])
	var plan: Dictionary = fl.plan_staging(0, ore)
	var pp: PackedFloat64Array = plan["position"]
	var pa: PackedFloat64Array = plan["angles"]
	print("staging %d pos %.9f %.9f %.9f angles %.3f %.3f %.3f gap %.9f" % [1 if plan["has_solution"] else 0, pp[0], pp[1], pp[2], pa[0], pa[1], pa[2], plan["tool_gap_m"]])
	var approach_steps := 0
	var dist := 0.0
	while approach_steps < 600:
		var cur: PackedFloat64Array = fl.get_position()
		var dx: float = pp[0] - cur[0]
		var dy: float = pp[1] - cur[1]
		var dz: float = pp[2] - cur[2]
		dist = sqrt(dx * dx + dy * dy + dz * dz)
		if dist <= 0.25:
			fl.set_velocity(0.0, 0.0, 0.0)
			break
		var speed: float = minf(2.0, maxf(0.25, dist))
		fl.set_velocity(dx / dist * speed, dy / dist * speed, dz / dist * speed)
		fl.advance_ticks(100000)
		approach_steps += 1
	print("approach steps %d dist %.9f contacts %d" % [approach_steps, dist, fl.drain_contact_events()])
	var path: Dictionary = fl.plan_arm_path(0, pa[0], pa[1], pa[2])
	var reached := 0
	var waypoints: Array = path["waypoints"]
	for w in waypoints:
		fl.set_joint_targets(0, w[0], w[1], w[2])
		for step in 100:
			var ja: PackedFloat64Array = fl.get_arm_state(0)["angles"]
			if absf(ja[0] - w[0]) < 1e-6 and absf(ja[1] - w[1]) < 1e-6 and absf(ja[2] - w[2]) < 1e-6:
				reached += 1
				break
			fl.advance_ticks(100000)
	var posed: PackedFloat64Array = fl.get_arm_state(0)["angles"]
	print("path %d waypoints %d reached %d angles %.6f %.6f %.6f" % [1 if path["ok"] else 0, waypoints.size(), reached, posed[0], posed[1], posed[2]])
	var fl_mined: Dictionary = fl.mine_deposit(ore)
	print("mine %d extracted %.9f remaining %.9f gap %.9f arm %d material %s" % [1 if fl_mined["accepted"] else 0, fl_mined["extracted_kg"], fl_mined["remaining_kg"], fl_mined["tool_gap_m"], fl_mined["arm"], fl_mined["material_id"]])
	print("inventory %.9f storage %.9f deposit %.9f" % [fl.get_inventory_kg(mat), fl.get_storage_used_kg(), fl.get_world_deposit_remaining_kg(ore)])

	var fl_save: String = fl.save_json()
	var after_save: Dictionary = fl.mine_deposit(ore)
	var extra := 1 if fl.add_deposit("ore-2", -30.0, 12.0, 0.0, 2.0, "nickel_regolith", "Nickel regolith", 80.0, 4.0) else 0
	var fl_load := 1 if fl.load_json(fl_save) else 0
	print("g08 after_save %d extra %d load %d surveyed %d deposit %.9f inventory %.9f ore2 %.3f" % [1 if after_save["accepted"] else 0, extra, fl_load, 1 if fl.is_surveyed(ore) else 0, fl.get_world_deposit_remaining_kg(ore), fl.get_inventory_kg(mat), fl.get_world_deposit_remaining_kg("ore-2")])
	var resumed: Dictionary = fl.mine_deposit(ore)
	print("g08 resumed %d deposit %.9f inventory %.9f" % [1 if resumed["accepted"] else 0, fl.get_world_deposit_remaining_kg(ore), fl.get_inventory_kg(mat)])
	var blank = ClassDB.instantiate("EverwardSim")
	var blank_load := 1 if fl.load_json(blank.save_json()) else 0
	print("g08 blank load %d ore1 %.3f surveyed %d" % [blank_load, fl.get_world_deposit_remaining_kg(ore), 1 if fl.is_surveyed(ore) else 0])

	var fx = ClassDB.instantiate("EverwardSim")
	fx.enable_fix_it(true)
	var awake: Dictionary = fx.get_integrity()
	print("fixit awake computation %.3f thermal %.3f sensors %.3f propulsion %.3f" % [awake["computation"], awake["thermal"], awake["sensors"], awake["propulsion"]])
	for step in 50:
		fx.advance_ticks(100000)
	var idle: Dictionary = fx.get_fix_it_status()
	print("fixit idle state %s subsystem %s need %.6f kg computation %.9f" % [idle["state"], idle["subsystem"], idle["material_required_kg"], fx.get_integrity()["computation"]])
	fx.setup_bootstrap_deposit(250.0)
	var fuel: Dictionary = fx.mine_once()
	var energy0: float = fx.get_stored_energy_j()
	print("fixit mined %d storage %.9f" % [1 if fuel["accepted"] else 0, fx.get_storage_used_kg()])
	for step in 200:
		fx.advance_ticks(100000)
	var fixed: Dictionary = fx.get_fix_it_status()
	var healed: Dictionary = fx.get_integrity()
	print("fixit after state %s subsystem %s computation %.9f thermal %.9f storage %.9f inventory %.9f energy_used %.3f" % [fixed["state"], fixed["subsystem"], healed["computation"], healed["thermal"], fx.get_storage_used_kg(), fx.get_inventory_kg("iron_regolith"), energy0 - fx.get_stored_energy_j()])
	var fix_events: PackedStringArray = fx.drain_fix_it_events()
	print("fixit events %d first %s" % [fix_events.size(), fix_events[1] if fix_events.size() > 1 else ""])
	print("fixit total consumed %.9f storage debit %.9f" % [fx.get_fix_it_status()["total_material_consumed_kg"], 5.0 - fx.get_storage_used_kg()])

	# Review regression (#444): a successful load discards any in-flight Fix_It executor.
	var load_fx = ClassDB.instantiate("EverwardSim")
	load_fx.enable_fix_it(true)
	load_fx.setup_bootstrap_deposit(250.0)
	load_fx.mine_once()
	load_fx.advance_ticks(100000)
	var repair_was_running: bool = load_fx.get_fix_it_status()["state"] == "repairing"
	var pristine_fx = ClassDB.instantiate("EverwardSim")
	pristine_fx.setup_bootstrap_deposit(250.0)
	pristine_fx.mine_once()
	var fix_load := 1 if load_fx.load_json(pristine_fx.save_json()) else 0
	load_fx.advance_ticks(100000)
	var after_fix_load: Dictionary = load_fx.get_fix_it_status()
	var loaded_computation: float = load_fx.get_integrity()["computation"]
	var load_reset_ok: bool = repair_was_running and fix_load == 1 and absf(loaded_computation - 1.0) < 1e-12 and after_fix_load["state"] == "monitor"
	if not load_reset_ok:
		print("PARITY_ERROR Fix_It executor survived load_json")
		quit(9)
		return
	print("fixit load reset %d computation %.9f state %s" % [1 if load_reset_ok else 0, loaded_computation, after_fix_load["state"]])
	var pw = ClassDB.instantiate("EverwardSim")
	pw.add_deposit("ore-p", 30.0, 0.0, 0.0, 2.0, "iron_regolith", "", 50.0, 5.0)
	var pol := 1 if pw.install_basic_survival_policy() else 0
	pw.advance_ticks(100000)
	var p1: PackedFloat64Array = pw.get_power_allocations()
	var scan1 := 1 if pw.start_scan("ore-p", 10.0) else 0
	print("g09 policy %d sensors %.3f scan %d guidance %s" % [pol, p1[0], scan1, pw.get_scan_power_guidance()["compact"]])
	pw.allocate_power(0, 50.0)
	pw.advance_ticks(100000)
	var scan2 := 1 if pw.start_scan("ore-p", 10.0) else 0
	print("g09 raised-with-policy sensors %.3f scan %d" % [pw.get_power_allocations()[0], scan2])
	var cleared := 1 if pw.clear_policy() else 0
	var cleared_again := 1 if pw.clear_policy() else 0
	pw.allocate_power(0, 50.0)
	pw.advance_ticks(100000)
	var scan3 := 1 if pw.start_scan("ore-p", 10.0) else 0
	for step in 105:
		pw.advance_ticks(100000)
	print("g09 cleared %d again %d sensors %.3f scan %d surveyed %d guidance %s" % [cleared, cleared_again, pw.get_power_allocations()[0], scan3, 1 if pw.is_surveyed("ore-p") else 0, "ready" if pw.get_scan_power_guidance()["state"] == "ready" else "below"])
	var hs = ClassDB.instantiate("EverwardSim")
	print("g11 sel %d %d %d %d %d %d" % [hs.bounded_selection(5, 1, 6), hs.bounded_selection(0, -1, 6), hs.bounded_selection(3, 10, 6), hs.bounded_selection(2, -1, 6), hs.bounded_selection(4, 1, 0), hs.bounded_selection(0, -3, 0)])
	quit(0)
