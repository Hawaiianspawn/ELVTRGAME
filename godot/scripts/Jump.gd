extends Node
## Dev jumper. Number row / numpad jumps straight to a phase, Tab advances to the next one.
##   1-4 battle wave 1-4   5 charge   6 cavalry (after the charge)   7 mess hall   8 road   9 ride   0 reveal

const PHASES := [
	["battle", 0], ["battle", 1], ["battle", 2], ["battle", 3],
	["charge", 3], ["cavalry", 3], ["messhall", 3], ["road", 3], ["ride", 3], ["reveal", 3],
]
var _label: Label


func _ready() -> void:
	var cl := CanvasLayer.new()
	cl.layer = 90
	_label = Label.new()
	_label.position = Vector2(700, 8)
	_label.size = Vector2(250, 20)
	_label.horizontal_alignment = HORIZONTAL_ALIGNMENT_RIGHT
	_label.add_theme_font_size_override("font_size", 12)
	_label.modulate = Color(1, 1, 1, 0.55)
	cl.add_child(_label)
	add_child(cl)
	_refresh()


func _phase_index() -> int:
	var scene := get_tree().current_scene
	if scene == null:
		return -1
	var path := scene.scene_file_path
	for i in range(PHASES.size()):
		if Game.SCENES.get(PHASES[i][0], "") == path:
			if PHASES[i][0] == "battle":
				return Game.wave
			return i
	return -1


func _refresh() -> void:
	var i := _phase_index()
	_label.text = "phase %d/10  [1-0 jump, Tab next]" % (i + 1) if i >= 0 else "[1-0 jump, Tab next]"


func _unhandled_input(e: InputEvent) -> void:
	if not (e is InputEventKey and e.pressed and not e.echo):
		return
	var k: int = e.keycode
	var idx := -1
	if k >= KEY_1 and k <= KEY_9:
		idx = k - KEY_1
	elif k >= KEY_KP_1 and k <= KEY_KP_9:
		idx = k - KEY_KP_1
	elif k == KEY_0 or k == KEY_KP_0:
		idx = 9
	elif k == KEY_TAB:
		idx = mini(_phase_index() + 1, PHASES.size() - 1)
	if idx < 0:
		return
	get_viewport().set_input_as_handled()
	_go(idx)


func _go(idx: int) -> void:
	var p: Array = PHASES[idx]
	Game.wave = int(p[1])
	Game.charged = idx >= 5
	if Game.magic < 40.0 and idx > 0:
		Game.magic = 40.0          # enough to try a spell when dropping in mid-run
	await Game.goto(p[0])
	_refresh()
