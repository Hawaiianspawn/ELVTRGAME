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
	# Shrunk + parked above the heading (text block starts ~y=100): at scale 2 the portrait's
	# 176px height ate into every line down to "v0.1.0". At scale 1 (88px) it clears the gap.
	_portrait.position = Vector2(480, 65)
	_portrait.scale = Vector2(1, 1)
	add_child(_portrait)
	_refresh()


func _refresh() -> void:
	# v-line reuses a blank line below the tagline (kept clear of the portrait sprite) so line
	# count matches the pre-version layout and nothing else shifts.
	var text := "KINDLED\nThe Green Dot\nv%s\n\n\n\nhero: %s   [H] next hero\n\n[Space] begin\n\nWASD move   Mouse aim   LMB cast\nZ/X/C spells   Q/E set lane unit type (hover lane)" % [Game.VERSION, Game.hero.trim_prefix("hero_")]
	if not OS.has_feature("web"):
		text += "\n\ndev: 1 siege, 2-5 wave, Tab next"
	_label.text = text
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
