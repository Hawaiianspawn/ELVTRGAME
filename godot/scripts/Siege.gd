extends Node2D
## Opening: the siege of the necromancer's castle. Green light and smoke pour from the keep.
## The lens flies over the battlefield for 3 s to the main gate; the gate opens; the hall begins.

const FLY := 3.0
const GATE_W := 200.0
const GATE_H := 240.0

var view := View.new()
var _t := 0.0
var _gate := 0.0                 # 0 shut .. 1 open
var _rng := RandomNumberGenerator.new()
var _smoke: Array = []           # {x, y, r, a}
var _army: Array[Sprite2D] = []
var caption: Label


func _ready() -> void:
	_rng.randomize()
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
	# castle: a block at depth 2000 with the gate in the middle, green light spilling from the keep
	var gd := 2000.0
	var k := view.s(gd)
	var base := view.project(0.0, gd)
	var wall_h := 420.0 * k
	var half_w := 900.0 * k
	var f := view.fog(gd)
	var stone := Color("#2b2b30") * f
	draw_rect(Rect2(base.x - half_w, base.y - wall_h, half_w * 2.0, wall_h), stone)
	# towers
	for tx: float in [-700.0, 700.0]:
		draw_rect(Rect2(base.x + tx * k - 90.0 * k, base.y - wall_h * 1.5, 180.0 * k, wall_h * 1.5), stone * 0.9)
	# the keep behind, lit green from within
	var keep_top := base.y - wall_h * 2.4
	draw_rect(Rect2(base.x - 260.0 * k, keep_top, 520.0 * k, wall_h * 2.4), Color("#1f2523") * f)
	var flick := 0.7 + 0.3 * sin(_t * 9.0)
	draw_circle(Vector2(base.x, keep_top), 140.0 * k * flick, Color(0.35, 1.0, 0.45, 0.35 * flick))
	for i in range(4):
		draw_line(Vector2(base.x, keep_top), Vector2(base.x + _rng.randf_range(-300, 300) * k, keep_top - 600.0 * k), Color(0.5, 1.0, 0.5, 0.3 * flick), 2.0)
	for m in _smoke:
		draw_circle(Vector2(base.x + m["x"] * k * 4.0, keep_top + m["y"] * k * 2.0 - 120.0 * k), m["r"] * k * 2.5, Color(0.3, 0.8, 0.4, m["a"] * 0.5))
	# gate: two doors swinging inward, hall glow behind
	var gw := GATE_W * k
	var gh := GATE_H * k
	draw_rect(Rect2(base.x - gw, base.y - gh, gw * 2.0, gh), Color(0.2, 0.9, 0.4, 0.55 * _gate))
	var door := gw * (1.0 - _gate)
	draw_rect(Rect2(base.x - gw, base.y - gh, door, gh), Color("#3a2a1c") * f)
	draw_rect(Rect2(base.x + gw - door, base.y - gh, door, gh), Color("#3a2a1c") * f)
	draw_rect(Rect2(base.x - gw, base.y - gh, gw * 2.0, gh), Color(0, 0, 0, 0.6), false, 2.0)
