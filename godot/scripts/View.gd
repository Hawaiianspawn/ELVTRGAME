class_name View
extends RefCounted
## 2.5D pinhole camera behind and above the army. World = (x lateral, d depth ahead of camera), ground plane.
## screen = (cx + x*s, horizon + cam_h*s), s = focal / d. Sprites scale with s, fog with d.

var horizon := 160.0
var focal := 360.0
var cam_h := 150.0
var cam_x := 0.0
var cam_d := 0.0              # camera position along depth; d values are absolute
var sprite_k := 1.3           # sprite pixel scale at s == 1
var fog_color := Color.BLACK
var fog_start := 380.0
var fog_end := 980.0
var fog_max := 1.0
var far_color := Color.BLACK
var sky_color := Color.BLACK   # was #3a3b3d; walls fade to black now, ceiling above must be true black too
var sky_low := Color.BLACK
var near_color := Color("#2a2a2b")

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
const FLOOR_TILE_W := 64.0    # world units per floor grid cell


## Deterministic integer hash so tile variant choice stays put as `scroll` marches, no flicker.
static func _hash2(a: int, b: int) -> int:
	var h := a * 374761393 + b * 668265263
	h = (h ^ (h >> 13)) * 1274126177
	return absi(h ^ (h >> 16))


static func _pool_pick(pool: Array, a: int, b: int) -> Texture2D:
	return pool[_hash2(a, b) % pool.size()]


static func _build_pool(plain: Array, other: Array, plain_repeat: int) -> Array:
	var pool: Array = []
	for t in plain:
		for i in range(plain_repeat):
			pool.append(t)
	pool.append_array(other)
	return pool


static var FLOOR_POOL: Array = _build_pool(FLOOR_PLAIN, FLOOR_OTHER, 24)   # 2 plain x24 + 12 other = 80% plain


func s(d: float) -> float:
	return focal / maxf(d - cam_d, 20.0)


func project(x: float, d: float) -> Vector2:
	var k := s(d)
	return Vector2(480.0 + (x - cam_x) * k, horizon + cam_h * k)


func sprite_scale(d: float) -> float:
	return s(d) * sprite_k


func fog(d: float) -> Color:
	return Color.WHITE.lerp(fog_color, clampf((d - cam_d - fog_start) / (fog_end - fog_start), 0.0, fog_max))


## Depth of the ground point under a screen y (inverse of project).
func depth_at_y(y: float) -> float:
	return cam_d + focal * cam_h / maxf(y - horizon, 1.0)


func x_at(sx: float, d: float) -> float:
	return (sx - 480.0) / s(d) + cam_x


## Sky, fogged ground, receding row lines. `scroll` = forward creep in world units.
func draw_ground(c: CanvasItem, scroll: float, x_half: float) -> void:
	c.draw_rect(Rect2(-2000, -2000, 5000, 2000), sky_color)
	for i in range(8):
		var y0 := horizon * i / 8.0
		c.draw_rect(Rect2(-2000, y0, 5000, horizon / 8.0 + 1), sky_color.lerp(sky_low, (i + 1) / 8.0))
	# ground: grid of textured floor tiles, near->far depth rows x lateral columns.
	# ponytail: capped grid (rows x cols below) to hold FPS with the full army; shrink further if the probe dips.
	var d0 := 60.0
	var rows := 12
	var cols := int(ceil(x_half * 2.0 / FLOOR_TILE_W))
	var scroll_step := int(scroll / FLOOR_TILE_W)
	for i in range(rows):
		var t0 := float(i) / rows
		var t1 := float(i + 1) / rows
		var da := d0 * pow(fog_end * 1.5 / d0, t0)
		var db := d0 * pow(fog_end * 1.5 / d0, t1)
		var f := fog(cam_d + da) * near_color   # near_color: floor base tint; fog() darkens with distance same as the walls
		for col in range(cols):
			var x0 := -x_half + col * FLOOR_TILE_W
			var x1 := x0 + FLOOR_TILE_W
			var near_l := project(x0, cam_d + da)
			var near_r := project(x1, cam_d + da)
			var far_l := project(x0, cam_d + db)
			var far_r := project(x1, cam_d + db)
			var tex := _pool_pick(FLOOR_POOL, col, i + scroll_step)
			c.draw_polygon(
				PackedVector2Array([near_l, near_r, far_r, far_l]),
				PackedColorArray([f, f, f, f]),
				PackedVector2Array([Vector2(0, 1), Vector2(1, 1), Vector2(1, 0), Vector2(0, 0)]),
				tex)
	# row lines marching toward the camera
	var step := 80.0
	var d := d0 + fposmod(-scroll - cam_d, step)
	while d < fog_end:
		var y := project(0, cam_d + d).y
		var a := 0.16 * (1.0 - clampf((d - fog_start) / (fog_end - fog_start), 0.0, 1.0))
		c.draw_line(Vector2(-2000, y), Vector2(3000, y), Color(0, 0, 0, a))
		d += step
	# lateral lines converging on the horizon
	var x := -x_half
	while x <= x_half:
		c.draw_line(project(x, cam_d + d0), project(x, cam_d + fog_end * 2.0), Color(0, 0, 0, 0.08))
		x += 90.0
	c.draw_line(Vector2(-2000, horizon), Vector2(3000, horizon), Color(0, 0, 0, 0.35), 2.0)
