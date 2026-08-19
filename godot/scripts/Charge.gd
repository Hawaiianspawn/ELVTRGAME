extends Node2D
## Playable bridge between wave 4 and the pyre. You are Jaxx, the blue roan, carrying the hero.
## CHARGE: auto-forward, gaining ground every second. A/D steer, Space surge. Plow through the tank wall
##         with four riders beside you until the Necromancer is close.
## CIRCLE: the riders orbit him. A/D speed or slow your orbit to dodge his telegraphed bolts; when he
##         channels (bright green), Space lunges in for a strike. Three strikes, then the story takes over.

enum Phase { CHARGE, CIRCLE, DONE }

const RIDER_D := 210.0            # player depth ahead of the camera during the charge
const NECRO_D := 3400.0
const WALL_X := 300.0
const ORBIT_R := 150.0
const STRIKES := 3

var view := View.new()
var army: Array[Unit] = []
var advancing := true
const CREEP := 14.0
var phase := Phase.CHARGE
var world: Node2D
var hud: Node2D
var actors: Array[Node2D] = []     # meta wx / wd / k
var enemies: Array[Node2D] = []
var riders: Array[Node2D] = []     # 0 = player
var necro: Sprite2D
var caption := ""
var caption_t := 0.0
var scroll := 0.0
var t := 0.0
# charge
var px := 0.0
var speed := 140.0
var surge_t := -9.0
var hits := 0
var hp := 100.0
# circle
var angle := PI * 1.5             # player orbit angle; PI*1.5 = nearest the camera
var omega := 1.1
var bolt_t := 2.5
var bolt_angle := 0.0
var bolt_phase := 0.0             # >0 telegraph countdown, <0 idle
var channel_t := 4.0
var channel_open := 0.0
var strikes := 0
var lunge := 0.0
var _rng := RandomNumberGenerator.new()


func _actor(n: Node2D, wx: float, wd: float, k := 1.0) -> Node2D:
	n.set_meta("wx", wx)
	n.set_meta("wd", wd)
	n.set_meta("k", k)
	world.add_child(n)
	actors.append(n)
	return n


func _sprite(name: String, facing: int, wx: float, wd: float, k := 1.0, tint := Color.WHITE) -> Sprite2D:
	var s := Game.make_sprite(name, facing)
	s.self_modulate = tint
	_actor(s, wx, wd, k)
	return s


func _rider(coat: String, wx: float, wd: float) -> Node2D:
	var r := Node2D.new()
	var h := Game.make_sprite(coat, 4)
	r.add_child(h)
	var k := Game.make_sprite("hammer", 4)
	k.scale = Vector2.ONE * 0.6
	k.position = Vector2(0, -34)
	r.add_child(k)
	r.set_meta("horse", h)
	r.set_meta("rider", k)
	_actor(r, wx, wd, 1.15)
	riders.append(r)
	return r


func _ready() -> void:
	_rng.randomize()
	view.cam_h = 230.0
	view.focal = 290.0
	world = Node2D.new()
	world.y_sort_enabled = true
	add_child(world)
	hud = Node2D.new()
	hud.z_index = 10
	hud.draw.connect(_draw_hud)
	add_child(hud)
	# the army we leave behind: the real block, still pushing, outrun in seconds
	army = Army.block(self, world, Game.waves[3]["reserves"], 476.0, 7, 285.0, _rng)
	for i in range(9):
		var f := Unit.new()
		f.setup(["veteran", "halberdier", "hammer", "sheathed", "vet_ranged"][i % 5], Unit.ALLY, self)
		f.state = Unit.State.RANK
		f.wx = (i - 4) * 64.0
		f.wd = 320.0
		world.add_child(f)
		army.append(f)
	_sprite("mage", 4, 0.0, 240.0)
	# the tank wall and stragglers, all the way to the necromancer
	var d := 620.0
	while d < NECRO_D - 250.0:
		var n := 4 if d < 1600.0 else 3
		for i in range(n):
			var e := _sprite("armored" if _rng.randf() < 0.7 else "undead", 0, _rng.randf_range(-WALL_X, WALL_X), d + _rng.randf_range(-25, 25))
			enemies.append(e)
		d += 70.0
	necro = _sprite("undead", 0, 0.0, NECRO_D, 1.7, Color(0.6, 1.0, 0.6))
	# riders: player on Jaxx in the middle
	_rider("horse_jaxx", 0.0, RIDER_D)
	_rider("horse_clyde", -150.0, RIDER_D - 40.0)
	_rider("horse_white", 150.0, RIDER_D - 40.0)
	_rider("horse_clyde", -260.0, RIDER_D - 90.0)
	_rider("horse_white", 260.0, RIDER_D - 90.0)
	say("You are the horse. Ride.", 3.0)


func say(s: String, secs := 2.5) -> void:
	caption = s
	caption_t = secs


func lane_x(_l: int) -> float:
	return 0.0


func separation(_u: Unit) -> Vector2:
	return Vector2.ZERO


func near_enemy(_u: Unit, _r: float) -> Unit:
	return null


func _process(delta: float) -> void:
	t += delta
	for u in army:
		u.wd += CREEP * delta
	caption_t -= delta
	match phase:
		Phase.CHARGE: _charge(delta)
		Phase.CIRCLE: _circle(delta)
	for a in actors:
		if a.has_meta("rel"):
			a.set_meta("wd", view.cam_d + float(a.get_meta("rel")))
		var wd: float = a.get_meta("wd")
		a.position = view.project(a.get_meta("wx"), wd)
		a.scale = Vector2.ONE * view.sprite_scale(wd) * float(a.get_meta("k"))
		a.modulate = view.fog(wd) if a.modulate.a >= 1.0 else a.modulate
		a.visible = wd > view.cam_d + 25.0
	queue_redraw()
	hud.queue_redraw()


func _charge(delta: float) -> void:
	# ground gained every second, more the longer you hold the line
	speed += 7.0 * delta
	var surging := t - surge_t < 0.6
	var v := speed + (220.0 if surging else 0.0)
	view.cam_d += v * delta
	scroll = view.cam_d
	if Input.is_action_pressed("move_left"):
		px -= 220.0 * delta
	if Input.is_action_pressed("move_right"):
		px += 220.0 * delta
	px = clampf(px, -WALL_X, WALL_X)
	if Input.is_action_just_pressed("jump") and t - surge_t > 2.2:
		surge_t = t
	# riders hold formation around the camera; teammates drift a little
	var me := riders[0]
	me.set_meta("wx", lerpf(float(me.get_meta("wx")), px, delta * 8.0))
	me.set_meta("wd", view.cam_d + RIDER_D)
	for i in range(1, riders.size()):
		var r := riders[i]
		var home: float = [-150.0, 150.0, -260.0, 260.0][i - 1]
		r.set_meta("wx", home + sin(t * 1.3 + i) * 25.0)
		r.set_meta("wd", view.cam_d + RIDER_D - float([40.0, 40.0, 90.0, 90.0][i - 1]) + sin(t * 2.0 + i) * 12.0)
		(r.get_meta("horse") as Sprite2D).position.y = -absf(sin(t * 9.0 + i)) * 4.0
	(me.get_meta("horse") as Sprite2D).position.y = -absf(sin(t * 9.0)) * 4.0
	# plow: any rider through an enemy knocks it flying; the player loses speed unless surging
	for e in enemies.duplicate():
		var ex: float = e.get_meta("wx")
		var ed: float = e.get_meta("wd")
		for i in range(riders.size()):
			var r := riders[i]
			if absf(ex - float(r.get_meta("wx"))) < 42.0 and absf(ed - float(r.get_meta("wd"))) < 34.0:
				enemies.erase(e)
				actors.erase(e)
				var tw := create_tween().set_parallel(true)
				tw.tween_property(e, "position", e.position + Vector2(_rng.randf_range(-260, 260), -260), 0.5)
				tw.tween_property(e, "rotation", _rng.randf_range(-3, 3), 0.5)
				tw.tween_property(e, "modulate:a", 0.0, 0.5)
				tw.chain().tween_callback(e.queue_free)
				if i == 0:
					hits += 1
					if not surging:
						speed = maxf(90.0, speed * 0.72)
						(me.get_meta("horse") as Sprite2D).modulate = Color(2, 1.4, 1.4)
				break
	(me.get_meta("horse") as Sprite2D).modulate = (me.get_meta("horse") as Sprite2D).modulate.lerp(Color.WHITE, delta * 5.0)
	if view.cam_d > 1200.0 and view.cam_d < 1210.0:
		say("The wall. Don't slow.")
	if view.cam_d + RIDER_D >= NECRO_D - 300.0:
		_begin_circle()


func _begin_circle() -> void:
	phase = Phase.CIRCLE
	view.cam_d = NECRO_D - 330.0
	scroll = view.cam_d
	for e in enemies:
		e.queue_free()
		actors.erase(e)
	enemies.clear()
	say("Circle him. Space when he glows.", 4.0)


func _rider_angle(i: int) -> float:
	return angle + i * TAU / riders.size()


func _circle(delta: float) -> void:
	# orbit
	var w := omega
	if Input.is_action_pressed("move_left"):
		w += 0.9
	if Input.is_action_pressed("move_right"):
		w -= 0.9
	angle += w * delta
	if lunge > 0.0:
		lunge -= delta
	for i in range(riders.size()):
		var a := _rider_angle(i)
		var r := ORBIT_R
		if i == 0 and lunge > 0.0:
			r = ORBIT_R - sin(clampf(lunge / 0.6, 0, 1) * PI) * 110.0
		var rd := riders[i]
		rd.set_meta("wx", cos(a) * r)
		rd.set_meta("wd", NECRO_D + sin(a) * r * 0.55)
		# facing follows the tangent: velocity = (-sin a, cos a); world +d is screen north
		var f := Game.facing_from(Vector2(-sin(a) * w, -cos(a) * w))
		(rd.get_meta("horse") as Sprite2D).frame = f
		(rd.get_meta("rider") as Sprite2D).frame = f
		(rd.get_meta("horse") as Sprite2D).position.y = -absf(sin(t * 9.0 + i)) * 4.0
	# necromancer bolts: telegraph a wedge at the player's angle, then fire
	bolt_t -= delta
	if bolt_phase > 0.0:
		bolt_phase -= delta
		if bolt_phase <= 0.0:
			var diff := absf(wrapf(angle - bolt_angle, -PI, PI))
			if diff < deg_to_rad(28.0):
				hp -= 20.0
				(riders[0].get_meta("horse") as Sprite2D).modulate = Color(2, 1, 1)
				say("Hit. Keep moving.")
			bolt_phase = -1.0
	elif bolt_t <= 0.0:
		bolt_t = maxf(1.4, 2.6 - strikes * 0.4)
		bolt_angle = angle + omega * 0.9      # leads you a little: change speed to dodge
		bolt_phase = 0.9
	(riders[0].get_meta("horse") as Sprite2D).modulate = (riders[0].get_meta("horse") as Sprite2D).modulate.lerp(Color.WHITE, delta * 5.0)
	# channel window
	channel_t -= delta
	if channel_open > 0.0:
		channel_open -= delta
		necro.self_modulate = Color(0.7, 1.0, 0.7).lerp(Color(1.6, 2.6, 1.6), 0.5 + 0.5 * sin(t * 18.0))
	else:
		necro.self_modulate = Color(0.6, 1.0, 0.6)
		if channel_t <= 0.0:
			channel_t = 3.6
			channel_open = 1.6
			say("Now.", 1.0)
	if Input.is_action_just_pressed("jump") and lunge <= 0.0:
		lunge = 0.6
		if channel_open > 0.0:
			strikes += 1
			channel_open = 0.0
			necro.modulate = Color(3, 3, 3)
			create_tween().tween_property(necro, "modulate", Color.WHITE, 0.4)
			say(["First blood.", "He staggers.", "The killing stroke."][mini(strikes - 1, 2)], 2.0)
			if strikes >= STRIKES:
				_finish()
		else:
			say("He isn't open. Watch for the glow.", 1.5)
	if hp <= 0.0:
		hp = 60.0
		say("Jaxx will not fall here.", 2.0)


func _finish() -> void:
	phase = Phase.DONE
	Game.charged = true
	await get_tree().create_timer(1.6).timeout
	Game.goto("cavalry")


func _draw() -> void:
	view.draw_ground(self, scroll, 700.0)
	# the green dot / the necromancer's lightning
	var flick := 0.6 + 0.4 * sin(Time.get_ticks_msec() / 90.0)
	var np := view.project(0.0, NECRO_D)
	var top := np + Vector2(0, -60 * view.sprite_scale(NECRO_D) * 1.7)
	draw_line(top, top + Vector2(_rng.randf_range(-40, 40), -140), Color(0.5, 1.0, 0.5, 0.5 * flick), 2.0)
	draw_circle(top, 5.0 + 3.0 * flick, Color(0.4, 1.0, 0.4, flick))
	if phase == Phase.CIRCLE and bolt_phase > 0.0:
		# telegraph wedge on the ground from the necromancer toward bolt_angle
		var pts := PackedVector2Array([np])
		for k in range(-6, 7):
			var a := bolt_angle + k * deg_to_rad(28.0) / 6.0
			pts.append(view.project(cos(a) * (ORBIT_R + 40.0), NECRO_D + sin(a) * (ORBIT_R + 40.0) * 0.55))
		draw_colored_polygon(pts, Color(0.4, 1.0, 0.4, 0.25 + 0.35 * (1.0 - bolt_phase / 0.9)))


func _draw_hud() -> void:
	var f := ThemeDB.fallback_font
	if phase == Phase.CHARGE:
		var prog := clampf(view.cam_d / (NECRO_D - 630.0), 0.0, 1.0)
		hud.draw_rect(Rect2(12, 520, 936, 8), Color(0, 0, 0, 0.5))
		hud.draw_rect(Rect2(12, 520, 936 * prog, 8), Color(0.4, 1.0, 0.4, 0.8))
		hud.draw_string(f, Vector2(12, 20), "A/D steer   Space surge   ground gained %d   speed %d   plowed %d" % [int(view.cam_d), int(speed), hits], HORIZONTAL_ALIGNMENT_LEFT, -1, 14, Color("#e9efec"))
	elif phase == Phase.CIRCLE:
		hud.draw_string(f, Vector2(12, 20), "A/D speed the circle   Space strike when he glows   HP %d   strikes %d / %d" % [int(hp), strikes, STRIKES], HORIZONTAL_ALIGNMENT_LEFT, -1, 14, Color("#e9efec"))
	if caption_t > 0.0:
		hud.draw_string(f, Vector2(0, 60), caption, HORIZONTAL_ALIGNMENT_CENTER, 960, 22, Color("#f0c260"))


func _unhandled_input(e: InputEvent) -> void:
	if e.is_action_pressed("skip") and phase != Phase.DONE:
		_finish()
