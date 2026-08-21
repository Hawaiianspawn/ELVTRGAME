extends Node2D
## Opening: the siege of the necromancer's castle. Green light and smoke pour from the keep.
## The lens flies over the battlefield for 3 s to the main gate; the gate opens; the hall begins.

const FLY := 3.0
# facade.png is cropped tight to its art (no padding) at this native size, 16:9.
const FACADE_IMG_W := 320.0
const FACADE_IMG_H := 180.0
const FACADE_WORLD_H := 1008.0                              # world units tall at depth gd
const FACADE_WORLD_W := FACADE_WORLD_H * FACADE_IMG_W / FACADE_IMG_H
# the baked portcullis opening's pixel bounds within facade.png, measured once in Pillow
# (RawArt/Renders/castle-gate/facade-final.png before crop, offset -32,-18 to match the crop).
const ARCH_PX := Rect2(126.0, 98.0, 68.0, 70.0)

var view := View.new()
var _t := 0.0
var _gate := 0.0                 # 0 shut .. 1 open
var _rng := RandomNumberGenerator.new()
var _smoke: Array = []           # {x, y, r, a}
var _army: Array[Sprite2D] = []
var caption: Label
var _facade_tex: Texture2D
var _door_l_tex: Texture2D
var _door_r_tex: Texture2D
var _glow_tex: Texture2D


func _ready() -> void:
	_rng.randomize()
	_facade_tex = load("res://assets/env/castle/facade.png")
	_door_l_tex = load("res://assets/env/castle/door_l.png")
	_door_r_tex = load("res://assets/env/castle/door_r.png")
	_glow_tex = load("res://assets/env/castle/hall_glow.png")
	view.horizon = 250.0
	view.cam_h = 200.0
	view.sprite_k = 2.2
	view.sky_color = Color("#1a221c")
	view.sky_low = Color("#0a0d0b")
	var world := Node2D.new()
	world.y_sort_enabled = true
	add_child(world)
	# the besieging army below the lens: ranks of ours facing the gate
	for i in range(90):
		var s := Game.make_sprite(["veteran", "halberdier", "hammer", "vet_ranged"][_rng.randi_range(0, 3)], 4)
		s.set_meta("wx", _rng.randf_range(-520.0, 520.0))
		s.set_meta("wd", _rng.randf_range(120.0, 1500.0))
		world.add_child(s)
		_army.append(s)
	for i in range(40):
		_smoke.append({"x": _rng.randf_range(-40, 40), "y": _rng.randf_range(0, 260), "r": _rng.randf_range(10, 34), "a": _rng.randf_range(0.1, 0.4)})
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


func _run() -> void:
	caption.text = "The Necromancer's Keep."
	# fly: the lens closes 1350 world units on the gate in FLY seconds
	var t := create_tween()
	t.tween_property(view, "cam_d", 1350.0, FLY).set_trans(Tween.TRANS_SINE).set_ease(Tween.EASE_IN_OUT)
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
	queue_redraw()
	for s in _army:
		var wd: float = s.get_meta("wd")
		s.visible = wd - view.cam_d > 40.0
		s.position = view.project(s.get_meta("wx"), wd)
		s.scale = Vector2.ONE * view.sprite_scale(wd)
		s.modulate = view.fog(wd)
	for m in _smoke:
		m["y"] -= delta * 40.0
		m["x"] += sin(_t + m["y"] * 0.03) * delta * 12.0
		if m["y"] < -60.0:
			m["y"] = 260.0
			m["x"] = _rng.randf_range(-40, 40)


func _draw() -> void:
	view.draw_ground(self, 0.0, 600.0)
	# castle: a block at depth 2000, drawn from the painted facade (walls, towers, ruined
	# battlements and the gate arch are all baked into the art now). Bottom edge of the
	# texture is flush with its own art (no padding), so pinning it to base.y pins the
	# flagstone threshold to the projected ground line exactly.
	var gd := 2000.0
	var k := view.s(gd)
	var base := view.project(0.0, gd)
	var f := view.fog(gd)
	var facade_w := FACADE_WORLD_W * k
	var facade_h := FACADE_WORLD_H * k
	var fx0 := base.x - facade_w * 0.5
	var fy0 := base.y - facade_h
	draw_texture_rect(_facade_tex, Rect2(fx0, fy0, facade_w, facade_h), false, f)
	# smoke still pours from the keep roofline
	var keep_top := fy0
	for m in _smoke:
		draw_circle(Vector2(base.x + m["x"] * k * 4.0, keep_top + m["y"] * k * 2.0 - 120.0 * k), m["r"] * k * 2.5, Color(0.3, 0.8, 0.4, m["a"] * 0.5))
	# gate: the baked portcullis opening's pixel rect, mapped into the same facade
	# transform, filled with the hall glow then two doors that squash toward their
	# hinge (the arch's outer edges) as _gate goes 0..1 — reads as a swing in this
	# projection, and sits exactly over the bars baked into the art.
	var sx := facade_w / FACADE_IMG_W
	var sy := facade_h / FACADE_IMG_H
	var arch_x0 := fx0 + ARCH_PX.position.x * sx
	var arch_y0 := fy0 + ARCH_PX.position.y * sy
	var arch_w := ARCH_PX.size.x * sx
	var arch_h := ARCH_PX.size.y * sy
	draw_texture_rect(_glow_tex, Rect2(arch_x0, arch_y0, arch_w, arch_h), false, Color(1.0, 1.0, 1.0, 0.35 + 0.55 * _gate))
	var half := arch_w * 0.5
	var door := half * (1.0 - _gate)
	if door > 0.5:
		draw_texture_rect(_door_l_tex, Rect2(arch_x0, arch_y0, door, arch_h), false, f)
		draw_texture_rect(_door_r_tex, Rect2(arch_x0 + arch_w - door, arch_y0, door, arch_h), false, f)
