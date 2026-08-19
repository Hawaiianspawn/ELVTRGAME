class_name View
extends RefCounted
## 2.5D pinhole camera behind and above the army. World = (x lateral, d depth ahead of camera), ground plane.
## screen = (cx + x*s, horizon + cam_h*s), s = focal / d. Sprites scale with s, fog with d.

var horizon := 160.0
var focal := 340.0
var cam_h := 190.0
var cam_x := 0.0
var sprite_k := 1.1           # sprite pixel scale at s == 1
var fog_color := Color("#3a3d3f")
var fog_start := 350.0
var fog_end := 1700.0
var far_color := Color("#4c4e4c")
var sky_color := Color("#8a8c8e")
var sky_low := Color("#5b5d60")
var near_color := Color("#3d3e3c")


func s(d: float) -> float:
	return focal / maxf(d, 20.0)


func project(x: float, d: float) -> Vector2:
	var k := s(d)
	return Vector2(480.0 + (x - cam_x) * k, horizon + cam_h * k)


func sprite_scale(d: float) -> float:
	return s(d) * sprite_k


func fog(d: float) -> Color:
	return Color.WHITE.lerp(fog_color, clampf((d - fog_start) / (fog_end - fog_start), 0.0, 0.85))


## Depth of the ground point under a screen y (inverse of project).
func depth_at_y(y: float) -> float:
	return focal * cam_h / maxf(y - horizon, 1.0)


func x_at(sx: float, d: float) -> float:
	return (sx - 480.0) / s(d) + cam_x


## Sky, fogged ground, receding row lines. `scroll` = forward creep in world units.
func draw_ground(c: CanvasItem, scroll: float, x_half: float) -> void:
	c.draw_rect(Rect2(-2000, -2000, 5000, 2000), sky_color)
	for i in range(8):
		var y0 := horizon * i / 8.0
		c.draw_rect(Rect2(-2000, y0, 5000, horizon / 8.0 + 1), sky_color.lerp(sky_low, (i + 1) / 8.0))
	# ground as depth bands, near = lighter
	var d0 := 60.0
	var steps := 24
	for i in range(steps):
		var t0 := float(i) / steps
		var t1 := float(i + 1) / steps
		var da := d0 * pow(fog_end * 1.5 / d0, t0)
		var db := d0 * pow(fog_end * 1.5 / d0, t1)
		var ya := project(0, da).y
		var yb := project(0, db).y
		var col := near_color.lerp(far_color, clampf((da - fog_start) / (fog_end - fog_start), 0.0, 1.0))
		c.draw_rect(Rect2(-2000, yb, 5000, ya - yb + 1), col)
	# row lines marching toward the camera
	var step := 80.0
	var d := d0 + fposmod(-scroll, step)
	while d < fog_end:
		var y := project(0, d).y
		var a := 0.16 * (1.0 - clampf((d - fog_start) / (fog_end - fog_start), 0.0, 1.0))
		c.draw_line(Vector2(-2000, y), Vector2(3000, y), Color(0, 0, 0, a))
		d += step
	# lateral lines converging on the horizon
	var x := -x_half
	while x <= x_half:
		c.draw_line(project(x, d0), project(x, fog_end * 2.0), Color(0, 0, 0, 0.08))
		x += 90.0
	c.draw_line(Vector2(-2000, horizon), Vector2(3000, horizon), Color(0, 0, 0, 0.35), 2.0)
