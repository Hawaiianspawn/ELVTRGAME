extends Node2D
## The necromancer has built a bigger army. Cut.

var caption: Label
var _rng := RandomNumberGenerator.new()
var necro: Sprite2D
var _t := 0.0


func _ready() -> void:
	_rng.randomize()
	var world := Node2D.new()
	world.y_sort_enabled = true
	add_child(world)
	# rows of the new army, far to near, all silhouettes
	for row in range(7):
		var y := 70.0 + row * 34.0
		var sc := lerpf(0.5, 1.1, row / 6.0)
		for i in range(22):
			var s := Game.make_sprite(["armored", "undead", "undead2", "ooze"][_rng.randi_range(0, 3)], 0)
			s.position = Vector2(-40 + i * 50 + (row % 2) * 25 + _rng.randf_range(-6, 6), y)
			s.scale = Vector2.ONE * sc
			s.modulate = Color(0.12, 0.16, 0.13)
			world.add_child(s)
	necro = Game.make_sprite("undead2", 0)
	necro.position = Vector2(480, 175)
	necro.scale = Vector2.ONE * 1.8
	necro.modulate = Color(0.5, 1.0, 0.5)
	world.add_child(necro)
	var j := Node2D.new()
	j.position = Vector2(480, 470)
	j.scale = Vector2.ONE * 1.4
	var h := Game.make_sprite("horse_jaxx", 4)
	j.add_child(h)
	var r := Game.make_sprite("mage", 4)
	r.scale = Vector2.ONE * 0.6
	r.position = Vector2(0, -34)
	j.add_child(r)
	world.add_child(j)
	var cl := CanvasLayer.new()
	add_child(cl)
	caption = Label.new()
	caption.position = Vector2(0, 240)
	caption.size = Vector2(960, 80)
	caption.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	caption.add_theme_font_size_override("font_size", 22)
	caption.add_theme_color_override("font_color", Color("#e9efec"))
	cl.add_child(caption)
	_run()


func _run() -> void:
	caption.text = "The treeline ends."
	await get_tree().create_timer(2.5).timeout
	caption.text = "He burned. Nothing left to reincarnate.\nSomeone else lit the green dot again."
	await get_tree().create_timer(4.0).timeout
	caption.text = "And this time he brought everyone."
	await get_tree().create_timer(3.0).timeout
	caption.text = "TO BE CONTINUED\n\n[Space] title"
	set_process_unhandled_input(true)


func _unhandled_input(e: InputEvent) -> void:
	if e.is_action_pressed("advance") and caption.text.begins_with("TO BE"):
		set_process_unhandled_input(false)
		get_tree().change_scene_to_file("res://scenes/Main.tscn")


func _process(d: float) -> void:
	_t += d
	queue_redraw()


func _draw() -> void:
	draw_rect(Rect2(0, 0, 960, 540), Color("#0e0e12"))
	draw_rect(Rect2(0, 300, 960, 240), Color("#16151a"))
	var flick := 0.6 + 0.4 * sin(_t * 11.0)
	draw_circle(Vector2(480, 10), 12.0 + 6.0 * flick, Color(0.4, 1.0, 0.4, flick))
	for i in range(3):
		draw_line(Vector2(480, 10), Vector2(480 + _rng.randf_range(-120, 120), -100), Color(0.5, 1.0, 0.5, 0.4 * flick), 2.0)
