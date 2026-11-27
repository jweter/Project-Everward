shader_type spatial;
// Terrain material for the smooth voxel mesh (spike): slope/altitude based rock, grass and snow with
// cheap procedural detail. Voxel Tools meshes carry no UVs, so everything keys off world position/normal.
uniform vec3 grass_a : source_color = vec3(0.16, 0.30, 0.10);
uniform vec3 grass_b : source_color = vec3(0.30, 0.42, 0.14);
uniform vec3 rock_a : source_color = vec3(0.33, 0.30, 0.28);
uniform vec3 rock_b : source_color = vec3(0.52, 0.48, 0.44);
uniform vec3 snow_color : source_color = vec3(0.92, 0.94, 0.98);
varying vec3 v_world;
varying vec3 v_normal;

float hash31(vec3 p) {
	p = fract(p * 0.3183099 + vec3(0.1, 0.2, 0.3));
	p *= 17.0;
	return fract(p.x * p.y * p.z * (p.x + p.y + p.z));
}
float vnoise(vec3 x) {
	vec3 i = floor(x);
	vec3 f = fract(x);
	f = f * f * (3.0 - 2.0 * f);
	return mix(mix(mix(hash31(i + vec3(0, 0, 0)), hash31(i + vec3(1, 0, 0)), f.x),
	               mix(hash31(i + vec3(0, 1, 0)), hash31(i + vec3(1, 1, 0)), f.x), f.y),
	           mix(mix(hash31(i + vec3(0, 0, 1)), hash31(i + vec3(1, 0, 1)), f.x),
	               mix(hash31(i + vec3(0, 1, 1)), hash31(i + vec3(1, 1, 1)), f.x), f.y), f.z);
}
float fbm(vec3 p) {
	float a = 0.5;
	float s = 0.0;
	for (int i = 0; i < 4; i++) {
		s += a * vnoise(p);
		p *= 2.03;
		a *= 0.5;
	}
	return s;
}

void vertex() {
	v_world = (MODEL_MATRIX * vec4(VERTEX, 1.0)).xyz;
	v_normal = normalize((MODEL_MATRIX * vec4(NORMAL, 0.0)).xyz);
}

void fragment() {
	float slope = clamp(v_normal.y, 0.0, 1.0);
	float detail = fbm(v_world * 0.35);
	float coarse = fbm(v_world * 0.04);
	vec3 rock = mix(rock_a, rock_b, detail);
	vec3 grass = mix(grass_a, grass_b, detail);
	vec3 snow = snow_color;
	vec3 col = mix(rock, grass, smoothstep(0.62, 0.82, slope + (coarse - 0.5) * 0.25) * (1.0 - smoothstep(120.0, 190.0, v_world.y)));
	float snow_line = 165.0 + (coarse - 0.5) * 40.0;
	col = mix(col, snow, smoothstep(snow_line, snow_line + 20.0, v_world.y) * smoothstep(0.45, 0.8, slope));
	// freshly exposed interior (hollowed rock) reads darker and warmer
	ALBEDO = col;
	ROUGHNESS = 0.92;
}
