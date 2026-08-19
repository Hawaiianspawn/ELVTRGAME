extends Node2D
## On rails on Jaxx. Three lanes, A/D switch, Space jump. Oozes are obstacles: with flame (magic) you bash
## through them; without, they hurt. Motes on the road refill the flame. Ends at the reveal.

const LANES := [330.0, 480.0, 630.0]
const RIDE_Y := 420.0
const LENGTH := 45.0

var lane := 1
var jaxx: Node2D
var horse: Sprite2D
var rider: Sprite2D
var jump_t := -1.0
var t := 0.0
var scroll := 0.0
var speed := 260.0
var hp := 100.0
var bashed := 0
var things: Array[Node2D] = []     # oozes and motes, meta "kind"
var spawn_t := 1.0
var _rng := RandomNumberGenerator.new()
var _done := false


func _ready() -> void:
	_rng.randomize()
	jaxx = Node2D.new()
	jaxx.position = Vector2(LANES[lane], RIDE_Y)
	jaxx.scale = Vector2.ONE * 1.3
	horse = Game.make_sprite("horse_jaxx", 4)
	jaxx.add_child(horse)
	rider = Game.make_sprite("mage", 4)
	rider.scale = Vector2.ONE * 0.6
	rider.position = Vector2(0, -34)
	jaxx.add_child(rider)
	add_child(jaxx)


func _unhandled_input(e: InputEvent) -> void:
	if _done:
		return
	if e.is_action_pressed("move_left") and lane > 0:
		lane -= 1
	elif e.is_action_pressed("move_right") and lane < 2:
		lane += 1
	elif e.is_action_pressed("jump") and jump_t < 0.0:
		jump_t = 0.0


func _spawn() -> void:
	var l := _rng.randi_range(0, 2)
	var mote := _rng.randf() < 0.3
	var n: Node2D
	if mote:
		n = Node2D.new()
		n.draw.connect(func(): n.draw_circle(Vector2.ZERO, 6.0, Color(0.4, 1.0, 0.4)))
	else:
		n = Game.make_sprite("ooze", 0)
		n.scale = Vector2.ONE * 0.9
	n.set_meta("kind", "mote" if mote else "ooze")
	n.position = Vector2(LANES[l], -60)
	add_child(n)
	things.append(n)


func _process(delta: float) -> void:
	if _done:
		return
	t += delta
	speed = 260.0 + t * 6.0
	scroll += speed * delta
	spawn_t -= delta
	if spawn_t <= 0.0:
		spawn_t = maxf(0.35, 1.1 - t * 0.015)
		_spawn()
	# horse
	jaxx.position.x = lerpf(jaxx.position.x, LANES[lane], delta * 12.0)
	var lift := 0.0
	if jump_t >= 0.0:
		jump_t += delta
		lift = sin(clampf(jump_t / 0.55, 0, 1) * PI) * 90.0
		if jump_t > 0.55:
			jump_t = -1.0
	horse.position.y = -lift - (absf(sin(t * 14.0)) * 3.0 if lift == 0.0 else 0.0)
	rider.position.y = -34.0 - lift
	# things
	for n in things.duplicate():
		n.position.y += speed * delta
		if n.position.y > 600:
			things.erase(n)
			n.queue_free()
			continue
		if absf(n.position.x - jaxx.position.x) < 50.0 and absf(n.position.y - RIDE_Y) < 34.0:
			var kind: String = n.get_meta("kind")
			if kind == "mote":
				Game.gain_magic(6.0)
			elif lift > 20.0:
				continue      # jumped over it
			elif Game.magic >= 3.0:
				Game.magic -= 3.0
				bashed += 1
				var tw := create_tween().set_parallel(true)
				tw.tween_property(n, "position", n.position + Vector2(_rng.randf_range(-200, 200), -200), 0.4)
				tw.tween_property(n, "modulate:a", 0.0, 0.4)
				tw.chain().tween_callback(n.queue_free)
				things.erase(n)
				continue
			else:
				hp -= 15.0
				horse.modulate = Color(2, 1, 1)
			things.erase(n)
			n.queue_free()
	horse.modulate = horse.modulate.lerp(Color.WHITE, delta * 6.0)
	if hp <= 0.0:
		hp = 100.0
		Game.magic = 30.0    # the horse doesn't die; you get back on
	if t > LENGTH:
		_done = true
		Game.goto("reveal")
	queue_redraw()


func _draw() -> void:
	draw_rect(Rect2(0, 0, 960, 540), Color("#16151a"))
	draw_rect(Rect2(250, 0, 460, 540), Color("#2b2b2e"))
	for x in [405.0, 555.0]:
		for i in range(14):
			var y := fposmod(i * 50.0 + scroll, 700.0) - 60.0
			draw_rect(Rect2(x - 3, y, 6, 24), Color("#3a3d3f"))
	# tree line silhouettes scrolling
	for i in range(30):
		var y := fposmod(i * 47.0 + scroll * 0.6, 700.0) - 60.0
		draw_rect(Rect2(40 + (i % 3) * 40, y, 24, 40 + (i % 4) * 12), Color("#0e0e12"))
		draw_rect(Rect2(880 - (i % 3) * 40, y, 24, 40 + (i % 4) * 12), Color("#0e0e12"))
	# flame on the mage
	draw_circle(jaxx.position + Vector2(0, -95 + rider.position.y + 34), 4.0 + minf(Game.magic, 200.0) * 0.06, Color(0.4, 1.0, 0.4, 0.8))
	var f := ThemeDB.fallback_font
	draw_string(f, Vector2(12, 20), "A/D lane   Space jump   flame bashes ooze (3 magic each)", HORIZONTAL_ALIGNMENT_LEFT, -1, 14, Color("#a0a08b"))
	draw_string(f, Vector2(12, 40), "MAGIC %d   HP %d   bashed %d" % [int(Game.magic), int(hp), bashed], HORIZONTAL_ALIGNMENT_LEFT, -1, 14, Color("#e9efec"))
	draw_rect(Rect2(12, 520, 936 * clampf(t / LENGTH, 0, 1), 6), Color(0.4, 1.0, 0.4, 0.6))
