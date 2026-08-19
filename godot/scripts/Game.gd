extends Node
## Autoload. Owns data tables, run state, scene routing and the shared sprite helper.

const DIRS := ["south", "south-east", "east", "north-east", "north", "north-west", "west", "south-west"]
const SCENES := {
	"battle": "res://scenes/battle/Battle.tscn",
	"charge": "res://scenes/battle/Charge.tscn",
	"cavalry": "res://scenes/cutscene/Cavalry.tscn",
	"messhall": "res://scenes/messhall/MessHall.tscn",
	"road": "res://scenes/road/Road.tscn",
	"ride": "res://scenes/ride/Ride.tscn",
	"reveal": "res://scenes/cutscene/Reveal.tscn",
}

var units: Dictionary
var waves: Array
var spells: Dictionary
var sprites: Dictionary          # name -> {cell, source}
var dialogue: Dictionary

# Run state (survives scene changes; checkpoint = start of wave)
var wave: int = 0
var magic: float = 0.0
var magic_ever: float = 0.0      # relic thresholds read this
var relics: Array[String] = []
var hero_hp: float = 100.0
var charged := false             # the ride-and-circle segment was played; Cavalry resumes at the killing stroke

var _fade: ColorRect


func _ready() -> void:
	units = _json("res://data/units.json")
	waves = _json("res://data/waves.json")
	spells = _json("res://data/spells.json")
	sprites = _json("res://assets/sprites/manifest.json")
	dialogue = _json("res://data/dialogue/messhall.json")
	var layer := CanvasLayer.new()
	layer.layer = 100
	_fade = ColorRect.new()
	_fade.color = Color.BLACK
	_fade.size = Vector2(960, 540)
	_fade.mouse_filter = Control.MOUSE_FILTER_IGNORE
	_fade.modulate.a = 0.0
	layer.add_child(_fade)
	add_child(layer)


func _json(path: String) -> Variant:
	return JSON.parse_string(FileAccess.get_file_as_string(path))


func goto(scene: String) -> void:
	await fade(1.0, 0.5)
	get_tree().change_scene_to_file(SCENES[scene])
	await get_tree().process_frame
	await fade(0.0, 0.5)


func fade(to: float, secs: float) -> void:
	var t := create_tween()
	t.tween_property(_fade, "modulate:a", to, secs)
	await t.finished


func reset_run() -> void:
	wave = 0
	magic = 0.0
	magic_ever = 0.0
	relics.clear()
	hero_hp = 100.0
	charged = false


## Sprite2D for a packed 8-direction strip. `facing` is an index into DIRS.
func make_sprite(name: String, facing: int = 0) -> Sprite2D:
	var s := Sprite2D.new()
	s.texture = load("res://assets/sprites/%s.png" % name)
	s.hframes = 8
	s.frame = facing
	s.centered = true
	# feet on the origin: cell is square, sprite bottom-aligned in the cell
	var cell: int = int(sprites[name]["cell"])
	s.offset = Vector2(0, -cell / 2.0)
	return s


func facing_from(v: Vector2) -> int:
	# 0=south … clockwise per DIRS. Screen space: +y is south.
	var a := rad_to_deg(atan2(v.x, v.y))   # 0 = south, +90 = east
	if a < 0.0:
		a += 360.0
	return int(round(a / 45.0)) % 8


func relic_bonus(key: String) -> float:
	var total := 0.0
	for r in spells["relics"]:
		if r["id"] in relics and r.has(key):
			total += float(r[key])
	return total


func has_relic_flag(key: String) -> bool:
	for r in spells["relics"]:
		if r["id"] in relics and r.get(key, false):
			return true
	return false


func gain_magic(amount: float) -> Array[String]:
	## Returns newly unlocked relic ids.
	amount *= 1.0 + relic_bonus("gather")
	magic += amount
	magic_ever += amount
	var fresh: Array[String] = []
	for r in spells["relics"]:
		if magic_ever >= float(r["at"]) and not (r["id"] in relics):
			relics.append(r["id"])
			fresh.append(r["id"])
	return fresh
