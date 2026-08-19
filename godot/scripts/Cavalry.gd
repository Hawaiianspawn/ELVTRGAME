extends Node2D
## Scripted, same View as the battle: cavalry charges from behind the camera, kills the Necromancer,
## one rider falls, both bodies burn, riders leave into the background, Jaxx walks out the bottom of frame.
## Every actor carries meta wx/wd (world) and is re-projected each frame; tweens drive the world coords.

var view := View.new()
var army: Array[Unit] = []
var advancing := true
const CREEP := 14.0
var caption: Label
var actors: Array[Node2D] = []
var necro: Sprite2D
var riders: Array[Node2D] = []
var jaxx: Node2D
var fallen: Sprite2D
var lightning := true
var scroll := 0.0
var _rng := RandomNumberGenerator.new()


func _actor(n: Node2D, wx: float, wd: float, k := 1.0) -> Node2D:
	n.set_meta("wx", wx)
	n.set_meta("wd", wd)
	n.set_meta("k", k)
	get_child(0).add_child(n)
	actors.append(n)
	return n


func _sprite(name: String, facing: int, wx: float, wd: float, k := 1.0, tint := Color.WHITE) -> Sprite2D:
	var s := Game.make_sprite(name, facing)
	s.self_modulate = tint
	_actor(s, wx, wd, k)
	return s


func _ready() -> void:
	_rng.randomize()
	view.cam_h = 230.0
	view.focal = 290.0
	var world := Node2D.new()
	world.y_sort_enabled = true
	add_child(world)
	# our ranks as the battle left them: the real block, still pushing
	army = Army.block(self, world, Game.waves[3]["reserves"], 476.0, 7, 285.0, _rng)
	for i in range(9):
		var f := Unit.new()
		f.setup(["veteran", "halberdier", "hammer", "sheathed", "vet_ranged"][i % 5], Unit.ALLY, self)
		f.state = Unit.State.RANK
		f.wx = (i - 4) * 64.0
		f.wd = 320.0
		world.add_child(f)
		army.append(f)
	# the tank wall remnants and the necromancer
	for i in range(8):
		var s := _sprite("armored", 0, (i - 3.5) * 90.0 + _rng.randf_range(-15, 15), 400.0 + _rng.randf_range(-15, 15))
		s.set_meta("enemy", true)
	necro = _sprite("undead", 0, 0.0, 520.0, 1.6, Color(0.6, 1.0, 0.6))
	_sprite("mage", 4, 0.0, 245.0)
	# riders: horse + small rider sprite, start behind the camera
	var coats := ["horse_clyde", "horse_white", "horse_jaxx", "horse_clyde", "horse_white"]
	for i in range(5):
		var r := Node2D.new()
		var h := Game.make_sprite(coats[i], 4)
		r.add_child(h)
		var k := Game.make_sprite("hammer", 4)
		k.scale = Vector2.ONE * 0.6
		k.position = Vector2(0, -34)
		r.add_child(k)
		r.set_meta("horse", h)
		r.set_meta("rider", k)
		_actor(r, (i - 2) * 110.0, 30.0)
		riders.append(r)
	jaxx = riders[2]
	fallen = jaxx.get_meta("rider")
	if Game.charged:
		# resume from the circle: wall already broken, riders ringed around him, Jaxx nearest the camera
		for a in actors.duplicate():
			if a.has_meta("enemy"):
				actors.erase(a)
				a.queue_free()
		for i in range(riders.size()):
			var ang := PI * 1.5 + (i - 2) * TAU / 5.0
			riders[i].set_meta("wx", cos(ang) * 150.0)
			riders[i].set_meta("wd", 520.0 + sin(ang) * 150.0 * 0.55)
			(riders[i].get_meta("horse") as Sprite2D).frame = 4 if i != 2 else 0
			(riders[i].get_meta("rider") as Sprite2D).frame = 4 if i != 2 else 0
	var cl := CanvasLayer.new()
	add_child(cl)
	caption = Label.new()
	caption.position = Vector2(0, 30)
	caption.size = Vector2(960, 40)
	caption.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	caption.add_theme_font_size_override("font_size", 20)
	caption.add_theme_color_override("font_color", Color("#e9efec"))
	cl.add_child(caption)
	_run()


func _move(n: Node2D, wx: float, wd: float, secs: float, delay := 0.0) -> Tween:
	var from := Vector2(n.get_meta("wx"), n.get_meta("wd"))
	var t := create_tween()
	t.tween_method(func(p: Vector2): n.set_meta("wx", p.x); n.set_meta("wd", p.y), from, Vector2(wx, wd), secs).set_delay(delay).set_trans(Tween.TRANS_QUAD).set_ease(Tween.EASE_OUT)
	return t


func _say(t: String, secs: float) -> void:
	caption.text = t
	await get_tree().create_timer(secs).timeout


func _run() -> void:
	if Game.charged:
		await _say("The killing stroke.", 1.2)
	else:
		await _say("The line holds. Barely.", 2.0)
		await _say("Hooves. Behind you.", 1.5)
		# charge: from behind the camera, through the wall, onto the necromancer
		for i in range(riders.size()):
			_move(riders[i], (i - 2) * 95.0, 470.0, 2.6, i * 0.15)
		await get_tree().create_timer(1.6).timeout
		for a in actors:
			if a.has_meta("enemy"):
				var t := create_tween()
				t.tween_property(a, "modulate", Color(2, 2, 2), 0.1)
				t.tween_property(a, "modulate:a", 0.0, 0.5)
		await get_tree().create_timer(1.2).timeout
	caption.text = ""
	# the necromancer dies
	necro.modulate = Color(3, 3, 3)
	lightning = false
	var nt := create_tween()
	nt.tween_property(necro, "rotation", PI / 2.0, 0.6)
	nt.parallel().tween_property(necro, "modulate", Color(0.3, 0.3, 0.3), 0.6)
	await nt.finished
	await _say("The green dot goes out.", 1.5)
	# one rider falls
	var ft := create_tween()
	ft.tween_property(fallen, "position", Vector2(30, 0), 0.5)
	ft.parallel().tween_property(fallen, "rotation", PI / 2.0, 0.5)
	await ft.finished
	await _say("...", 2.5)
	# riders turn to face the body, take a moment
	for r in riders:
		(r.get_meta("horse") as Sprite2D).frame = 0
		if r != jaxx:
			(r.get_meta("rider") as Sprite2D).frame = 0
	await _say("They burn both. Nothing left to reincarnate.", 1.0)
	_pyre(necro)
	_pyre(jaxx, Vector2(30, 0))
	await get_tree().create_timer(2.5).timeout
	create_tween().tween_property(necro, "modulate:a", 0.0, 1.5)
	create_tween().tween_property(fallen, "modulate:a", 0.0, 1.5)
	await get_tree().create_timer(2.0).timeout
	# riders leave into the background; Jaxx stays
	for r in riders:
		if r == jaxx:
			continue
		(r.get_meta("horse") as Sprite2D).frame = 4
		(r.get_meta("rider") as Sprite2D).frame = 4
		_move(r, float(r.get_meta("wx")) * 1.5, 1500.0, 4.0)
		create_tween().tween_property(r, "modulate:a", 0.0, 4.0)
	await _say("", 3.5)
	# fire out. Jaxx walks into the camera and out the bottom of frame.
	(jaxx.get_meta("horse") as Sprite2D).frame = 0
	await _say("The blue roan. His name was Jaxx.", 1.5)
	await _move(jaxx, 40.0, 60.0, 4.5).finished
	Game.goto("messhall")


func _pyre(host: Node2D, off := Vector2.ZERO) -> void:
	var f := CPUParticles2D.new()
	f.position = off
	f.amount = 60
	f.lifetime = 1.2
	f.direction = Vector2.UP
	f.spread = 20.0
	f.initial_velocity_min = 40.0
	f.initial_velocity_max = 90.0
	f.gravity = Vector2(0, -30)
	f.scale_amount_min = 3.0
	f.scale_amount_max = 6.0
	f.color = Color(1.0, 0.6, 0.2)
	f.emission_shape = CPUParticles2D.EMISSION_SHAPE_SPHERE
	f.emission_sphere_radius = 14.0
	f.z_index = 10
	host.add_child(f)
	var t := create_tween()
	t.tween_interval(4.0)
	t.tween_callback(func(): f.emitting = false)


func lane_x(_l: int) -> float:
	return 0.0


func _process(delta: float) -> void:
	scroll += 6.0 * delta
	for a in actors:
		var wd: float = a.get_meta("wd")
		a.position = view.project(a.get_meta("wx"), wd)
		a.scale = Vector2.ONE * view.sprite_scale(wd) * float(a.get_meta("k"))
		a.modulate = view.fog(wd) if a.modulate.a >= 1.0 else a.modulate
	queue_redraw()


func _draw() -> void:
	view.draw_ground(self, scroll, 700.0)
	if lightning:
		var flick := 0.6 + 0.4 * sin(Time.get_ticks_msec() / 90.0)
		var dot := Vector2(480, view.horizon - 8)
		draw_circle(dot, 8.0 + 4.0 * flick, Color(0.4, 1.0, 0.4, flick))
		draw_line(dot, dot + Vector2(_rng.randf_range(-40, 40), -110), Color(0.5, 1.0, 0.5, 0.5 * flick), 2.0)


func _unhandled_input(e: InputEvent) -> void:
	if e.is_action_pressed("skip"):
		set_process_unhandled_input(false)
		Game.goto("messhall")
