extends RefCounted
# Look presets for the world spike: the "design tokens" layer of the Everward look.
# Every colour below was sampled from an owner reference image (palettes extracted by median-cut
# quantisation; see docs/LOOK_DEVELOPMENT.md). Atoms (tokens) -> materials -> assets -> scenes.

const LOOKS := {
	# Owner photo: low sun lighting an altocumulus deck from below, slate gaps between cloudlets.
	"ember_dusk": {
		"sky_top": Color("#3c4458"), "sky_horizon": Color("#ae7456"), "ground": Color("#372b29"),
		"sun_color": Color("#ffb27a"), "sun_energy": 1.25, "sun_pitch": -9.0, "sun_yaw": -35.0,
		"ambient_energy": 0.55, "fog_color": Color("#78594f"), "fog_density": 0.0012,
		"cloud_lit": Color("#ffb27a"), "cloud_shadow": Color("#6c5651"), "coverage": 0.48,
		"grass_a": Color("#313d1e"), "grass_b": Color("#4a5228"), "rock_a": Color("#2d2521"), "rock_b": Color("#645a54"),
		"snow": Color("#e7c7b0"), "foliage_base": Color("#1e2213"), "foliage_tip": Color("#40472d"),
		"trunk": Color("#2a1d16"), "autumn": Color("#c0582a"), "autumn_mix": 0.1,
	},
	# Red moon rising over misty conifers; teal-black sky; moon on a storm sea.
	"moonrise": {
		"sky_top": Color("#0e2a33"), "sky_horizon": Color("#ab8376"), "ground": Color("#091719"),
		"sun_color": Color("#f0b39c"), "sun_energy": 0.55, "sun_pitch": -11.0, "sun_yaw": 20.0,
		"ambient_energy": 0.45, "fog_color": Color("#25555f"), "fog_density": 0.0022,
		"cloud_lit": Color("#b98d7f"), "cloud_shadow": Color("#203440"), "coverage": 0.5,
		"grass_a": Color("#0f2421"), "grass_b": Color("#1c3a36"), "rock_a": Color("#142b35"), "rock_b": Color("#3a434c"),
		"snow": Color("#9fb4bb"), "foliage_base": Color("#020a0b"), "foliage_tip": Color("#0b2023"),
		"trunk": Color("#050c0d"), "autumn": Color("#915955"), "autumn_mix": 0.0,
	},
	# Grass ridge in a break of storm light: dark wet rock, vivid lit grass, bright cloud gap.
	"storm_ridge": {
		"sky_top": Color("#3a3f3a"), "sky_horizon": Color("#c7bba9"), "ground": Color("#2d3228"),
		"sun_color": Color("#fff0d4"), "sun_energy": 1.9, "sun_pitch": -24.0, "sun_yaw": -60.0,
		"ambient_energy": 0.6, "fog_color": Color("#838172"), "fog_density": 0.0014,
		"cloud_lit": Color("#e6dccb"), "cloud_shadow": Color("#4e5444"), "coverage": 0.44,
		"grass_a": Color("#4f6b1f"), "grass_b": Color("#8fae3a"), "rock_a": Color("#141814"), "rock_b": Color("#2d3228"),
		"snow": Color("#d9d6cc"), "foliage_base": Color("#1b2a12"), "foliage_tip": Color("#3d5a1c"),
		"trunk": Color("#231c14"), "autumn": Color("#a07a3a"), "autumn_mix": 0.0,
	},
	# Fog-white sky, orange larches among dark spruce, still grey water (owner marsh photo too).
	"autumn_mist": {
		"sky_top": Color("#c3ccd7"), "sky_horizon": Color("#e8e6ec"), "ground": Color("#3d3c28"),
		"sun_color": Color("#fff4ea"), "sun_energy": 0.9, "sun_pitch": -40.0, "sun_yaw": 10.0,
		"ambient_energy": 1.0, "fog_color": Color("#d0d8e1"), "fog_density": 0.0035,
		"cloud_lit": Color("#edeef2"), "cloud_shadow": Color("#a29aa1"), "coverage": 0.4,
		"grass_a": Color("#3d3c28"), "grass_b": Color("#6e5a3a"), "rock_a": Color("#3e3131"), "rock_b": Color("#76766d"),
		"snow": Color("#eef0f4"), "foliage_base": Color("#12171b"), "foliage_tip": Color("#24302a"),
		"trunk": Color("#1e1814"), "autumn": Color("#d2601e"), "autumn_mix": 0.45,
	},
}

# Space palettes for later scenes (accretion discs, nebulae). Not used by the planet spike yet.
const SPACE_TOKENS := {
	"accretion_gold": [Color("#010000"), Color("#291006"), Color("#512a15"), Color("#935d34"), Color("#ecbf74")],
	"accretion_ash": [Color("#090808"), Color("#2d2521"), Color("#403c3a"), Color("#645a54"), Color("#a8998a")],
	"lensed_white": [Color("#000000"), Color("#272424"), Color("#838085"), Color("#f4f1ee")],
	"nebula_red": [Color("#03050b"), Color("#2a0809"), Color("#510f0d"), Color("#6b3739"), Color("#9fc4ff")],
	"nebula_teal_ember": [Color("#071921"), Color("#112936"), Color("#5b585e"), Color("#7e8c98"), Color("#e0893c")],
	"vortex_blue": [Color("#000103"), Color("#061732"), Color("#0c2959"), Color("#2d589d"), Color("#c9a6d8")],
}

static func names() -> PackedStringArray:
	return PackedStringArray(LOOKS.keys())
