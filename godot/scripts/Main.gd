extends Node2D
## Title card, then routes into the battle.

func _ready() -> void:
	var l := Label.new()
	l.text = "KINDLED\nThe Green Dot\n\n[Space] begin\n\nWASD move   Mouse aim   LMB cast   RMB hold: siphon\n1-3 spells   Q/E set lane unit type (hover lane)"
	l.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	l.size = Vector2(960, 540)
	l.vertical_alignment = VERTICAL_ALIGNMENT_CENTER
	l.add_theme_font_size_override("font_size", 22)
	l.add_theme_color_override("font_color", Color("#a0a08b"))
	add_child(l)


func _unhandled_input(e: InputEvent) -> void:
	if e.is_action_pressed("advance"):
		set_process_unhandled_input(false)
		Game.reset_run()
		Game.goto("battle")
