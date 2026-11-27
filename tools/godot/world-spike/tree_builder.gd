extends RefCounted
# Procedural trees for the world spike (assets layer: built only from the tree material).
#   conifer:   tapered trunk, drooping whorls of branches carrying needle-cluster cards, a spire on top
#   broadleaf: short trunk splitting into limbs and twigs, rounded clumps of leaf cards at the twig ends
# Vertex COLOR carries data for tree.gdshader, not colour:
#   r = part (0 bark, 1 foliage), g = per-card tint variation, b = height in the tree (0 root .. 1 top)
# Foliage cards get normals pointing out of the crown, so the canopy shades as one soft mass.

const NEEDLE := 0
const LEAF := 1

var rng := RandomNumberGenerator.new()
var _st: SurfaceTool
var _height := 1.0


func _init(seed_value: int = 1) -> void:
	rng.seed = seed_value


func build_conifer() -> ArrayMesh:
	_begin(rng.randf_range(12.0, 17.0))
	var h := _height
	var r0 := h * 0.022
	_tube(Vector3.ZERO, Vector3(0, h * 0.97, 0), r0, 0.04, 7, 5)
	var whorls := int(h * 0.8)
	for w in whorls:
		var t := lerpf(0.2, 0.96, float(w) / float(whorls - 1))
		var y := h * t
		var length := (1.0 - t) * h * 0.4 + 0.7
		var n := rng.randi_range(5, 7)
		var phase := rng.randf() * TAU
		var droop := lerpf(0.42, 0.08, t)
		for b in n:
			var ang := phase + b * TAU / n + rng.randf_range(-0.25, 0.25)
			var dir := Vector3(cos(ang), -droop + rng.randf_range(-0.08, 0.08), sin(ang)).normalized()
			var start := Vector3(0, y, 0)
			var end := start + dir * length * rng.randf_range(0.85, 1.1)
			_tube(start, end, 0.06 * (1.2 - t), 0.015, 3, 1)
			var cards := maxi(1, int(length / 1.1))
			for k in cards:
				var f := (k + 0.65) / cards
				var pos := start.lerp(end, f) + Vector3(0, 0.08, 0)
				var size := lerpf(1.9, 1.1, t) * rng.randf_range(0.85, 1.15)
				_foliage_card(pos, dir, size, true)
	# spire
	for i in 4:
		var pos := Vector3(rng.randf_range(-0.15, 0.15), h * (0.9 + i * 0.03), rng.randf_range(-0.15, 0.15))
		_foliage_card(pos, Vector3.UP, 1.1, false)
	return _commit()


func build_broadleaf() -> ArrayMesh:
	_begin(rng.randf_range(10.0, 14.0))
	var trunk_h := _height * rng.randf_range(0.32, 0.4)
	var top := Vector3(rng.randf_range(-0.3, 0.3), trunk_h, rng.randf_range(-0.3, 0.3))
	_tube(Vector3.ZERO, top, _height * 0.03, _height * 0.022, 7, 3)
	var limbs := rng.randi_range(3, 5)
	var phase := rng.randf() * TAU
	for l in limbs:
		var ang := phase + l * TAU / limbs + rng.randf_range(-0.3, 0.3)
		var rise := rng.randf_range(0.7, 1.25)
		var dir := Vector3(cos(ang), rise, sin(ang)).normalized()
		var limb_len := _height * rng.randf_range(0.3, 0.38)
		var limb_end := top + dir * limb_len
		_tube(top, limb_end, _height * 0.018, _height * 0.008, 5, 2)
		# inner fill along the limb so the crown reads as one mass, not clumps on sticks
		for f in 3:
			var along := top.lerp(limb_end, 0.45 + f * 0.2)
			_foliage_card(along + Vector3(rng.randf_range(-0.6, 0.6), rng.randf_range(0.2, 0.9), rng.randf_range(-0.6, 0.6)), dir, rng.randf_range(2.4, 3.2), false)
		for s in rng.randi_range(3, 4):
			var sdir := (dir + Vector3(rng.randf_range(-0.6, 0.6), rng.randf_range(-0.1, 0.5), rng.randf_range(-0.6, 0.6))).normalized()
			var twig_end := limb_end + sdir * _height * rng.randf_range(0.14, 0.22)
			_tube(limb_end, twig_end, _height * 0.007, 0.03, 4, 1)
			for c in rng.randi_range(10, 13):
				var offset := Vector3(rng.randf_range(-1, 1), rng.randf_range(-0.6, 1), rng.randf_range(-1, 1)).normalized() * rng.randf_range(0.3, 2.1)
				_foliage_card(twig_end + offset, sdir, rng.randf_range(2.3, 3.2), false)
	return _commit()


# ------------------------------------------------------------------------- textures
static func make_texture(kind: int, seed_value: int = 7) -> ImageTexture:
	var r := RandomNumberGenerator.new()
	r.seed = seed_value
	var size := 128
	var img := Image.create(size, size, false, Image.FORMAT_RGBA8)
	img.fill(Color(0, 0, 0, 0))
	if kind == NEEDLE:
		# a twig along the middle with needles sprayed either side
		for i in 160:
			var u := r.randf_range(0.08, 0.92)
			var side := -1.0 if r.randf() < 0.5 else 1.0
			var length := r.randf_range(0.12, 0.34) * (1.0 - absf(u - 0.5))
			var ang := side * r.randf_range(0.6, 1.2)
			var shade := r.randf_range(0.55, 1.0)
			for s in 24:
				var f := s / 23.0
				var x := u + cos(ang) * length * f * 0.35
				var y := 0.5 + sin(ang) * length * f
				_dot(img, x, y, 1, Color(shade, shade, shade, 1))
	else:
		# small leaves scattered in a rounded clump
		for i in 75:
			var a := r.randf() * TAU
			var rad := sqrt(r.randf()) * 0.42
			var cx := 0.5 + cos(a) * rad
			var cy := 0.5 + sin(a) * rad
			var shade := r.randf_range(0.5, 1.0)
			var rot := r.randf() * TAU
			for s in 40:
				var t := r.randf() * TAU
				var rr := sqrt(r.randf())
				var lx := cos(t) * rr * 0.05
				var ly := sin(t) * rr * 0.025
				_dot(img, cx + lx * cos(rot) - ly * sin(rot), cy + lx * sin(rot) + ly * cos(rot), 1, Color(shade, shade, shade, 1))
	img.generate_mipmaps()
	return ImageTexture.create_from_image(img)


static func _dot(img: Image, x: float, y: float, radius: int, c: Color) -> void:
	var size := img.get_width()
	var px := int(x * size)
	var py := int(y * size)
	for dy in range(-radius, radius + 1):
		for dx in range(-radius, radius + 1):
			var qx := px + dx
			var qy := py + dy
			if qx >= 0 and qy >= 0 and qx < size and qy < size:
				img.set_pixel(qx, qy, c)


# ------------------------------------------------------------------------- geometry
func _begin(height: float) -> void:
	_height = height
	_st = SurfaceTool.new()
	_st.begin(Mesh.PRIMITIVE_TRIANGLES)


func _commit() -> ArrayMesh:
	return _st.commit()


func _vertex(p: Vector3, n: Vector3, uv: Vector2, part: float, tint: float) -> void:
	_st.set_color(Color(part, tint, clampf(p.y / _height, 0.0, 1.0), 1.0))
	_st.set_normal(n)
	_st.set_uv(uv)
	_st.add_vertex(p)


func _tube(p0: Vector3, p1: Vector3, r0: float, r1: float, sides: int, segments: int) -> void:
	var axis := (p1 - p0).normalized()
	var side_a := axis.cross(Vector3.FORWARD if absf(axis.dot(Vector3.FORWARD)) < 0.9 else Vector3.RIGHT).normalized()
	var side_b := axis.cross(side_a).normalized()
	for seg in segments:
		var f0 := float(seg) / segments
		var f1 := float(seg + 1) / segments
		var c0 := p0.lerp(p1, f0)
		var c1 := p0.lerp(p1, f1)
		var ra := lerpf(r0, r1, f0)
		var rb := lerpf(r0, r1, f1)
		for i in sides:
			var a0 := TAU * i / sides
			var a1 := TAU * (i + 1) / sides
			var n0 := side_a * cos(a0) + side_b * sin(a0)
			var n1 := side_a * cos(a1) + side_b * sin(a1)
			var u0 := float(i) / sides
			var u1 := float(i + 1) / sides
			_vertex(c0 + n0 * ra, n0, Vector2(u0, f0), 0.0, 0.5)
			_vertex(c1 + n0 * rb, n0, Vector2(u0, f1), 0.0, 0.5)
			_vertex(c1 + n1 * rb, n1, Vector2(u1, f1), 0.0, 0.5)
			_vertex(c0 + n0 * ra, n0, Vector2(u0, f0), 0.0, 0.5)
			_vertex(c1 + n1 * rb, n1, Vector2(u1, f1), 0.0, 0.5)
			_vertex(c0 + n1 * ra, n1, Vector2(u1, f0), 0.0, 0.5)


# Two crossed quads. `lie_flat` makes the first quad roughly horizontal (needle sprays layer along branches).
func _foliage_card(center: Vector3, along: Vector3, size: float, lie_flat: bool) -> void:
	var tint := rng.randf()
	var out := (center - Vector3(0, center.y * 0.85, 0) + Vector3(0, 0.7, 0)).normalized()
	var a := along.normalized()
	var b := a.cross(Vector3.UP).normalized() if absf(a.dot(Vector3.UP)) < 0.95 else Vector3.RIGHT
	var up := Vector3.UP
	if not lie_flat:
		var t := rng.randf() * TAU
		b = (b * cos(t) + a.cross(b).normalized() * sin(t)).normalized()
		up = b.cross(a).normalized()
	var quads := [[a, b], [a, up if lie_flat else b.cross(a).normalized()]]
	for q in quads:
		var u: Vector3 = q[0] * size * 0.5
		var v: Vector3 = q[1] * size * 0.5
		var p00 := center - u - v
		var p10 := center + u - v
		var p11 := center + u + v
		var p01 := center - u + v
		_vertex(p00, out, Vector2(0, 1), 1.0, tint)
		_vertex(p10, out, Vector2(1, 1), 1.0, tint)
		_vertex(p11, out, Vector2(1, 0), 1.0, tint)
		_vertex(p00, out, Vector2(0, 1), 1.0, tint)
		_vertex(p11, out, Vector2(1, 0), 1.0, tint)
		_vertex(p01, out, Vector2(0, 0), 1.0, tint)
