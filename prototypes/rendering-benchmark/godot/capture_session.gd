extends Node

const HANDOFF_PATH := "res://benchmark_handoff.json"
const OUTPUT_PATH := "user://godot_capture_observation.json"
const WARMUP_SECONDS := 5.0

var handoff: Dictionary = {}
var warmup_elapsed := 0.0
var capture_elapsed := 0.0
var capturing := false
var completed := false
var cpu_frame_samples_ms: Array[float] = []
var peak_static_memory_bytes := 0.0


func _ready() -> void:
    handoff = _load_handoff(HANDOFF_PATH)


func _process(delta: float) -> void:
    if completed:
        return

    if not capturing:
        warmup_elapsed += delta
        if warmup_elapsed >= WARMUP_SECONDS:
            _begin_capture()
        return

    cpu_frame_samples_ms.append(Performance.get_monitor(Performance.TIME_PROCESS) * 1000.0)
    peak_static_memory_bytes = max(
        peak_static_memory_bytes,
        Performance.get_monitor(Performance.MEMORY_STATIC)
    )
    capture_elapsed += delta

    if capture_elapsed >= float(handoff["duration_seconds"]):
        _finish_capture()


func _begin_capture() -> void:
    capturing = true
    capture_elapsed = 0.0
    cpu_frame_samples_ms.clear()
    peak_static_memory_bytes = 0.0
    var adapter := get_parent()
    if adapter.has_method("restart_canonical_playback"):
        adapter.restart_canonical_playback()
    print("Everward Godot benchmark capture started after %.1f s warmup" % WARMUP_SECONDS)


func _finish_capture() -> void:
    capturing = false
    completed = true
    var observation := _build_observation()
    var file := FileAccess.open(OUTPUT_PATH, FileAccess.WRITE)
    if file == null:
        push_error("Unable to write benchmark observation: %s" % OUTPUT_PATH)
        return
    file.store_string(JSON.stringify(observation, "\t"))
    file.close()
    print("Everward Godot benchmark capture observation written to %s" % OUTPUT_PATH)


func _build_observation() -> Dictionary:
    var sorted_samples := cpu_frame_samples_ms.duplicate()
    sorted_samples.sort()
    var memory_info := OS.get_memory_info()
    var physical_memory := float(memory_info.get("physical", -1))
    var engine_version := Engine.get_version_info()

    return {
        "observation_version": 1,
        "engine": "godot",
        "scenario_name": handoff["scenario_name"],
        "scenario_version": handoff["scenario_version"],
        "captured_at_utc": Time.get_datetime_string_from_system(true, false) + "Z",
        "warmup_seconds": WARMUP_SECONDS,
        "capture_duration_seconds": handoff["duration_seconds"],
        "frame_sample_count": sorted_samples.size(),
        "hardware": {
            "engine_version": str(engine_version.get("string", "unknown")),
            "os_version": "%s %s" % [OS.get_name(), OS.get_version()],
            "cpu_model": OS.get_processor_name(),
            "gpu_model": RenderingServer.get_video_adapter_name(),
            "ram_gib": physical_memory / 1073741824.0 if physical_memory >= 0.0 else -1.0,
        },
        "cpu_frame_time_ms": {
            "mean": _mean(sorted_samples),
            "p50": _percentile(sorted_samples, 0.50),
            "p95": _percentile(sorted_samples, 0.95),
            "max": sorted_samples[sorted_samples.size() - 1] if not sorted_samples.is_empty() else 0.0,
        },
        "static_memory_peak_mib": peak_static_memory_bytes / 1048576.0,
        "run_record_prefill": {
            "engine_version": str(engine_version.get("string", "unknown")),
            "os_version": "%s %s" % [OS.get_name(), OS.get_version()],
            "cpu_model": OS.get_processor_name(),
            "gpu_model": RenderingServer.get_video_adapter_name(),
            "ram_gib": physical_memory / 1073741824.0 if physical_memory >= 0.0 else -1.0,
            "cpu_frame_time_ms": _percentile(sorted_samples, 0.50),
        },
        "manual_evidence_still_required": [
            "project_settings",
            "gpu_frame_time_ms",
            "peak_memory_mib",
            "implementation_hours",
            "build_size_mib",
            "screenshots",
            "notes",
        ],
        "measurement_notes": [
            "CPU frame timing uses Godot Performance.TIME_PROCESS.",
            "static_memory_peak_mib is diagnostic only and is not substituted for decision-grade peak process memory.",
            "GPU frame time remains manual because this adapter does not infer it from CPU-side timing.",
        ],
    }


func _load_handoff(path: String) -> Dictionary:
    if not FileAccess.file_exists(path):
        push_error("Missing canonical benchmark handoff: %s" % path)
        get_tree().quit(2)
        return {}
    var file := FileAccess.open(path, FileAccess.READ)
    var parsed = JSON.parse_string(file.get_as_text())
    if typeof(parsed) != TYPE_DICTIONARY:
        push_error("Benchmark handoff must contain a JSON object")
        get_tree().quit(2)
        return {}
    return parsed


func _mean(values: Array[float]) -> float:
    if values.is_empty():
        return 0.0
    var total := 0.0
    for value in values:
        total += value
    return total / float(values.size())


func _percentile(values: Array[float], fraction: float) -> float:
    if values.is_empty():
        return 0.0
    var index := int(round((values.size() - 1) * fraction))
    return values[clamp(index, 0, values.size() - 1)]
