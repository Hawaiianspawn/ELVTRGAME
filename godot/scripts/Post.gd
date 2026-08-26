extends CanvasLayer
class_name Post
## Togglable post-process preset stack: one full-screen ColorRect running
## assets/shaders/post.gdshader against the screen texture. Mounted by Battle.gd on a
## layer between the world/props/fx (1) and the HUD text/panel layer (20), so only the
## former gets processed. Preset 0 is off (ColorRect hidden, shader never runs).

const NAMES := ["off", "dither", "glow", "grade", "edges", "crt"]
const SHADER := preload("res://assets/shaders/post.gdshader")

var preset := 0
var _rect: ColorRect


func _ready() -> void:
	layer = 10
	_rect = ColorRect.new()
	_rect.set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)
	_rect.mouse_filter = Control.MOUSE_FILTER_IGNORE
	var mat := ShaderMaterial.new()
	mat.shader = SHADER
	_rect.material = mat
	_rect.visible = false
	add_child(_rect)


func cycle(dir: int) -> void:
	set_preset((preset + dir + NAMES.size()) % NAMES.size())


func set_preset(i: int) -> void:
	preset = i
	_rect.visible = preset != 0
	_rect.material.set_shader_parameter("preset", preset)


func preset_name() -> String:
	return NAMES[preset]
