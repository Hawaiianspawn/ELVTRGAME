class_name Hall3D
extends Node3D
## The hall as real 3D geometry: tiled floor, two brick walls, black depth fog — plus the shared
## helpers that map the sim into it. Sim coordinates are unchanged (wx lateral, wd depth ahead of
## the camera); a point sits at Vector3(wx, height, -wd). The camera looks down -Z from
## (cam_x, cam_h, 0) and never yaws, so a Sprite3D facing +Z is already a billboard and the
## 8-direction frames carry over untouched. Depth testing sorts the crowd per pixel, so nothing
## y-sorts and juggled units no longer punch through the sprite in front of them.
##
## Named Hall3D, not World3D: `World3D` is a built-in engine class and cannot be shadowed.
##
## Heights, lunges and fx radii are quoted in SPRITE PIXELS (what every tuned number in Battle.gd
## and Unit.gd is already in); PIXEL converts them to world units.

const PIXEL := 2.2              # world units per sprite pixel: an 88px cell stands 193.6 units tall
const TILE := 64.0              # floor cell / wall brick course, world units
const COURSES := 3              # wall height in courses
const NEAR_D := 128.0           # nearest tiled floor row (the frame's bottom edge meets the ground ~d=240)
const FAR_D := 1216.0           # last tiled row; one flat quad covers the rest, fully fogged anyway
const EDGE_D := 3000.0
const WALL_D0 := 40.0
const WALL_D1 := 2000.0
const FOG_START := 380.0
const FOG_END := 980.0
const FLOOR_TINT := Color("#2a2a2b")   # floor base tint; the tiles are multiplied down to this
const FLOOR_Y := -1.0           # a hair under the sprites' feet so quad and billboard never z-fight

# Floor tiles: indices 0,1 are plain, weighted ~80% via repeat count in the pool below.
const FLOOR_PLAIN := [preload("res://assets/env/hall/floor_00.png"), preload("res://assets/env/hall/floor_01.png")]
const FLOOR_OTHER := [
	preload("res://assets/env/hall/floor_02.png"), preload("res://assets/env/hall/floor_03.png"),
	preload("res://assets/env/hall/floor_04.png"), preload("res://assets/env/hall/floor_05.png"),
	preload("res://assets/env/hall/floor_06.png"), preload("res://assets/env/hall/floor_07.png"),
	preload("res://assets/env/hall/floor_08.png"), preload("res://assets/env/hall/floor_09.png"),
	preload("res://assets/env/hall/floor_10.png"), preload("res://assets/env/hall/floor_11.png"),
	preload("res://assets/env/hall/floor_14.png"), preload("res://assets/env/hall/floor_15.png"),
]
# Wall: wall_14 with its baked top ledge cropped off (rows 12..43) -> 32x32, loops both ways.
const WALL_TEX := preload("res://assets/env/hall/wall_flat.png")


## Deterministic integer hash so tile variant choice stays put as `scroll` marches, no flicker.
static func _hash2(a: int, b: int) -> int:
	var h := a * 374761393 + b * 668265263
	h = (h ^ (h >> 13)) * 1274126177
	return absi(h ^ (h >> 16))


static func _build_pool(plain: Array, other: Array, plain_repeat: int) -> Array:
	var pool: Array = []
	for t in plain:
		for i in range(plain_repeat):
			pool.append(t)
	pool.append_array(other)
	return pool


static var FLOOR_POOL: Array = _build_pool(FLOOR_PLAIN, FLOOR_OTHER, 24)   # 2 plain x24 + 12 other = 80% plain


# ---- sprites ---------------------------------------------------------------------------------

static var _atlas: Texture2D


static func _region(name: String, frames: int) -> AtlasTexture:
	if _atlas == null:
		_atlas = load("res://assets/sprites/atlas.png")
	var at := AtlasTexture.new()
	at.atlas = _atlas
	var cell: int = int(Game.sprites[name]["cell"])
	at.region = Rect2(0, int(Game.sprites[name]["y"]), cell * frames, cell)
	return at


static func _pixel_sprite(at: AtlasTexture, frames: int) -> Sprite3D:
	var s := Sprite3D.new()
	s.texture = at
	s.hframes = frames
	s.pixel_size = PIXEL
	s.billboard = BaseMaterial3D.BILLBOARD_DISABLED   # the camera never yaws: facing +Z already faces it
	s.texture_filter = BaseMaterial3D.TEXTURE_FILTER_NEAREST
	s.shaded = false
	s.centered = true
	return s


## Sprite3D for a packed 8-direction strip; `facing` indexes Game.DIRS, feet sit on the node origin.
## Opaque + alpha-discard, so it writes depth and the crowd sorts per pixel instead of per sprite.
static func make_sprite3d(name: String, facing := 0) -> Sprite3D:
	var s := _pixel_sprite(_region(name, 8), 8)
	s.frame = facing
	s.alpha_cut = SpriteBase3D.ALPHA_CUT_DISCARD
	s.alpha_scissor_threshold = 0.35
	s.offset = Vector2(0, int(Game.sprites[name]["cell"]) / 2.0)
	# own material per sprite: gl_compatibility has no instance uniforms, and the hit flash is a
	# per-unit uniform. The override drops Sprite3D's filter/scissor, so the shader carries them.
	var m := ShaderMaterial.new()
	m.shader = UNIT_SHADER
	m.set_shader_parameter("texture_albedo", _atlas)
	s.material_override = m
	return s


## Hit flash 0..1 on a make_sprite3d sprite; Unit._place drives it from the modulate pulse.
static func set_flash(s: Sprite3D, f: float) -> void:
	(s.material_override as ShaderMaterial).set_shader_parameter("flash", f)


## One-shot effect clip (manifest fx_* rows): hframes = clip length, frame 0, centred on the origin.
## Blended, not discarded — these are drawn at 20% alpha, a scissor would erase them whole.
static func make_fx3d(name: String) -> Sprite3D:
	var n: int = int(Game.sprites[name]["frames"])
	var s := _pixel_sprite(_region(name, n), n)
	s.alpha_cut = SpriteBase3D.ALPHA_CUT_DISABLED
	return s


# ---- projection ------------------------------------------------------------------------------

## OutRun bend: render-space only. The sim stays on a straight hall; every consumer of to_world /
## unproject picks the shift up, cursor_world takes it back out. Statics, because the projection
## helpers are static. Battle tweens curve_a/curve_l per wave; set_scroll feeds phase and pushes
## all three into the hall_bend shader so floor and walls bend by the same closed form.
static var curve_a := 0.0       # max lateral slope per depth unit; 0 = straight
static var curve_l := 460.0     # wavelength divisor
static var phase := 0.0         # = scroll, so the bend sweeps under you as the hall creeps


## Lateral shift at depth d: integral of curve_a * sin((u + phase) / curve_l) du from 0 to d.
static func curve_dx(d: float) -> float:
	return curve_a * curve_l * (cos(phase / curve_l) - cos((d + phase) / curve_l))


static func to_world(wx: float, wd: float, h := 0.0) -> Vector3:
	return Vector3(wx + curve_dx(wd), h, -wd)


## Half the viewport height over tan(fov/2): screen pixels per world unit at depth 1.
static func focal_px(cam: Camera3D) -> float:
	return 0.5 * cam.get_viewport().get_visible_rect().size.y / tan(deg_to_rad(cam.fov) * 0.5)


## Screen pixels per world unit at depth `wd`: what every 2D-overlay radius scales by.
static func screen_scale(cam: Camera3D, wd: float) -> float:
	return focal_px(cam) / maxf(wd, 20.0)


static func unproject(cam: Camera3D, wx: float, wd: float, h := 0.0) -> Vector2:
	return cam.unproject_position(to_world(wx, wd, h))


## Screen point of the top edge of a unit's sprite. Unit._air_cap and the adversary's
## "airborne unit above the frame" check both read this, so they cannot disagree.
static func sprite_top_screen(cam: Camera3D, u: Node3D) -> Vector2:
	return unproject(cam, u.wx, u.wd, u.air_h * PIXEL + u.top_world())


## Inverse of sprite_top_screen: the greatest height (world units) whose sprite top, `top_world`
## above the feet, still lands at screen y >= margin. Screen y is a ratio of two affine functions
## of height, so this solves in closed form — nothing to iterate.
static func air_cap(cam: Camera3D, wd: float, top_world: float, margin := 8.0) -> float:
	var t := cam.global_transform
	var m := (0.5 * cam.get_viewport().get_visible_rect().size.y - margin) / focal_px(cam)
	var th: float = t.basis.get_euler().x
	var c := cos(th)
	var s := sin(th)
	var den := c - m * s
	if den <= 0.0001:
		return 0.0
	return maxf(0.0, wd * (m * c + s) / den + t.origin.y - top_world)


## Ground point under a screen position: ray onto the y=0 plane, depth clamped to 60..max_d and
## the lateral read back at that clamped depth (so a cursor above the horizon still lands sanely).
static func cursor_world(cam: Camera3D, screen_pos: Vector2, max_d: float) -> Vector2:
	var o := cam.project_ray_origin(screen_pos)
	var n := cam.project_ray_normal(screen_pos)
	var d := max_d
	if n.y < -0.000001:
		d = clampf(-(o.z + n.z * (-o.y / n.y)), 60.0, max_d)
	if absf(n.z) < 0.000001:
		return Vector2(o.x - curve_dx(d), d)
	return Vector2(o.x + n.x * (o.z + d) / -n.z - curve_dx(d), d)   # back onto the straight sim lane


# ---- the hall --------------------------------------------------------------------------------

var _floor: Node3D
var _tiles: Array[MeshInstance3D] = []
var _pool_mats: Array[ShaderMaterial] = []   # parallel to FLOOR_POOL
var _wall_mat: ShaderMaterial
var _bend_mats: Array[ShaderMaterial] = []   # every material the bend uniforms go to
var _cols := 0
var _rows := 0
var _shift := -0x7FFFFFFF

const BEND_SHADER := preload("res://assets/shaders/hall_bend.gdshader")
const UNIT_SHADER := preload("res://assets/shaders/unit_sprite.gdshader")
const WALL_SEGS := 24           # depth bands per wall quad so the bend reads as a curve, not a kink


## Nearest-filtered tile x tint, unshaded, on the bend shader (straight until curve_a is set).
func _unshaded(tex: Texture2D, tint: Color) -> ShaderMaterial:
	var m := ShaderMaterial.new()
	m.shader = BEND_SHADER
	m.set_shader_parameter("tex", tex)
	m.set_shader_parameter("tint", tint)
	_bend_mats.append(m)
	return m


## Floor, walls and the black fog, for a hall `half` world units either side of centre.
func build(half: float) -> void:
	curve_a = 0.0   # a fresh hall starts straight; Battle ramps it per wave
	phase = 0.0
	var we := WorldEnvironment.new()
	var e := Environment.new()
	e.background_mode = Environment.BG_COLOR
	e.background_color = Color.BLACK
	e.ambient_light_source = Environment.AMBIENT_SOURCE_COLOR
	e.ambient_light_color = Color.WHITE
	e.ambient_light_energy = 1.0
	e.fog_enabled = true
	e.fog_mode = Environment.FOG_MODE_DEPTH
	e.fog_light_color = Color.BLACK
	e.fog_density = 1.0
	e.fog_depth_begin = FOG_START
	e.fog_depth_end = FOG_END
	e.fog_depth_curve = 1.0
	e.fog_sky_affect = 0.0
	we.environment = e
	add_child(we)
	_build_floor(half)
	_build_walls(half)


## One quad per grid cell sharing 14 materials, all under one node so the treadmill is a single
## transform per frame. ponytail: 14x7 quads holds fps fine; if it ever doesn't, merge per material.
func _build_floor(half: float) -> void:
	var mats := {}
	for t in FLOOR_POOL:
		if not mats.has(t):
			mats[t] = _unshaded(t, FLOOR_TINT)
		_pool_mats.append(mats[t])
	var mesh := PlaneMesh.new()
	mesh.size = Vector2(TILE, TILE)
	_cols = int(ceil(half * 2.0 / TILE))
	_rows = int((FAR_D - NEAR_D) / TILE)
	_floor = Node3D.new()
	add_child(_floor)
	for row in range(_rows):
		for col in range(_cols):
			var mi := MeshInstance3D.new()
			mi.mesh = mesh
			mi.position = Vector3(-half + (col + 0.5) * TILE, FLOOR_Y, -(NEAR_D + (row + 0.5) * TILE))
			_tiles.append(mi)
			_floor.add_child(mi)
	var far := MeshInstance3D.new()
	var fm := PlaneMesh.new()
	fm.size = Vector2(half * 2.0 + TILE * 2.0, EDGE_D - FAR_D)
	far.mesh = fm
	far.material_override = _unshaded(FLOOR_PLAIN[0], FLOOR_TINT)
	far.position = Vector3(0, FLOOR_Y, -(FAR_D + EDGE_D) * 0.5)
	add_child(far)
	_apply_variants(0)


func _apply_variants(shift: int) -> void:
	if shift == _shift:
		return
	_shift = shift
	for row in range(_rows):
		for col in range(_cols):
			_tiles[row * _cols + col].material_override = _pool_mats[_hash2(col, row + shift) % _pool_mats.size()]


## Two vertical walls, one quad each per vertical band — the GPU interpolates u perspective-correctly
## so no depth banding is needed. Top band fades to black: no ceiling edge, same read as before.
func _build_walls(half: float) -> void:
	var st := SurfaceTool.new()
	st.begin(Mesh.PRIMITIVE_TRIANGLES)
	var top := TILE * COURSES
	var mid := top * 0.6
	for side: float in [-1.0, 1.0]:
		_wall_quad(st, side * half, 0.0, mid, Color.WHITE, Color.WHITE)
		_wall_quad(st, side * half, mid, top, Color.WHITE, Color.BLACK)
	var mi := MeshInstance3D.new()
	mi.mesh = st.commit()
	_wall_mat = _unshaded(WALL_TEX, Color.WHITE)   # vertex colour and no culling are in the shader
	mi.material_override = _wall_mat
	add_child(mi)


## One band of wall, WALL_SEGS quads down its depth: the bend shader moves vertices, so a single
## quad would only bend at its two ends.
func _wall_quad(st: SurfaceTool, x: float, y0: float, y1: float, c0: Color, c1: Color) -> void:
	var top := TILE * COURSES
	for seg in range(WALL_SEGS):
		var da := lerpf(WALL_D0, WALL_D1, float(seg) / WALL_SEGS)
		var db := lerpf(WALL_D0, WALL_D1, float(seg + 1) / WALL_SEGS)
		var ua := da / TILE
		var ub := db / TILE
		var p := [Vector3(x, y0, -da), Vector3(x, y0, -db), Vector3(x, y1, -db), Vector3(x, y1, -da)]
		var uv := [Vector2(ua, (top - y0) / TILE), Vector2(ub, (top - y0) / TILE), Vector2(ub, (top - y1) / TILE), Vector2(ua, (top - y1) / TILE)]
		var col := [c0, c0, c1, c1]
		for i in [0, 1, 2, 0, 2, 3]:
			st.set_color(col[i])
			st.set_uv(uv[i])
			st.set_normal(Vector3(-signf(x), 0.0, 0.0))
			st.add_vertex(p[i])


## Forward creep: floor tiles march toward the camera and wrap a cell at a time (the variant grid
## shifts with them so no tile ever changes texture under you), bricks slide with the same phase.
func set_scroll(s: float) -> void:
	_floor.position.z = fposmod(s, TILE)
	_apply_variants(int(floor(s / TILE)))
	_wall_mat.set_shader_parameter("uv_offset", Vector2(s / TILE, 0.0))
	phase = s
	for m in _bend_mats:   # ponytail: ~16 materials x 3 params a frame; shader globals if it ever shows
		m.set_shader_parameter("curve_a", curve_a)
		m.set_shader_parameter("curve_l", curve_l)
		m.set_shader_parameter("phase", phase)
