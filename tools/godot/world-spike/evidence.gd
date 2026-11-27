# Evidence report for the headless acceptance runs (#378 G04, G12, G13 in Godot).
#
# Every --firstloop / --powertest / --hudtest run writes <out>/report.json next to metrics.txt with:
#   * exact build identity: git commit, branch, dirty state and dirty files of the checkout the project runs from,
#     and the MD5/size of the EverwardSim extension library this engine loads;
#   * run conditions: engine version and hash, renderer, headless or not, command line, host memory, start and end
#     times (UTC) and process memory at the start and the end;
#   * timed events: every key the run presses, every first-loop message, every simulation contact, each with wall
#     milliseconds since the start and the simulation time;
#   * preconditions and checks, each with its result and detail;
#   * a verdict: PASS, FAIL, or INCONCLUSIVE.
#
# INCONCLUSIVE means the run could not establish its own situation, so it claims nothing about the game: the exact
# build is unknown, or a precondition (the scenario's starting state, set up by the harness) failed. A failed check
# after all preconditions hold is a FAIL, i.e. a finding about the game. This keeps harness failures apart from
# source defects (#378 G12). A verified claim needs PASS with build.known true and build.dirty false.
extends RefCounted

const SCHEMA := "everward.godot.evidence/1"
const LIBRARY := "../sim-binding/project/bin/everward_sim.windows.template_%s.x86_64.dll"
const MAX_EVENTS := 4000

var mode := ""
var started_msec := 0
var report := {}
var events: Array = []
var dropped_events := 0
var preconditions: Array = []
var checks: Array = []
var verdict := ""
var reasons: Array = []
var sim_tick := Callable()   # returns the simulation tick (microseconds), or -1 before the simulation runs

func _init(run_mode: String, args: PackedStringArray, simulate_unknown_build := false) -> void:
	mode = run_mode
	started_msec = Time.get_ticks_msec()
	var v := Engine.get_version_info()
	report = {
		"schema": SCHEMA,
		"mode": mode,
		"args": Array(args),
		"started_utc": Time.get_datetime_string_from_system(true),
		"engine": {"version": v["string"], "hash": v.get("hash", ""), "debug_build": OS.is_debug_build(),
			"renderer": RenderingServer.get_video_adapter_name(), "display": DisplayServer.get_name()},
		"build": _build_identity(simulate_unknown_build),
		"extension": _extension_identity(),
		"host": {"os": OS.get_name(), "processors": OS.get_processor_count(), "memory": OS.get_memory_info()},
		"memory_start": _process_memory(),
	}

func event(kind: String, detail: String) -> void:
	if events.size() >= MAX_EVENTS:
		dropped_events += 1
		return
	var tick: int = int(sim_tick.call()) if sim_tick.is_valid() else -1
	events.append({"t_ms": Time.get_ticks_msec() - started_msec, "sim_s": tick / 1.0e6 if tick >= 0 else null,
		"kind": kind, "detail": detail})

func precondition(name: String, ok: bool, detail: String) -> void:
	preconditions.append({"name": name, "ok": ok, "detail": detail})
	event("precondition", "%s %s" % ["PASS" if ok else "FAIL", name])

func check(name: String, ok: bool, detail: String) -> void:
	checks.append({"name": name, "ok": ok, "detail": detail})
	event("check", "%s %s" % ["PASS" if ok else "FAIL", name])

# Lines for metrics.txt and the console, in the order the run recorded them.
func lines() -> Array[String]:
	var out: Array[String] = []
	for p in preconditions:
		out.append("%s precondition %s: %s" % ["PASS" if p["ok"] else "FAIL", p["name"], p["detail"]])
	for c in checks:
		out.append("%s %s: %s" % ["PASS" if c["ok"] else "FAIL", c["name"], c["detail"]])
	return out

func summary() -> String:
	return verdict if reasons.is_empty() else "%s %s" % [verdict, ", ".join(reasons)]

# Decide the verdict and write report.json. Unknown build or a failed precondition makes the run INCONCLUSIVE even
# when checks failed too (they are still listed), because nothing can be attributed to the game.
func finish(out_dir: String, extra_inconclusive := "") -> String:
	reasons.clear()
	var failed_pre: Array = preconditions.filter(func(p: Dictionary) -> bool: return not p["ok"])
	var failed: Array = checks.filter(func(c: Dictionary) -> bool: return not c["ok"])
	if not report["build"]["known"]:
		reasons.append("build identity unknown")
	if extra_inconclusive != "":
		reasons.append(extra_inconclusive)
	for p in failed_pre:
		reasons.append("precondition " + String(p["name"]))
	if not reasons.is_empty():
		verdict = "INCONCLUSIVE"
	elif not failed.is_empty() or checks.is_empty():
		verdict = "FAIL"
		reasons = failed.map(func(c: Dictionary) -> String: return String(c["name"])) if not failed.is_empty() else ["no checks ran"]
	else:
		verdict = "PASS"
	report["finished_utc"] = Time.get_datetime_string_from_system(true)
	report["duration_ms"] = Time.get_ticks_msec() - started_msec
	report["memory_end"] = _process_memory()
	report["preconditions"] = preconditions
	report["checks"] = checks
	report["events"] = events
	report["dropped_events"] = dropped_events
	report["verdict"] = verdict
	report["reasons"] = reasons
	# No report, no result (Codex #451): a run whose evidence cannot be written is INCONCLUSIVE, never a PASS, even if
	# an older report.json is left behind.
	var path := out_dir.path_join("report.json")
	var f := FileAccess.open(path, FileAccess.WRITE)
	var written := false
	if f != null:
		f.store_string(JSON.stringify(report, "  ", false))
		written = f.get_error() == OK
		f.close()
	if not written:
		push_error("could not write " + path)
		verdict = "INCONCLUSIVE"
		reasons = ["report.json not written (%s)" % error_string(FileAccess.get_open_error() if f == null else ERR_FILE_CANT_WRITE)] + reasons
		report["verdict"] = verdict
		report["reasons"] = reasons
	return verdict

func _process_memory() -> Dictionary:
	return {"static_bytes": OS.get_static_memory_usage(), "static_peak_bytes": OS.get_static_memory_peak_usage()}

func _git(args: Array) -> Dictionary:
	var out := []
	var code := OS.execute("git", ["-C", ProjectSettings.globalize_path("res://")] + args, out, false)
	return {"code": code, "text": (String(out[0]) if out.size() > 0 else "").strip_edges()}

# Fail closed: no git, a failed command or a malformed answer means the build is unknown; a failed status is dirty.
func _build_identity(simulate_unknown: bool) -> Dictionary:
	var head: Dictionary = {"code": -1, "text": ""} if simulate_unknown else _git(["rev-parse", "HEAD"])
	var commit: String = head["text"]
	var known: bool = head["code"] == 0 and commit.length() == 40 and commit.is_valid_hex_number()
	var b := {"known": known, "commit": commit if known else ""}
	if not known:
		return b
	var st := _git(["status", "--porcelain", "--untracked-files=normal"])
	var files: Array = Array(String(st["text"]).split("\n", false))
	b["dirty"] = st["code"] != 0 or not files.is_empty()
	b["dirty_files"] = files.slice(0, 50)
	b["dirty_file_count"] = files.size()
	b["branch"] = _git(["rev-parse", "--abbrev-ref", "HEAD"])["text"]
	return b

func _extension_identity() -> Dictionary:
	var path := ProjectSettings.globalize_path("res://").path_join(LIBRARY % ("debug" if OS.is_debug_build() else "release")).simplify_path()
	var e := {"loaded": ClassDB.class_exists("EverwardSim"), "library": path, "exists": FileAccess.file_exists(path)}
	if e["exists"]:
		e["md5"] = FileAccess.get_md5(path)
		var f := FileAccess.open(path, FileAccess.READ)
		if f != null:
			e["size_bytes"] = f.get_length()
			f.close()
	return e
