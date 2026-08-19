class_name Dialogue
extends CanvasLayer
## Bottom box, click/space advances. `await Dialogue.play(self, lines)`.

static func play(host: Node, lines: Array) -> void:
	var d := Dialogue.new()
	d.layer = 50
	host.add_child(d)
	await d._run(lines)
	d.queue_free()


var _box: PanelContainer
var _name: Label
var _text: Label
var _next := false


func _init() -> void:
	_box = PanelContainer.new()
	_box.position = Vector2(80, 400)
	_box.size = Vector2(800, 120)
	var v := VBoxContainer.new()
	_name = Label.new()
	_name.add_theme_color_override("font_color", Color("#f0c260"))
	_text = Label.new()
	_text.autowrap_mode = TextServer.AUTOWRAP_WORD_SMART
	_text.custom_minimum_size.x = 780
	var hint := Label.new()
	hint.text = "[space]"
	hint.modulate.a = 0.5
	hint.horizontal_alignment = HORIZONTAL_ALIGNMENT_RIGHT
	v.add_child(_name)
	v.add_child(_text)
	v.add_child(hint)
	_box.add_child(v)
	add_child(_box)


func _run(lines: Array) -> void:
	for line in lines:
		_name.text = str(line.get("who", ""))
		_text.text = str(line["text"])
		_next = false
		while not _next:
			await get_tree().process_frame


func _unhandled_input(e: InputEvent) -> void:
	if e.is_action_pressed("advance"):
		_next = true
		get_viewport().set_input_as_handled()
