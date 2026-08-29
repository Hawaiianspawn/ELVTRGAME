class_name ArmyPanel
extends PanelContainer
## One bottom-strip army panel. Compact + idle line for the three unselected types; large, on top,
## and showing a random swap-in line for whichever type just stepped up. Battle.gd owns four of
## these (one per Army.TYPES entry) and drives selection from `_set_army`.

const COMPACT_SIZE := Vector2(222, 64)
const SELECTED_SIZE := Vector2(260, 112)
const BOTTOM_Y := 538.0

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
	box.add_theme_constant_override("separation", 0)
	add_child(box)
	var row := HBoxContainer.new()
	row.add_theme_constant_override("separation", 8)
	box.add_child(row)
	_tag = Ui.label(row, str(data.get("tag", data.get("label", type))).to_upper(), Vector2.ZERO, Ui.COL_TEXT)
	_tag.add_theme_font_override("font", Ui.font("head16"))
	_count = Ui.label(row, "x0", Vector2.ZERO, Ui.COL_TEXT)
	_count.add_theme_font_override("font", Ui.font("head16"))
	_ability = Ui.label(box, "", Vector2.ZERO, Ui.COL_DIM)
	_blurb = Ui.label(box, str(data.get("idle", "")), Vector2.ZERO, Ui.COL_DIM)
	_blurb.text_overrun_behavior = TextServer.OVERRUN_TRIM_ELLIPSIS
	size = COMPACT_SIZE
	position = Vector2(slot_x, BOTTOM_Y - COMPACT_SIZE.y)


## Kit stone card (idle) / gold-cornered card (selected), nine-sliced to the panel size.
static func _style(is_selected: bool) -> StyleBoxTexture:
	var sb := Ui.nine("panel_sel" if is_selected else "panel_idle", 40 if is_selected else 22)
	sb.set_content_margin_all(30 if is_selected else 12)   # gold filigree corners are wide; keep text off them
	sb.content_margin_top = 16 if is_selected else 8
	sb.content_margin_bottom = 8
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
	_tag.add_theme_color_override("font_color", Ui.COL_EMBER if selected else Ui.COL_TEXT)
	if selected:
		move_to_front()
		var lines: Array = data.get("swap", [])
		_blurb.text = String(lines[rng.randi_range(0, lines.size() - 1)]) if not lines.is_empty() else str(data.get("idle", ""))
		_blurb.add_theme_color_override("font_color", Ui.COL_TEXT)
		_blurb.autowrap_mode = TextServer.AUTOWRAP_WORD_SMART
		_blurb.text_overrun_behavior = TextServer.OVERRUN_NO_TRIMMING
	else:
		_blurb.text = str(data.get("idle", ""))
		_blurb.add_theme_color_override("font_color", Ui.COL_DIM)
		_blurb.autowrap_mode = TextServer.AUTOWRAP_OFF
		_blurb.text_overrun_behavior = TextServer.OVERRUN_TRIM_ELLIPSIS
	var target_size := SELECTED_SIZE if selected else COMPACT_SIZE
	var target_x := clampf(slot_x - (target_size.x - COMPACT_SIZE.x) / 2.0, 4.0, 960.0 - 4.0 - target_size.x)
	var target_pos := Vector2(target_x, BOTTOM_Y - target_size.y)
	if _tween:
		_tween.kill()
	_tween = create_tween().set_trans(Tween.TRANS_CUBIC).set_parallel(true)
	_tween.tween_property(self, "size", target_size, 0.25)
	_tween.tween_property(self, "position", target_pos, 0.25)
