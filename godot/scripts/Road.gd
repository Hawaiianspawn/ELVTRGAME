extends Node2D
## Out the door, following the horse into the night. Travellers thicken (undead walkers, fresh knights),
## then oozes rise out of the road. When it's too many to wade through, Jaxx comes back for you.

const SPEED := 60.0

var hero: Node2D
var hero_sprite: Sprite2D
var world: Node2D
var walkers: Array[Node2D] = []
var oozes: Array[Node2D] = []
var t := 0.0
var scroll := 0.0
var spawn_t := 0.0
var ooze_t := 0.0
var touched := 0
var caption := "Follow the horse."
var _rng := RandomNumberGenerator.new()
var _ending := false


func _ready() -> void:
	_rng.randomize()
	world = Node2D.new()
	world.y_sort_enabled = true
	add_child(world)
	hero = Node2D.new()
	hero.position = Vector2(480, 430)
	hero_sprite = Game.make_sprite(Game.hero, 4)
	hero.add_child(hero_sprite)
	world.add_child(hero)


func _walker() -> void:
	var undead := _rng.randf() < 0.6
	var s := Game.make_sprite("undead" if undead else ["veteran", "halberdier", "hammer"][_rng.randi_range(0, 2)], 4)
	if undead:
		s.modulate = Color(0.55, 0.6, 0.55)
	s.position = Vector2(_rng.randf_range(120, 840), 600)
	s.set_meta("v", _rng.randf_range(20, 45))
	world.add_child(s)
	walkers.append(s)


func _ooze() -> void:
	var s := Game.make_sprite("ooze", 0)
	s.position = Vector2(-40.0 if _rng.randf() < 0.5 else 1000.0, _rng.randf_range(150, 500))
	s.scale = Vector2.ONE * 0.2
	create_tween().tween_property(s, "scale", Vector2.ONE, 0.6)
	world.add_child(s)
	oozes.append(s)


func _process(delta: float) -> void:
	if _ending:
		return
	t += delta
	scroll += SPEED * delta
	var mv := Input.get_vector("move_left", "move_right", "move_up", "move_down")
	hero.position += Vector2(mv.x, mv.y * 0.5) * 170.0 * delta
	hero.position = hero.position.clamp(Vector2(60, 300), Vector2(900, 500))
	hero_sprite.frame = 4 if mv == Vector2.ZERO else Game.facing_from(mv)
	# travellers thicken over time
	spawn_t -= delta
	if spawn_t <= 0.0:
		spawn_t = maxf(0.25, 1.6 - t * 0.05)
		_walker()
	for w in walkers.duplicate():
		w.position.y -= (SPEED * 0.4 + float(w.get_meta("v"))) * delta
		w.scale = Vector2.ONE * lerpf(0.4, 1.1, clampf(w.position.y / 540.0, 0, 1))
		if w.position.y < 60:
			walkers.erase(w)
			w.queue_free()
	# oozes rise after the road gets crowded
	if t > 14.0:
		caption = "Ooze. Out of the road itself."
		ooze_t -= delta
		if ooze_t <= 0.0:
			ooze_t = maxf(0.4, 2.0 - (t - 14.0) * 0.1)
			_ooze()
	for o in oozes.duplicate():
		var d: Vector2 = hero.position - o.position
		if d.length() < 30.0:
			touched += 1
			hero_sprite.modulate = Color(2, 1, 1)
			hero.position += Vector2(signf(d.x) * 40, 0)
			oozes.erase(o)
			o.queue_free()
			continue
		o.position += d.normalized() * 55.0 * delta
	hero_sprite.modulate = hero_sprite.modulate.lerp(Color.WHITE, delta * 6.0)
	if oozes.size() >= 7 or touched >= 4 or t > 40.0:
		_jaxx_arrives()
	queue_redraw()


func _unhandled_input(e: InputEvent) -> void:
	if e.is_action_pressed("cast") and Game.magic >= 8.0 and not _ending:
		Game.magic -= 8.0
		var cur := get_global_mouse_position()
		for o in oozes.duplicate():
			if o.position.distance_to(cur) < 50.0:
				oozes.erase(o)
				o.queue_free()
				Game.gain_magic(4.0)


func _jaxx_arrives() -> void:
	_ending = true
	caption = "Too many. Then - hooves."
	var j := Game.make_sprite("horse_jaxx", 0)
	j.position = Vector2(480, -60)
	j.scale = Vector2.ONE * 0.5
	world.add_child(j)
	var tw := create_tween().set_parallel(true)
	tw.tween_property(j, "position", hero.position + Vector2(0, -20), 1.6)
	tw.tween_property(j, "scale", Vector2.ONE * 1.2, 1.6)
	await tw.finished
	caption = "Jaxx. He came back for the flame."
	await get_tree().create_timer(1.8).timeout
	Game.goto("ride")


func _draw() -> void:
	draw_rect(Rect2(0, 0, 960, 540), Color("#16151a"))
	draw_rect(Rect2(300, 0, 360, 540), Color("#2b2b2e"))                 # road
	for i in range(12):
		var y := fposmod(i * 50.0 + scroll, 600.0) - 30.0
		draw_rect(Rect2(474, y, 12, 26), Color("#3a3d3f"))
	draw_circle(hero.position + Vector2(0, -60), 4.0 + minf(Game.magic, 200.0) * 0.06, Color(0.4, 1.0, 0.4, 0.8))
	draw_string(ThemeDB.fallback_font, Vector2(12, 20), caption, HORIZONTAL_ALIGNMENT_LEFT, -1, 16, Color("#e9efec"))
	draw_string(ThemeDB.fallback_font, Vector2(12, 530), "A/D sidestep   LMB bolt (8 magic)   MAGIC %d" % int(Game.magic), HORIZONTAL_ALIGNMENT_LEFT, -1, 14, Color("#a0a08b"))
