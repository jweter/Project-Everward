shader_type spatial;
// Tree shader (spike): ambient wind sway plus a proximity shake/push when the ship flies near.
// Amplitude grows with height above the trunk base so roots stay planted.
uniform vec3 base_color : source_color = vec3(0.12, 0.3, 0.1);
uniform vec3 tip_color : source_color = vec3(0.3, 0.5, 0.15);
uniform vec3 ship_pos = vec3(0.0, -10000.0, 0.0);
uniform float shake_radius = 38.0;
uniform float shake_strength = 8.0;
uniform float tree_height = 9.0;
uniform float roughness_v : hint_range(0.0, 1.0) = 0.85;

varying float v_h;
varying float v_y;

void vertex() {
	v_y = VERTEX.y;
	v_h = clamp(VERTEX.y / tree_height, 0.0, 1.0);
	vec3 root = MODEL_MATRIX[3].xyz;
	float sway = sin(TIME * 1.4 + root.x * 0.07 + root.z * 0.05) * 0.18 * v_h * v_h;
	vec3 offset = vec3(sway, 0.0, sway * 0.6);
	vec3 to_ship = root - ship_pos;
	float dist = length(to_ship);
	float prox = 1.0 - smoothstep(0.0, shake_radius, dist);
	if (prox > 0.0) {
		vec3 away = normalize(vec3(to_ship.x, 0.0, to_ship.z) + vec3(0.001));
		float shake = sin(TIME * 22.0 + root.x * 0.9 + root.z * 0.6) * 0.55 + sin(TIME * 13.0 + root.z) * 0.35;
		offset += (away * 0.8 + vec3(shake, 0.0, shake * 0.7)) * prox * prox * shake_strength * v_h * v_h;
	}
	VERTEX += inverse(mat3(MODEL_MATRIX)) * offset;
}

void fragment() {
	vec3 foliage = mix(base_color, tip_color, v_h);
	ALBEDO = mix(vec3(0.26, 0.17, 0.1), foliage, smoothstep(2.6, 3.4, v_y));
	ROUGHNESS = roughness_v;
}
