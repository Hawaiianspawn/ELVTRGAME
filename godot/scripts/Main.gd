extends Node2D
## Title card: shows the turret hero, then routes into the battle. Kit lintel holds the wordmark,
## a kit hero card holds the hero at 2x on a slow turntable, the plate under it names the role.

const FACE_SECS := 0.7   # turntable: seconds per facing step
const BG_POS := Vector2(-20, -10)
const BG_SCALE := 2.5

# painted lanterns in title_bg (x, y in bg pixels, glow radius in screen px), front pair
# to vanishing point -- additive glows flicker on top so the illustration reads live
const LAMPS := [
	Vector3(27, 113, 26), Vector3(370, 113, 26),
	Vector3(64, 133, 18), Vector3(329, 132, 18),
	Vector3(88, 145, 14), Vector3(305, 145, 14),
	Vector3(105, 152, 12), Vector3(287, 154, 12),
	Vector3(117, 157, 10), Vector3(275, 158, 10),
	Vector3(126, 163, 9), Vector3(264, 165, 9),
	Vector3(137, 167, 8), Vector3(257, 170, 8),
	Vector3(147, 173, 7), Vector3(246, 175, 7),
	Vector3(157, 177, 6), Vector3(237, 178, 6),
]

var _name: Label
var _prompt: Label
var _portrait: Sprite2D
var _medallion: TextureRect
var _glows: Array[TextureRect] = []
var _embers: Array[TextureRect] = []
var _t := 0.0


func _ready() -> void:
	Sound.ambient("ambient_menu.wav")
	Input.set_custom_mouse_cursor(Ui.tex("cursor"), Input.CURSOR_ARROW, Vector2(3, 2))
	var ui := Control.new()
	add_child(ui)
	# 400x224 hall illustration full-bleed: 2.5x = 1000x560, centred overscan crops 20/10px
	# per side so the whole frame is hall, no letterbox
	var bg := Ui.sprite(ui, "title_bg", BG_POS)
	bg.scale = Vector2(BG_SCALE, BG_SCALE)
	bg.modulate = Color(0.55, 0.55, 0.55)
	# lantern glows: one soft radial per painted lamp, additive, each on its own flicker phase
	var grad := Gradient.new()
	grad.set_color(0, Color(0.45, 1.0, 0.55, 0.5))
	grad.set_color(1, Color(0.45, 1.0, 0.55, 0.0))
	var gt := GradientTexture2D.new()
	gt.gradient = grad
	gt.fill = GradientTexture2D.FILL_RADIAL
	gt.fill_from = Vector2(0.5, 0.5)
	gt.fill_to = Vector2(0.5, 0.0)
	var add := CanvasItemMaterial.new()
	add.blend_mode = CanvasItemMaterial.BLEND_MODE_ADD
	for l in LAMPS:
		var g := TextureRect.new()
		g.texture = gt
		g.material = add
		g.size = Vector2(l.z, l.z) * 2.5
		g.position = BG_POS + Vector2(l.x, l.y) * BG_SCALE - g.size * 0.5
		g.mouse_filter = Control.MOUSE_FILTER_IGNORE
		ui.add_child(g)
		_glows.append(g)
	# necrotic motes drifting up through the hall, faded in/out over their life
	var motes := CPUParticles2D.new()
	motes.amount = 26
	motes.lifetime = 9.0
	motes.preprocess = 9.0   # the air is already alive on the first frame
	motes.emission_shape = CPUParticles2D.EMISSION_SHAPE_RECTANGLE
	motes.emission_rect_extents = Vector2(470, 160)
	motes.position = Vector2(480, 330)
	motes.direction = Vector2(0, -1)
	motes.spread = 20.0
	motes.gravity = Vector2.ZERO
	motes.initial_velocity_min = 6.0
	motes.initial_velocity_max = 16.0
	motes.scale_amount_min = 1.0
	motes.scale_amount_max = 2.0
	motes.color = Color(0.5, 1.0, 0.6, 0.4)
	var ramp := Gradient.new()
	ramp.set_color(0, Color(1, 1, 1, 0.0))
	ramp.add_point(0.3, Color(1, 1, 1, 1.0))
	ramp.set_color(1, Color(1, 1, 1, 0.0))
	motes.color_ramp = ramp
	ui.add_child(motes)
	Ui.sprite(ui, "frame_lintel", Vector2(240, 14))
	Ui.label(ui, "KINDLED", Vector2(240, 48), Ui.COL_EMBER, 32, 481.0).add_theme_font_override("font", Ui.font("title32"))
	Ui.label(ui, "The Necromancer's Keep", Vector2(240, 88), Ui.COL_DIM, 16, 481.0)
	Ui.sprite(ui, "hero_card", Vector2(396, 150))
	var plate := Ui.sprite(ui, "plate", Vector2(384, 368))
	_name = Ui.label(plate, "", Vector2(0, 10), Ui.COL_TEXT, 16, 193.0)
	_medallion = Ui.portrait(ui, Game.hero, Vector2(278, 207))
	# the chrome's gold catches the same light as the lanterns: ember breathing on the
	# lintel rosettes, the card crest and the medallion ring (x, y screen centre, radius)
	var egrad := Gradient.new()
	egrad.set_color(0, Color(1.0, 0.78, 0.38, 0.4))
	egrad.set_color(1, Color(1.0, 0.78, 0.38, 0.0))
	var egt := GradientTexture2D.new()
	egt.gradient = egrad
	egt.fill = GradientTexture2D.FILL_RADIAL
	egt.fill_from = Vector2(0.5, 0.5)
	egt.fill_to = Vector2(0.5, 0.0)
	for p in [Vector3(266, 44, 30), Vector3(690, 44, 30), Vector3(479, 163, 22), Vector3(327, 258, 24)]:
		var g := TextureRect.new()
		g.texture = egt
		g.material = add   # same additive blend as the lantern glows
		g.size = Vector2(p.z, p.z) * 2.0
		g.position = Vector2(p.x, p.y) - g.size * 0.5
		g.mouse_filter = Control.MOUSE_FILTER_IGNORE
		ui.add_child(g)
		_embers.append(g)
	# the call to arms breathes; the controls sit quiet underneath it
	_prompt = Ui.label(ui, "[SPACE]  BEGIN", Vector2(0, 418), Ui.COL_EMBER, 20, 960.0)
	var pt := create_tween().set_loops()
	pt.tween_property(_prompt, "modulate:a", 0.35, 0.9).set_trans(Tween.TRANS_SINE).set_ease(Tween.EASE_IN_OUT)
	pt.tween_property(_prompt, "modulate:a", 1.0, 0.9).set_trans(Tween.TRANS_SINE).set_ease(Tween.EASE_IN_OUT)
	var controls := "Mouse aim   LMB gatling   RMB cannon   Q/E set lane unit type"
	if not OS.has_feature("web"):
		controls += "\ndev: 1-4 wave, Tab next"
	Ui.label(ui, controls, Vector2(0, 452), Ui.COL_DIM, 16, 960.0)
	Ui.label(ui, "v" + Game.VERSION, Vector2(0, 516), Ui.COL_DIM, 16, 950.0).horizontal_alignment = HORIZONTAL_ALIGNMENT_RIGHT
	_portrait = Game.make_sprite(Game.hero, 0)
	_portrait.position = Vector2(480, 344)   # feet at the card's lower well edge; 88px cell at 2x fills the well
	_portrait.scale = Vector2(2, 2)
	add_child(_portrait)
	_refresh()


func _process(delta: float) -> void:
	# slow clockwise turntable through the 8 packed facings
	_t += delta
	_portrait.frame = int(_t / FACE_SECS) % 8
	# lamp flicker: layered sines at co-prime-ish rates per lamp read as candlelight
	for i in _glows.size():
		var fi := float(i)
		_glows[i].modulate.a = 0.6 + 0.25 * sin(_t * (2.1 + 0.37 * fi) + fi * 2.4) + 0.15 * sin(_t * 7.3 + fi * 5.1)
	# the gold breathes slower than the candle flicker: hearth, not flame
	for i in _embers.size():
		_embers[i].modulate.a = 0.55 + 0.2 * sin(_t * 1.4 + float(i) * 1.7)


func _refresh() -> void:
	_name.text = Game.hero.trim_prefix("hero_").to_upper()
	(_medallion.get_child(0) as TextureRect).texture = Ui.tex("portraits/" + Game.hero)
	var s := Game.make_sprite(Game.hero, 0)
	_portrait.texture = s.texture
	_portrait.offset = s.offset


func _unhandled_input(e: InputEvent) -> void:
	if e.is_action_pressed("advance"):
		set_process_unhandled_input(false)
		Sound.play("ui_relic_chime.wav", 0.0)
		Game.reset_run()
		Game.goto("battle")
