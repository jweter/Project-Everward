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
	quit(0)
