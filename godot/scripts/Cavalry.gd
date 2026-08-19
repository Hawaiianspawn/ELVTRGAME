extends Node2D
## Scripted: cavalry charges from behind the camera, kills the Necromancer, one rider falls,
## both bodies burn, riders leave into the background, Jaxx walks out the bottom of frame.

var caption: Label
var necro: Sprite2D
var riders: Array[Node2D] = []
var jaxx: Node2D
var fallen: Sprite2D
var lightning := true
var _rng := RandomNumberGenerator.new()


func _ready() -> void:
	_rng.randomize()
	var world := Node2D.new()
	world.y_sort_enabled = true
	add_child(world)
	# survivors of the tank wall
	for i in range(9):
		var s := Game.make_sprite(["shield", "pike", "archer", "greatsword"][i % 4], 4)
		s.position = Vector2(80 + i * 100, 300)
		s.scale = Vector2.ONE * 1.6
		world.add_child(s)
	for i in range(6):
		var s := Game.make_sprite("armored", 0)
		s.position = Vector2(150 + i * 130 + _rng.randf_range(-20, 20), 210 + _rng.randf_range(-20, 20))
		s.scale = Vector2.ONE * 1.25
		s.set_meta("enemy", true)
		world.add_child(s)
	necro = Game.make_sprite("undead", 0)
	necro.position = Vector2(480, 130)
	necro.scale = Vector2.ONE * 1.5
	necro.modulate = Color(0.6, 1.0, 0.6)
	world.add_child(necro)
	var hero := Game.make_sprite("mage", 4)
	hero.position = Vector2(480, 440)
	hero.scale = Vector2.ONE * 2.2
	world.add_child(hero)
	# riders: horse + small rider sprite on top
	var coats := ["horse_clyde", "horse_white", "horse_jaxx", "horse_clyde", "horse_white"]
	for i in range(5):
		var r := Node2D.new()
		var h := Game.make_sprite(coats[i], 4)
		r.add_child(h)
		var k := Game.make_sprite("greatsword", 4)
		k.scale = Vector2.ONE * 0.6
		k.position = Vector2(0, -34)
		r.add_child(k)
		r.position = Vector2(200 + i * 140, 700)
		r.scale = Vector2.ONE * 3.6
		r.set_meta("horse", h)
		r.set_meta("rider", k)
		world.add_child(r)
		riders.append(r)
	jaxx = riders[2]
	fallen = jaxx.get_meta("rider")
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


func _say(t: String, secs: float) -> void:
	caption.text = t
	await get_tree().create_timer(secs).timeout


func _run() -> void:
	await _say("The line holds. Barely.", 2.0)
	await _say("Hooves. Behind you.", 1.5)
	# charge: from behind the camera, through the wall, onto the necromancer
	for i in range(riders.size()):
		var r := riders[i]
		var t := create_tween().set_parallel(true)
		t.tween_property(r, "position", Vector2(260 + i * 110, 175), 2.4).set_delay(i * 0.15).set_trans(Tween.TRANS_QUAD).set_ease(Tween.EASE_OUT)
		t.tween_property(r, "scale", Vector2.ONE * 1.3, 2.4).set_delay(i * 0.15)
	await get_tree().create_timer(1.4).timeout
	for c in get_children()[0].get_children():
		if c.has_meta("enemy"):
			var t := create_tween()
			t.tween_property(c, "modulate", Color(2, 2, 2), 0.1)
			t.tween_property(c, "modulate:a", 0.0, 0.5)
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
	_pyre(necro.position)
	_pyre(jaxx.position + Vector2(30, 0))
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
		var t := create_tween().set_parallel(true)
		t.tween_property(r, "position:y", -40.0, 3.0)
		t.tween_property(r, "scale", Vector2.ONE * 0.5, 3.0)
		t.tween_property(r, "modulate:a", 0.0, 3.0)
	await _say("", 3.5)
	# fire out. Jaxx walks into the camera and out the bottom of frame.
	(jaxx.get_meta("horse") as Sprite2D).frame = 0
	await _say("The blue roan. His name was Jaxx.", 1.5)
	var jt := create_tween().set_parallel(true)
	jt.tween_property(jaxx, "position", Vector2(520, 720), 4.0)
	jt.tween_property(jaxx, "scale", Vector2.ONE * 4.5, 4.0)
	await jt.finished
	Game.goto("messhall")


func _pyre(p: Vector2) -> void:
	var f := CPUParticles2D.new()
	f.position = p
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
	add_child(f)
	var t := create_tween()
	t.tween_interval(4.0)
	t.tween_callback(func(): f.emitting = false)


func _process(_d: float) -> void:
	queue_redraw()


func _draw() -> void:
	draw_rect(Rect2(-100, -200, 1200, 240), Color("#211e20"))
	draw_rect(Rect2(-100, 40, 1200, 260), Color("#3a3d3f"))
	draw_rect(Rect2(-100, 300, 1200, 400), Color("#4a4b48"))
	if lightning:
		var flick := 0.6 + 0.4 * sin(Time.get_ticks_msec() / 90.0)
		draw_circle(Vector2(480, 24), 8.0 + 4.0 * flick, Color(0.4, 1.0, 0.4, flick))
		draw_line(Vector2(480, 24), Vector2(480 + _rng.randf_range(-40, 40), -80), Color(0.5, 1.0, 0.5, 0.5 * flick), 2.0)


func _unhandled_input(e: InputEvent) -> void:
	if e.is_action_pressed("skip"):
		set_process_unhandled_input(false)
		Game.goto("messhall")
