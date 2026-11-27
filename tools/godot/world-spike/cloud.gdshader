shader_type spatial;
// Raymarched volumetric cloud layer drawn as a full-screen quad (spike). The ship's recent positions
// (trail[]) carve a cavity and push the noise pattern outward, so clouds part and billow around it
// and recover as the trail ages (radius is shrunk on the CPU side).
render_mode unshaded, depth_draw_never, depth_test_disabled, cull_disabled, blend_mix;

uniform sampler2D depth_tex : hint_depth_texture, repeat_disable, filter_nearest;
uniform sampler3D noise_tex : repeat_enable, filter_linear;

uniform float cloud_base = 110.0;
uniform float cloud_top = 240.0;
uniform float coverage : hint_range(0.0, 1.0) = 0.52;
uniform float noise_scale = 0.0042;
uniform vec3 wind = vec3(0.012, 0.0, 0.006);
uniform float max_distance = 1400.0;
uniform int steps = 28;
uniform vec3 sun_dir = vec3(0.45, 0.75, 0.3);
uniform vec3 sun_color : source_color = vec3(1.0, 0.96, 0.88);
uniform vec3 ambient_color : source_color = vec3(0.55, 0.66, 0.85);
uniform float absorption = 0.045;
uniform vec4 trail[16]; // xyz = world position, w = current radius (0 = unused)
uniform vec4 trail_bound = vec4(0.0, -99999.0, 0.0, 0.0); // sphere enclosing every trail point (skips the loop elsewhere)

float hash(vec2 p) {
	return fract(sin(dot(p, vec2(12.9898, 78.233))) * 43758.5453);
}

float density(vec3 p) {
	float h = (p.y - cloud_base) / (cloud_top - cloud_base);
	if (h <= 0.0 || h >= 1.0) return 0.0;
	float shape = smoothstep(0.0, 0.18, h) * (1.0 - smoothstep(0.55, 1.0, h));
	vec3 q = p * noise_scale + wind * TIME;
	float cavity = 1.0;
	float tb = length(p - trail_bound.xyz) - trail_bound.w;
	for (int i = 0; i < 16; i++) {
		if (tb > 0.0) break;
		vec4 t = trail[i];
		if (t.w > 0.0) {
			vec3 dv = p - t.xyz;
			float dist = length(dv) + 0.001;
			float r = t.w;
			if (dist < r * 2.2) {
				// push the noise pattern outward (displaced moisture) and open a cavity
				q += (dv / dist) * exp(-pow(dist / r, 2.0)) * 0.035;
				cavity *= smoothstep(r * 0.45, r * 1.5, dist);
			}
		}
	}
	float n = texture(noise_tex, q).r;
	float d = smoothstep(coverage, coverage + 0.28, n) * shape * cavity;
	return d;
}

void vertex() {
	POSITION = vec4(VERTEX.xy, 1.0, 1.0);
}

void fragment() {
	float depth = texture(depth_tex, SCREEN_UV).x;
	vec3 ndc = vec3(SCREEN_UV * 2.0 - 1.0, depth);
	vec4 view = INV_PROJECTION_MATRIX * vec4(ndc, 1.0);
	view.xyz /= view.w;
	vec4 pv = INV_PROJECTION_MATRIX * vec4(ndc.xy, 0.5, 1.0);
	pv.xyz /= pv.w;
	vec3 dir_view = normalize(pv.xyz);
	vec3 rd = normalize((INV_VIEW_MATRIX * vec4(dir_view, 0.0)).xyz);
	vec3 ro = CAMERA_POSITION_WORLD;

	float scene_t = (depth < 0.0000001) ? 1.0e6 : (-view.z) / max(-dir_view.z, 0.0001);
	float tmax = min(scene_t, max_distance);

	// intersect the horizontal slab
	float t0 = 0.0;
	float t1 = tmax;
	if (abs(rd.y) < 0.0001) {
		if (ro.y < cloud_base || ro.y > cloud_top) { ALPHA = 0.0; ALBEDO = vec3(0.0); }
	} else {
		float ta = (cloud_base - ro.y) / rd.y;
		float tb = (cloud_top - ro.y) / rd.y;
		t0 = max(min(ta, tb), 0.0);
		t1 = min(max(ta, tb), tmax);
	}
	if (t1 <= t0) {
		ALPHA = 0.0;
		ALBEDO = vec3(0.0);
	} else {
		float dt = (t1 - t0) / float(steps);
		float t = t0 + dt * hash(SCREEN_UV * 1000.0 + vec2(TIME));
		float transmittance = 1.0;
		vec3 col = vec3(0.0);
		vec3 sd = normalize(sun_dir);
		for (int i = 0; i < steps; i++) {
			vec3 p = ro + rd * t;
			float d = density(p);
			if (d > 0.003) {
				float l1 = density(p + sd * 40.0);
				float light = exp(-l1 * 70.0 * absorption * 2.2);
				float h = clamp((p.y - cloud_base) / (cloud_top - cloud_base), 0.0, 1.0);
				vec3 lit = sun_color * light * (0.35 + 0.9 * h) + ambient_color * (0.5 + 0.5 * h);
				float a = 1.0 - exp(-d * absorption * dt);
				col += transmittance * a * lit;
				transmittance *= 1.0 - a;
				if (transmittance < 0.02) break;
			}
			t += dt;
		}
		ALBEDO = col / max(1.0 - transmittance, 0.0001);
		ALPHA = clamp(1.0 - transmittance, 0.0, 1.0);
	}
}
