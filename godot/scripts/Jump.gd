extends Node
## Dev jumper. Number row / numpad jumps straight to a phase, Tab advances to the next one.
##   1 siege (gate)   2-5 hall wave 1-4

const PHASES := [
	["siege", 0], ["battle", 0], ["battle", 1], ["battle", 2], ["battle", 3],
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
				return Game.wave + 1
			return i
	return -1


func _refresh() -> void:
	var i := _phase_index()
	_label.text = "phase %d/5  [1-5 jump, Tab next]" % (i + 1) if i >= 0 else "[1-5 jump, Tab next]"


func _unhandled_input(e: InputEvent) -> void:
	if not (e is InputEventKey and e.pressed and not e.echo):
		return
	var k: int = e.keycode
	var idx := -1
	if k >= KEY_1 and k <= KEY_9:
		idx = k - KEY_1
	elif k >= KEY_KP_1 and k <= KEY_KP_9:
		idx = k - KEY_KP_1
	elif k == KEY_TAB:
		idx = mini(_phase_index() + 1, PHASES.size() - 1)
	if idx < 0 or idx >= PHASES.size():
		return
	get_viewport().set_input_as_handled()
	_go(idx)


func _go(idx: int) -> void:
	var p: Array = PHASES[idx]
	Game.wave = int(p[1])
	if Game.magic < 40.0 and idx > 0:
		Game.magic = 40.0          # enough to try a spell when dropping in mid-run
	await Game.goto(p[0])
	_refresh()
