extends CanvasLayer
class_name Post
## Togglable post-process preset stack: one full-screen ColorRect running
## assets/shaders/post.gdshader against the screen texture. Mounted by Battle.gd above
## the world/props/fx (1) AND the HUD text/panel layer (20) so the whole frame gets the
## look; only the dev jump toast (90) and the scene fade (100) escape.
## Preset 0 is off (ColorRect hidden, shader never runs).

const NAMES := ["off", "dither", "glow", "grade", "crt", "retro", "dusty4"]   # "edges" cut 2026-08-31 (owner call)
const SHADER := preload("res://assets/shaders/post.gdshader")

var preset := 3   # grade is the shipped default (owner call 2026-08-31); [ / ] still cycle, 0 = off
var _rect: ColorRect


func _ready() -> void:
	layer = 50
	_rect = ColorRect.new()
	_rect.set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)
	_rect.mouse_filter = Control.MOUSE_FILTER_IGNORE
	var mat := ShaderMaterial.new()
	mat.shader = SHADER
	_rect.material = mat
	_rect.visible = false
	add_child(_rect)
	set_preset(preset)


func cycle(dir: int) -> void:
	set_preset((preset + dir + NAMES.size()) % NAMES.size())


func set_preset(i: int) -> void:
	preset = i
	_rect.visible = preset != 0
	_rect.material.set_shader_parameter("preset", preset)


func preset_name() -> String:
	return NAMES[preset]
