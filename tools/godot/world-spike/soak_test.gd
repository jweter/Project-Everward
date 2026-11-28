extends RefCounted
# Does the game still hold up after a long session? (#378 G12/G13.)
#
# Every other mode measures a few seconds. --visualtest watches memory for four. That is enough to catch a frame
# that never draws and nothing else: a leak, a drifting frame time, a node count that climbs every loop and a
# slow slide from 60 fps to 30 all look perfect in a four-second window. Everward is meant to be played for a long
# time, and this laptop has already killed a test run tonight for being short of memory, so the long run is the
# one measurement nobody had.
#
# So: play the real loop over and over for a chosen number of minutes, sampling every 20 s, and judge the TREND
# rather than any single sample. The checks are about drift, not absolutes --
#
#   * memory must not climb steadily (compare the last third of samples against the first)
#   * the live node count must come back down: a loop that leaks scene nodes grows without bound
#   * the frame rate of the final third must not be meaningfully worse than the first third
#   * no sample may stall outright
#
# -- --soaktest --minutes=10 (default 2, so the suite stays usable; the owner's unattended runs use longer).
# Negative control: --selftest-fail=leak allocates a growing list of nodes each loop and must be caught.

const FirstLoop = preload("res://first_loop.gd")
const MATERIAL := "iron_bearing_silicate_regolith"
const DEPOSIT := "ore-1"
const START_KG := 120.0
const SAMPLE_S := 20.0
const MB := 1048576.0

var m: Node3D
var samples: Array = []
var leaked: Array = []   # only with --selftest-fail=leak


func _init(game: Node3D) -> void:
	m = game


func _sample(at_s: float) -> Dictionary:
	var s := {
		"t_s": snappedf(at_s, 0.1),
		"fps": float(Engine.get_frames_per_second()),
		"static_mb": snappedf(OS.get_static_memory_usage() / MB, 0.1),
		"nodes": m.get_tree().get_node_count(),
		"orphans": Performance.get_monitor(Performance.OBJECT_ORPHAN_NODE_COUNT),
		"objects": Performance.get_monitor(Performance.OBJECT_COUNT),
		"loops": int(m.evidence.report.get("soak", {}).get("loops", 0)),
		"cargo_kg": snappedf(float(m.sim.get_inventory_kg(MATERIAL)), 0.01),
		"tick": int(m.sim.get_tick()),
	}
	samples.append(s)
	print("SOAK %6.1fs fps %5.1f  static %7.1f MB  nodes %5d  orphans %4d  objects %6d  loops %d"
		% [s["t_s"], s["fps"], s["static_mb"], s["nodes"], s["orphans"], s["objects"], s["loops"]])
	return s


# The mean of a field over a slice of the samples: trends are read from thirds, never from one reading.
func _mean(field: String, from_i: int, to_i: int) -> float:
	var total := 0.0
	var n := 0
	for i in range(maxi(0, from_i), mini(to_i, samples.size())):
		total += float(samples[i][field])
		n += 1
	return total / maxf(1.0, float(n))


# One pass of the real loop through the real keys: scan, survey, deploy, tool, approach, mine, stow.
func _one_loop() -> void:
	var fl = m.first_loop
	if not m.sim.is_surveyed(DEPOSIT):
		if not m.sim.is_scanning():
			m._press(KEY_O)
			m._press(KEY_R)
		return
	var st: Dictionary = m.sim.get_arm_state(0)
	if not bool(st["deployed"]) and not bool(st["deploying"]):
		m._press(KEY_1)
		m._press(KEY_T)
		return
	if bool(st["deployed"]) and not bool(st["tool_attached"]):
		m._press(KEY_Y)
		return
	if not fl.aligned and not fl.approaching:
		m._press(KEY_P)
		return
	if fl.aligned:
		m._press(KEY_G)
		m.evidence.report["soak"]["loops"] = int(m.evidence.report["soak"]["loops"]) + 1
		# The deposit runs out; refill it so the loop can keep running rather than idling at an empty rock. This is
		# scenario setup through the same add_deposit the first loop uses, not a shortcut through mining.
		if float(m.sim.get_world_deposit_remaining_kg(DEPOSIT)) <= 1.0:
			m._press(KEY_Y)   # tool off, arm free
			fl.add_deposit(DEPOSIT, fl.deposits[0]["center"], 3.0, MATERIAL, "Iron-bearing regolith", START_KG)


func run(minutes: float) -> void:
	m._begin_evidence("soaktest")
	m.evidence.report["soak"] = {"minutes_requested": minutes, "sample_s": SAMPLE_S, "loops": 0, "samples": samples}
	var bounded: bool = minutes > 0.0 and minutes <= 120.0
	m.evidence.precondition("a bounded run length", bounded, "%.2f minutes requested, allowed 0 to 120" % minutes)
	if not bounded:
		m._finish_evidence("SOAKTEST", 18)
		return
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
	m.evidence.precondition("a resource body to work", registered, "%s at 40 m" % DEPOSIT)
	if not registered:
		m._finish_evidence("SOAKTEST", 18)
		return
	await m._sim_run(0.5)

	var total_s := minutes * 60.0
	var elapsed := 0.0
	var next_sample := 0.0
	_sample(0.0)
	next_sample = SAMPLE_S
	while elapsed < total_s:
		await m._sim_run(0.5)
		elapsed += 0.5
		await _one_loop()
		if m.selftest_fail == "leak":
			# The negative control: hold on to nodes the loop should have released.
			for _i in 40:
				var n := Node3D.new()
				m.add_child(n)
				leaked.append(n)
		if elapsed >= next_sample:
			_sample(elapsed)
			next_sample += SAMPLE_S

	# --- the trends --------------------------------------------------------------------------------------------
	var n := samples.size()
	m.evidence.precondition("enough samples to read a trend", n >= 4,
		"%d samples over %.1f minutes at one per %.0f s" % [n, minutes, SAMPLE_S])
	if n < 4:
		m._finish_evidence("SOAKTEST", 18)
		return
	var third := maxi(1, int(n / 3))
	var mem_early := _mean("static_mb", 0, third)
	var mem_late := _mean("static_mb", n - third, n)
	var fps_early := _mean("fps", 0, third)
	var fps_late := _mean("fps", n - third, n)
	var nodes_early := _mean("nodes", 0, third)
	var nodes_late := _mean("nodes", n - third, n)
	var orphans_late := _mean("orphans", n - third, n)
	var loops: int = int(m.evidence.report["soak"]["loops"])

	var check := func(name: String, ok: bool, detail: String) -> void: m.evidence.check(name, ok, detail)

	# A leak shows as the late third holding materially more than the early third. Some growth is legitimate as
	# terrain streams in, so the bar is a proportion rather than any absolute figure.
	var mem_growth: float = mem_late - mem_early
	var mem_ratio: float = mem_late / maxf(1.0, mem_early)
	check.call("memory does not climb across a long session", mem_ratio <= 1.25,
		"static memory averaged %.1f MB in the first third and %.1f MB in the last (%+.1f MB, x%.3f) over %.1f minutes and %d loops"
		% [mem_early, mem_late, mem_growth, mem_ratio, minutes, loops])

	check.call("the scene does not accumulate nodes", nodes_late <= nodes_early * 1.25 + 50.0,
		"%.0f live nodes in the first third, %.0f in the last; %.0f orphaned at the end"
		% [nodes_early, nodes_late, orphans_late])

	check.call("nothing is left orphaned", orphans_late <= 50.0,
		"%.0f orphaned nodes (created and never freed) at the end of the run" % orphans_late)

	# Frame rate is judged against its own start, not a fixed target: this laptop's iGPU is the reference machine
	# and what matters is that a long session does not get worse.
	check.call("the frame rate does not decay over a long session", fps_late >= fps_early * 0.85,
		"%.1f fps average in the first third, %.1f in the last (%.0f%% of the start) on %s"
		% [fps_early, fps_late, 100.0 * fps_late / maxf(0.1, fps_early), RenderingServer.get_video_adapter_name()])

	var stalled := -1.0
	for s in samples:
		if float(s["fps"]) > 0.0 and float(s["fps"]) < 5.0:
			stalled = float(s["t_s"])
	check.call("no sample stalled", stalled < 0.0,
		"worst sample %.1f fps" % _worst_fps() if stalled < 0.0 else "stalled to under 5 fps at %.1f s" % stalled)

	check.call("the loop actually ran", loops >= 1,
		"%d complete scan-approach-mine loops in %.1f minutes; cargo %.2f kg"
		% [loops, minutes, m.sim.get_inventory_kg(MATERIAL)])

	m._finish_evidence("SOAKTEST", 18)


func _worst_fps() -> float:
	var worst := 1e9
	for s in samples:
		worst = minf(worst, float(s["fps"]))
	return worst
