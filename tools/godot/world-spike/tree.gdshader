shader_type spatial;
// Tree shader (spike) for tree_builder.gd meshes. Vertex COLOR is data, not colour:
//   r = part (0 bark, 1 foliage), g = per-card tint variation, b = height in the tree (0 root .. 1 top).
// Ambient wind sway grows with height; foliage also flutters; when the ship flies near, the crown shakes
// and leans away. Foliage cards are alpha-cut from a needle or leaf texture and lit softly from behind.
render_mode cull_disabled;

uniform sampler2D leaf_tex : source_color, filter_linear_mipmap, repeat_disable;
uniform vec3 base_color : source_color = vec3(0.12, 0.3, 0.1);
uniform vec3 tip_color : source_color = vec3(0.3, 0.5, 0.15);
uniform vec3 trunk_color : source_color = vec3(0.26, 0.17, 0.1);
uniform vec3 autumn_color : source_color = vec3(0.82, 0.38, 0.12);
uniform float autumn_mix : hint_range(0.0, 1.0) = 0.0;
uniform vec3 ship_pos = vec3(0.0, -10000.0, 0.0);
uniform float shake_radius = 38.0;
uniform float shake_strength = 8.0;
uniform float roughness_v : hint_range(0.0, 1.0) = 0.85;

varying float v_part;
varying float v_tint;
varying float v_h;
varying float v_pick;

void vertex() {
	v_part = COLOR.r;
	v_tint = COLOR.g;
	v_h = COLOR.b;
	vec3 root = MODEL_MATRIX[3].xyz;
	v_pick = fract(sin(dot(root.xz, vec2(12.9898, 78.233))) * 43758.5453);
	float sway = sin(TIME * 1.4 + root.x * 0.07 + root.z * 0.05) * 0.35 * v_h * v_h;
	vec3 offset = vec3(sway, 0.0, sway * 0.6);
	offset += v_part * vec3(
		sin(TIME * 5.3 + VERTEX.x * 2.1 + VERTEX.y * 1.3),
		sin(TIME * 4.1 + VERTEX.z * 1.7) * 0.5,
		cos(TIME * 4.7 + VERTEX.y * 1.9)) * 0.05 * v_h;
	vec3 to_ship = root - ship_pos;
	float dist = length(to_ship);
	float prox = 1.0 - smoothstep(0.0, shake_radius, dist);
	if (prox > 0.0) {
		vec3 away = normalize(vec3(to_ship.x, 0.0, to_ship.z) + vec3(0.001));
		float shake = sin(TIME * 22.0 + root.x * 0.9 + root.z * 0.6) * 0.55 + sin(TIME * 13.0 + root.z) * 0.35;
		offset += (away * 0.8 + vec3(shake, 0.0, shake * 0.7)) * prox * prox * shake_strength * 0.12 * v_h * v_h;
	}
	VERTEX += inverse(mat3(MODEL_MATRIX)) * offset;
}

void fragment() {
	ALPHA_SCISSOR_THRESHOLD = 0.5;
	if (v_part > 0.5) {
		vec4 t = texture(leaf_tex, UV);
		ALPHA = t.a;
		vec3 foliage = mix(base_color, tip_color, clamp(v_h * 0.7 + v_tint * 0.45, 0.0, 1.0));
		if (v_pick < autumn_mix) {
			foliage = mix(autumn_color * 0.55, autumn_color * 1.1, clamp(v_tint * 0.7 + v_h * 0.4, 0.0, 1.0));
		}
		ALBEDO = foliage * (0.6 + 0.6 * t.r);
		BACKLIGHT = foliage * 0.35;
		ROUGHNESS = roughness_v;
	} else {
		float grain = fract(sin(dot(floor(UV * vec2(7.0, 45.0)), vec2(12.9898, 78.233))) * 43758.5453);
		ALPHA = 1.0;
		ALBEDO = trunk_color * (0.75 + 0.45 * grain);
		ROUGHNESS = 0.95;
	}
}
