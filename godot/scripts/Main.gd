extends Node2D
## Title card: pick a hero, then route into the battle. Kit lintel holds the wordmark, a kit
## hero card holds the pick at 2x, the plate under it names the role.

var _name: Label
var _hint: Label
var _portrait: Sprite2D
var _medallion: TextureRect


func _ready() -> void:
	Input.set_custom_mouse_cursor(Ui.tex("cursor"), Input.CURSOR_ARROW, Vector2(3, 2))
	var ui := Control.new()
	add_child(ui)
	var bg := Ui.sprite(ui, "title_bg", Vector2(80, 46))   # 400x224 hall illustration at 2x, dimmed under the chrome
	bg.scale = Vector2(2, 2)
	bg.modulate = Color(0.55, 0.55, 0.55)
	Ui.sprite(ui, "frame_lintel", Vector2(240, 14))
	Ui.label(ui, "KINDLED", Vector2(240, 48), Ui.COL_EMBER, 32, 481.0).add_theme_font_override("font", Ui.font("title32"))
	Ui.label(ui, "The Green Dot", Vector2(240, 88), Ui.COL_DIM, 16, 481.0)
	Ui.sprite(ui, "hero_card", Vector2(396, 150))
	var plate := Ui.sprite(ui, "plate", Vector2(384, 368))
	_name = Ui.label(plate, "", Vector2(0, 10), Ui.COL_TEXT, 16, 193.0)
	_medallion = Ui.portrait(ui, Game.hero, Vector2(278, 207))
	_hint = Ui.label(ui, "", Vector2(0, 424), Ui.COL_DIM, 16, 960.0)
	Ui.label(ui, "v" + Game.VERSION, Vector2(0, 516), Ui.COL_DIM, 16, 950.0).horizontal_alignment = HORIZONTAL_ALIGNMENT_RIGHT
	_portrait = Game.make_sprite(Game.hero, 0)
	_portrait.position = Vector2(480, 344)   # feet at the card's lower well edge; 88px cell at 2x fills the well
	_portrait.scale = Vector2(2, 2)
	add_child(_portrait)
	_refresh()


func _refresh() -> void:
	_name.text = Game.hero.trim_prefix("hero_").to_upper()
	(_medallion.get_child(0) as TextureRect).texture = Ui.tex("portraits/" + Game.hero)
	var text := "[H] next hero      [Space] begin\n\nWASD move   Mouse aim   LMB cast   Z/X/C spells   Q/E set lane unit type"
	if not OS.has_feature("web"):
		text += "\ndev: 1 siege, 2-5 wave, Tab next"
	_hint.text = text
	var s := Game.make_sprite(Game.hero, 0)
	_portrait.texture = s.texture
	_portrait.offset = s.offset


func _unhandled_input(e: InputEvent) -> void:
	if e is InputEventKey and e.pressed and not e.echo and e.keycode == KEY_H:
		Game.hero = Game.HEROES[(Game.HEROES.find(Game.hero) + 1) % Game.HEROES.size()]
		_refresh()
		return
	if e.is_action_pressed("advance"):
		set_process_unhandled_input(false)
		Game.reset_run()
		Game.goto("siege")
