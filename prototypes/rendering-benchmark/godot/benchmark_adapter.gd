extends Node3D

const HANDOFF_PATH := "res://benchmark_handoff.json"
const EXPECTED_HANDOFF_VERSION := 2
const EXPECTED_SCENARIO_NAME := "icy-asteroid-mining"

@onready var asteroid: MeshInstance3D = $Asteroid
@onready var probe: MeshInstance3D = $Probe
@onready var planet: MeshInstance3D = $Planet
@onready var star_light: DirectionalLight3D = $StarLight
@onready var benchmark_camera: Camera3D = $BenchmarkCamera
@onready var telemetry: Label = $HUD/Telemetry

var handoff: Dictionary = {}
var elapsed_real_seconds := 0.0
var active_camera_stage := ""


func _ready() -> void:
    handoff = _load_handoff(HANDOFF_PATH)
    _validate_handoff(handoff)
    _apply_static_scene_truth()
    _apply_camera_stage(_camera_stage_at(0.0))


func _process(delta: float) -> void:
    elapsed_real_seconds = fmod(
        elapsed_real_seconds + delta,
        float(handoff["duration_seconds"])
    )
    var stage := _camera_stage_at(elapsed_real_seconds)
    if stage != active_camera_stage:
        _apply_camera_stage(stage)
    _apply_deterministic_animation(elapsed_real_seconds)
    _update_hud(elapsed_real_seconds)


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


func _validate_handoff(bundle: Dictionary) -> void:
    var required := [
        "handoff_version",
        "scenario_name",
        "scenario_version",
        "duration_seconds",
        "target_resolution",
        "target_fps",
        "simulation_seconds_per_real_second",
        "camera_sequence",
        "camera_stage_durations_seconds",
        "required_scene_features",
        "objects",
        "cameras",
        "animation_periods_seconds",
        "feature_bindings"
    ]
    for key in required:
        if not bundle.has(key):
            push_error("Benchmark handoff missing required key: %s" % key)
            get_tree().quit(2)
            return
    if int(bundle["handoff_version"]) != EXPECTED_HANDOFF_VERSION:
        push_error("Unsupported benchmark handoff version")
        get_tree().quit(2)
        return
    if str(bundle["scenario_name"]) != EXPECTED_SCENARIO_NAME:
        push_error("Unexpected benchmark scenario")
        get_tree().quit(2)


func _apply_static_scene_truth() -> void:
    var objects: Dictionary = handoff["objects"]
    asteroid.position = _vec3(objects["asteroid"]["position_m"])
    probe.position = _vec3(objects["probe"]["position_m"])
    planet.position = _vec3(objects["planet"]["position_m"])

    var direction := _vec3(objects["star_light"]["direction"]).normalized()
    star_light.look_at(star_light.global_position + direction, Vector3.UP)


func _camera_stage_at(elapsed: float) -> String:
    var sequence: Array = handoff["camera_sequence"]
    var durations: Array = handoff["camera_stage_durations_seconds"]
    var boundary := 0.0
    for index in range(sequence.size()):
        boundary += float(durations[index])
        if elapsed < boundary:
            return str(sequence[index])
    return str(sequence[sequence.size() - 1])


func _apply_camera_stage(stage: String) -> void:
    var config: Dictionary = handoff["cameras"][stage]
    benchmark_camera.position = _vec3(config["position_m"])
    benchmark_camera.fov = float(config["vertical_fov_degrees"])
    benchmark_camera.look_at(_vec3(config["target_m"]), Vector3.UP)
    active_camera_stage = stage


func _apply_deterministic_animation(elapsed: float) -> void:
    var periods: Dictionary = handoff["animation_periods_seconds"]
    var asteroid_phase := fmod(elapsed, float(periods["asteroid_rotation"])) / float(periods["asteroid_rotation"])
    asteroid.rotation.y = asteroid_phase * TAU

    var mining_phase := fmod(elapsed, float(periods["mining_mechanism"])) / float(periods["mining_mechanism"])
    probe.rotation.z = sin(mining_phase * TAU) * 0.08


func _update_hud(elapsed: float) -> void:
    var simulated := elapsed * float(handoff["simulation_seconds_per_real_second"])
    telemetry.text = (
        "EVERWARD // GODOT PROTOTYPE C\n"
        + "camera: %s\n" % active_camera_stage
        + "real: %.3f s   simulated: %.3f s" % [elapsed, simulated]
    )


func _vec3(values: Array) -> Vector3:
    return Vector3(float(values[0]), float(values[1]), float(values[2]))
