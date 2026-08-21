class_name ArmyPanel
extends PanelContainer
## One bottom-strip army panel. Compact + idle line for the three unselected types; large, on top,
## and showing a random swap-in line for whichever type just stepped up. Battle.gd owns four of
## these (one per Army.TYPES entry) and drives selection from `_set_army`.

const COMPACT_SIZE := Vector2(222, 50)
const SELECTED_SIZE := Vector2(260, 100)
const BOTTOM_Y := 538.0
const COL_IDLE := "#a0a08b"
const COL_BRIGHT := "#e9efec"
const COL_SELECTED := "#f0c260"

var type: String
var data: Dictionary        # Game.units[type]: label, ability_label, tag, swap, idle
var slot_x: float           # compact-state left edge, fixed per panel
var selected := false

var _tag: Label
var _count: Label
var _ability: Label
var _blurb: Label
var _tween: Tween


func _init(unit_type: String, unit_data: Dictionary, x: float) -> void:
	type = unit_type
	data = unit_data
	slot_x = x
	clip_contents = true
	add_theme_stylebox_override("panel", _style(false))
	var box := VBoxContainer.new()
	box.add_theme_constant_override("separation", 1)
	add_child(box)
	var row := HBoxContainer.new()
	box.add_child(row)
	_tag = _mk_label(row, str(data.get("tag", data.get("label", type))).to_upper(), 11, Color(COL_BRIGHT))
	_count = _mk_label(row, "x0", 11, Color(COL_BRIGHT))
	_ability = _mk_label(box, "", 10, Color(COL_IDLE))
	_blurb = _mk_label(box, str(data.get("idle", "")), 10, Color(COL_IDLE))
	_blurb.text_overrun_behavior = TextServer.OVERRUN_TRIM_ELLIPSIS
	size = COMPACT_SIZE
	position = Vector2(slot_x, BOTTOM_Y - COMPACT_SIZE.y)


func _mk_label(parent: Node, text: String, fsize: int, col: Color) -> Label:
	var l := Label.new()
	l.text = text
	l.add_theme_font_size_override("font_size", fsize)
	l.add_theme_color_override("font_color", col)
	parent.add_child(l)
	return l


## Dark stone panel look, kept in one place so a generated frame texture can swap in for this later.
static func _style(is_selected: bool) -> StyleBoxFlat:
	var sb := StyleBoxFlat.new()
	sb.bg_color = Color(0.043, 0.051, 0.047, 0.85)   # #0b0d0c
	sb.set_border_width_all(3 if is_selected else 2)
	sb.border_color = Color(COL_SELECTED) if is_selected else Color(0.35, 0.35, 0.32)
	sb.set_content_margin_all(4)
	return sb


## Called every frame from Battle._draw_hud with the live rank count / cooldown.
func update(count: int, cd: float) -> void:
	_count.text = "x%d" % count
	_ability.text = "%s%s" % [str(data.get("ability_label", "")), "" if cd <= 0.0 else " %.0fs" % cd]


## Fires when this panel's type becomes (or stops being) the active army. Grows/shrinks with a
## tween and, on becoming selected, picks a fresh random swap-in line and comes to draw front.
func set_selected(is_selected: bool, rng: RandomNumberGenerator) -> void:
	if is_selected == selected:
		return
	selected = is_selected
	add_theme_stylebox_override("panel", _style(selected))
	_tag.add_theme_color_override("font_color", Color(COL_SELECTED) if selected else Color(COL_BRIGHT))
	if selected:
		move_to_front()
		var lines: Array = data.get("swap", [])
		_blurb.text = String(lines[rng.randi_range(0, lines.size() - 1)]) if not lines.is_empty() else str(data.get("idle", ""))
		_blurb.add_theme_color_override("font_color", Color(COL_BRIGHT))
		_blurb.autowrap_mode = TextServer.AUTOWRAP_WORD_SMART
		_blurb.text_overrun_behavior = TextServer.OVERRUN_NO_TRIMMING
	else:
		_blurb.text = str(data.get("idle", ""))
		_blurb.add_theme_color_override("font_color", Color(COL_IDLE))
		_blurb.autowrap_mode = TextServer.AUTOWRAP_OFF
		_blurb.text_overrun_behavior = TextServer.OVERRUN_TRIM_ELLIPSIS
	var target_size := SELECTED_SIZE if selected else COMPACT_SIZE
	var target_x := slot_x - (target_size.x - COMPACT_SIZE.x) / 2.0
	var target_pos := Vector2(target_x, BOTTOM_Y - target_size.y)
	if _tween:
		_tween.kill()
	_tween = create_tween().set_trans(Tween.TRANS_CUBIC).set_parallel(true)
	_tween.tween_property(self, "size", target_size, 0.25)
	_tween.tween_property(self, "position", target_pos, 0.25)
