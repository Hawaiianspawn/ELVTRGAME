extends Node2D
## Title card: pick a hero, then route into the battle.

var _label: Label
var _portrait: Sprite2D


func _ready() -> void:
	_label = Label.new()
	_label.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	_label.size = Vector2(960, 540)
	_label.vertical_alignment = VERTICAL_ALIGNMENT_CENTER
	_label.add_theme_font_size_override("font_size", 22)
	_label.add_theme_color_override("font_color", Color("#a0a08b"))
	add_child(_label)
	_portrait = Game.make_sprite(Game.hero, 0)
	_portrait.position = Vector2(480, 150)
	_portrait.scale = Vector2(2, 2)
	add_child(_portrait)
	_refresh()


func _refresh() -> void:
	_label.text = "KINDLED\nThe Green Dot\n\n\n\n\nhero: %s   [H] next hero\n\n[Space] begin\n\nWASD move   Mouse aim   LMB cast   RMB hold: siphon\nZ/X/C spells   Q/E set lane unit type (hover lane)\n\ndev: 1 siege, 2-5 wave, Tab next, P 3D probe" % Game.hero.trim_prefix("hero_")
	_portrait.texture = Game.make_sprite(Game.hero, 0).texture
	_portrait.offset = Game.make_sprite(Game.hero, 0).offset


func _unhandled_input(e: InputEvent) -> void:
	if e is InputEventKey and e.pressed and not e.echo and e.keycode == KEY_H:
		Game.hero = Game.HEROES[(Game.HEROES.find(Game.hero) + 1) % Game.HEROES.size()]
		_refresh()
		return
	if e.is_action_pressed("advance"):
		set_process_unhandled_input(false)
		Game.reset_run()
		Game.goto("siege")
