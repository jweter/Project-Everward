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
	quit(0)
