extends Node2D
## Celebration. 3x3 tables, a bar, three vets to talk to. Door opens once you've heard why they burned him
## and why nobody chases the horse.

var hero: Node2D
var hero_sprite: Sprite2D
var hotspots: Array[Dictionary] = []   # {pos, key, label, done}
var hint := ""
var _rng := RandomNumberGenerator.new()
var _busy := false


func _ready() -> void:
	_rng.randomize()
	var world := Node2D.new()
	world.y_sort_enabled = true
	add_child(world)
	var pool := ["veteran", "halberdier", "hammer", "sheathed", "vet_ranged", "mage"]
	for r in range(3):
		for c in range(3):
			var tp := Vector2(240 + c * 240, 200 + r * 110)
			for k in range(3):
				var s := Game.make_sprite(pool[_rng.randi_range(0, 5)], [0, 2, 6][k])
				s.position = tp + [Vector2(0, -34), Vector2(-52, 6), Vector2(52, 6)][k]
				s.scale = Vector2.ONE * 0.75
				world.add_child(s)
	hero = Node2D.new()
	hero.position = Vector2(480, 470)
	hero_sprite = Game.make_sprite(Game.hero, 4)
	hero_sprite.scale = Vector2.ONE * 0.9
	hero.add_child(hero_sprite)
	world.add_child(hero)
	hotspots = [
		{"pos": Vector2(240, 166), "key": "vet_burn", "label": "Old Halloran", "done": false},
		{"pos": Vector2(720, 166), "key": "vet_horse", "label": "Sergeant Wyn", "done": false},
		{"pos": Vector2(480, 386), "key": "vet_captain", "label": "Merle", "done": false},
		{"pos": Vector2(480, 80), "key": "bar", "label": "the bar", "done": false},
		{"pos": Vector2(900, 500), "key": "door", "label": "the door", "done": false},
	]


func _process(delta: float) -> void:
	if _busy:
		return
	var mv := Input.get_vector("move_left", "move_right", "move_up", "move_down")
	hero.position += mv * 170.0 * delta
	hero.position = hero.position.clamp(Vector2(40, 90), Vector2(920, 520))
	if mv != Vector2.ZERO:
		hero_sprite.frame = Game.facing_from(mv)
	hint = ""
	for h in hotspots:
		if hero.position.distance_to(h["pos"]) < 60.0:
			hint = "[Space] " + h["label"]
	queue_redraw()


func _unhandled_input(e: InputEvent) -> void:
	if _busy or not e.is_action_pressed("advance"):
		return
	for h in hotspots:
		if hero.position.distance_to(h["pos"]) < 60.0:
			_talk(h)
			return


func _talk(h: Dictionary) -> void:
	_busy = true
	if h["key"] == "door":
		var burn: bool = hotspots[0]["done"]
		var horse: bool = hotspots[1]["done"]
		if not (burn and horse):
			await Dialogue.play(self, [{"who": "You", "text": "Not yet. Someone here owes me a straight answer."}])
			_busy = false
			return
		await Dialogue.play(self, Game.dialogue["door"])
		Game.goto("road")
		return
	await Dialogue.play(self, Game.dialogue[h["key"]])
	h["done"] = true
	_busy = false


func _draw() -> void:
	draw_rect(Rect2(0, 0, 960, 540), Color("#2a2320"))
	draw_rect(Rect2(0, 0, 960, 110), Color("#3b2d24"))          # bar
	draw_rect(Rect2(60, 92, 840, 14), Color("#5e2d20"))
	draw_rect(Rect2(870, 470, 60, 70), Color("#211210"))         # door
	draw_string(ThemeDB.fallback_font, Vector2(872, 462), "door", HORIZONTAL_ALIGNMENT_LEFT, -1, 12, Color("#a0a08b"))
	for r in range(3):
		for c in range(3):
			draw_rect(Rect2(200 + c * 240, 186 + r * 110, 80, 26), Color("#5e2d20"))
	for h in hotspots:
		if not h["done"] and h["key"] != "door":
			draw_string(ThemeDB.fallback_font, h["pos"] + Vector2(-4, -70), "!", HORIZONTAL_ALIGNMENT_LEFT, -1, 18, Color("#f0c260"))
	# hero flame
	draw_circle(hero.position + Vector2(0, -60), 4.0 + minf(Game.magic, 200.0) * 0.06, Color(0.4, 1.0, 0.4, 0.8))
	draw_string(ThemeDB.fallback_font, Vector2(12, 530), hint, HORIZONTAL_ALIGNMENT_LEFT, -1, 16, Color("#e9efec"))
	draw_string(ThemeDB.fallback_font, Vector2(12, 20), "The mess hall. WASD walk, Space talk.", HORIZONTAL_ALIGNMENT_LEFT, -1, 14, Color("#a0a08b"))
