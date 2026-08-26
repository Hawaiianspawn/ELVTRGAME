extends Node3D
## Opening: the siege of the necromancer's castle. Green light and smoke pour from the keep.
## The lens flies over the battlefield for 3 s to the main gate; the gate opens; the hall begins.
## Real 3D scene (task-161, same port Battle.gd got in task-158 — see docs/qa/hall-3d.md). World
## units 1:1 with the sim: a point at (wx, wd) sits at Vector3(wx, 0, -wd). Same lens as Battle,
## reused via BattleScript's constants rather than duplicated by eye; Hall3D holds the shared
## sprite factory + projection helpers (class_name Hall3D, not World3D — see hall-3d.md).

const BattleScript := preload("res://scripts/Battle.gd")

const FLY := 3.0
# facade.png is cropped tight to its art (no padding) at this native size, 16:9.
const FACADE_IMG_W := 320.0
const FACADE_IMG_H := 180.0
const FACADE_WORLD_H := 1008.0                              # world units tall at depth GATE_D
const FACADE_WORLD_W := FACADE_WORLD_H * FACADE_IMG_W / FACADE_IMG_H
const FACADE_PX := FACADE_WORLD_H / FACADE_IMG_H            # world units per facade texture pixel
# the baked portcullis opening's pixel bounds within facade.png, measured once in Pillow
# (RawArt/Renders/castle-gate/facade-final.png before crop, offset -32,-18 to match the crop).
const ARCH_PX := Rect2(126.0, 98.0, 68.0, 70.0)
const GATE_D := 2000.0             # facade depth, world units ahead of the camera's start position

var camera: Camera3D
var world: Node3D
var _t := 0.0
var _gate := 0.0                 # 0 shut .. 1 open
var _rng := RandomNumberGenerator.new()
var _smoke: Array = []           # {x, y, r, a}, facade-relative screen-space overlay puffs
var caption: Label
var _door_l: Sprite3D
var _door_r: Sprite3D
var _glow: Sprite3D
var _arch_half_w := 0.0
var _arch_h := 0.0
var _arch_cx := 0.0
var _arch_cy := 0.0
var smoke_layer: Node2D


func _ready() -> void:
	_rng.randomize()
	world = Node3D.new()
	add_child(world)
	var hall := Hall3D.new()
	world.add_child(hall)
	hall._build_floor(600.0)
	_build_env()
	camera = Camera3D.new()
	camera.position = Vector3(0.0, BattleScript.CAM_H, 0.0)
	camera.fov = rad_to_deg(2.0 * atan(BattleScript.HALF_H / BattleScript.FOCAL))
	camera.keep_aspect = Camera3D.KEEP_HEIGHT
	camera.rotation_degrees.x = BattleScript.pitch_for(BattleScript.HORIZON)
	camera.near = 1.0
	camera.far = 4000.0
	camera.current = true
	add_child(camera)
	_build_facade()
	# the besieging army below the lens: ranks of ours facing the gate. Real 3D, so no per-frame
	# projection/scale/fog upkeep needed (that was the whole old view.project/sprite_scale/fog
	# song and dance) — placed once, the camera and Environment fog do the rest every frame.
	for i in range(90):
		var s := Hall3D.make_sprite3d(["veteran", "halberdier", "hammer", "vet_ranged"][_rng.randi_range(0, 3)], 4)
		s.position = Hall3D.to_world(_rng.randf_range(-520.0, 520.0), _rng.randf_range(120.0, 1500.0))
		world.add_child(s)
	for i in range(40):
		_smoke.append({"x": _rng.randf_range(-40, 40), "y": _rng.randf_range(0, 260), "r": _rng.randf_range(10, 34), "a": _rng.randf_range(0.1, 0.4)})
	var ov := CanvasLayer.new()
	add_child(ov)
	smoke_layer = Node2D.new()
	smoke_layer.draw.connect(_draw_smoke)
	ov.add_child(smoke_layer)
	var cl := CanvasLayer.new()
	add_child(cl)
	caption = Label.new()
	caption.position = Vector2(0, 440)
	caption.size = Vector2(960, 80)
	caption.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	caption.add_theme_font_size_override("font_size", 22)
	caption.add_theme_color_override("font_color", Color("#e9efec"))
	cl.add_child(caption)
	_run()


## Black sky-to-fog Environment where the old draw_ground sky gradient used to live: flat
## background colour plus the same depth fog range the old 2D fog lerp used, now applied by the renderer.
func _build_env() -> void:
	var we := WorldEnvironment.new()
	var e := Environment.new()
	e.background_mode = Environment.BG_COLOR
	e.background_color = Color("#1a221c")
	e.ambient_light_source = Environment.AMBIENT_SOURCE_COLOR
	e.ambient_light_color = Color.WHITE
	e.ambient_light_energy = 1.0
	e.fog_enabled = true
	e.fog_mode = Environment.FOG_MODE_DEPTH
	e.fog_light_color = Color("#0a0d0b")
	e.fog_density = 1.0
	e.fog_depth_begin = Hall3D.FOG_START
	e.fog_depth_end = Hall3D.FOG_END
	e.fog_depth_curve = 1.0
	e.fog_sky_affect = 0.0
	we.environment = e
	add_child(we)


## The castle facade quad standing at depth GATE_D, the arch cut into it (glow just behind, two
## doors just in front that squash toward their hinge as _gate opens).
func _build_facade() -> void:
	var facade := Sprite3D.new()
	facade.texture = load("res://assets/env/castle/facade.png")
	facade.pixel_size = FACADE_PX
	facade.texture_filter = BaseMaterial3D.TEXTURE_FILTER_NEAREST
	facade.alpha_cut = SpriteBase3D.ALPHA_CUT_DISCARD
	facade.shaded = false
	facade.billboard = BaseMaterial3D.BILLBOARD_DISABLED
	facade.position = Vector3(0.0, FACADE_WORLD_H * 0.5, -GATE_D)
	world.add_child(facade)
	# arch rect in facade-pixel space (origin top-left, y-down) -> world offsets from facade centre
	_arch_half_w = ARCH_PX.size.x * 0.5 * FACADE_PX
	_arch_h = ARCH_PX.size.y * FACADE_PX
	_arch_cx = -FACADE_WORLD_W * 0.5 + (ARCH_PX.position.x + ARCH_PX.size.x * 0.5) * FACADE_PX
	var arch_top_y := FACADE_WORLD_H - ARCH_PX.position.y * FACADE_PX
	_arch_cy = arch_top_y - _arch_h * 0.5
	var glow_tex: Texture2D = load("res://assets/env/castle/hall_glow.png")
	_glow = Sprite3D.new()
	_glow.texture = glow_tex
	_glow.pixel_size = _arch_h / glow_tex.get_height()
	_glow.scale.x = (_arch_half_w * 2.0) / (glow_tex.get_width() * _glow.pixel_size)
	_glow.texture_filter = BaseMaterial3D.TEXTURE_FILTER_NEAREST
	_glow.shaded = false
	_glow.billboard = BaseMaterial3D.BILLBOARD_DISABLED
	_glow.modulate.a = 0.35
	_glow.position = Vector3(_arch_cx, _arch_cy, -(GATE_D + 4.0))
	world.add_child(_glow)
	_door_l = _door(load("res://assets/env/castle/door_l.png"))
	_door_r = _door(load("res://assets/env/castle/door_r.png"))
	_update_gate()


func _door(tex: Texture2D) -> Sprite3D:
	var s := Sprite3D.new()
	s.texture = tex
	s.pixel_size = _arch_h / tex.get_height()   # full arch height; width is stretched per-frame
	s.texture_filter = BaseMaterial3D.TEXTURE_FILTER_NEAREST
	s.shaded = false
	s.billboard = BaseMaterial3D.BILLBOARD_DISABLED
	s.centered = true
	world.add_child(s)
	return s


## Doors squash toward their hinge (the arch's outer edges) as _gate goes 0..1 — the same
## width-stretch trick the old 2D draw used (draw_texture_rect ignoring source aspect), done here
## with a fixed-height pixel_size plus a per-frame scale.x and reposition so the hinge edge holds.
func _update_gate() -> void:
	var door_w := _arch_half_w * (1.0 - _gate)
	for side: float in [-1.0, 1.0]:
		var d: Sprite3D = _door_l if side < 0.0 else _door_r
		var natural_w: float = d.texture.get_width() * d.pixel_size
		d.scale.x = door_w / natural_w if natural_w > 0.0 else 0.0
		d.position = Vector3(_arch_cx + side * (_arch_half_w - door_w * 0.5), _arch_cy, -(GATE_D - 4.0))
		d.visible = door_w > 0.25
	_glow.modulate.a = 0.35 + 0.55 * _gate


func _run() -> void:
	caption.text = "The Necromancer's Keep."
	# fly: the lens closes 1350 world units on the gate in FLY seconds
	var t := create_tween()
	t.tween_property(camera, "position:z", -1350.0, FLY).set_trans(Tween.TRANS_SINE).set_ease(Tween.EASE_IN_OUT)
	await t.finished
	caption.text = "The gate opens."
	var g := create_tween()
	g.tween_property(self, "_gate", 1.0, 1.6).set_trans(Tween.TRANS_CUBIC).set_ease(Tween.EASE_IN_OUT)
	await g.finished
	await get_tree().create_timer(0.5).timeout
	Game.goto("battle")


func _unhandled_input(e: InputEvent) -> void:
	if e.is_action_pressed("skip") or e.is_action_pressed("advance"):
		set_process_unhandled_input(false)
		Game.goto("battle")


func _process(delta: float) -> void:
	_t += delta
	_update_gate()
	smoke_layer.queue_redraw()
	for m in _smoke:
		m["y"] -= delta * 40.0
		m["x"] += sin(_t + m["y"] * 0.03) * delta * 12.0
		if m["y"] < -60.0:
			m["y"] = 260.0
			m["x"] = _rng.randf_range(-40, 40)


## Green smoke pours from the keep roofline — still a screen-space overlay (parity with the old
## 2D draw), unprojected each frame so it tracks the facade's screen position as the lens flies in.
func _draw_smoke() -> void:
	var base := Hall3D.unproject(camera, 0.0, GATE_D)
	var k := Hall3D.screen_scale(camera, GATE_D)
	var keep_top := base.y - FACADE_WORLD_H * k
	for m in _smoke:
		smoke_layer.draw_circle(Vector2(base.x + m["x"] * k * 4.0, keep_top + m["y"] * k * 2.0 - 120.0 * k), m["r"] * k * 2.5, Color(0.3, 0.8, 0.4, m["a"] * 0.5))
