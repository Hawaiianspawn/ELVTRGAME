extends Node
## Autoload. Owns data tables, run state, scene routing and the shared sprite helper.

const VERSION := "0.1.0"
const DIRS := ["south", "south-east", "east", "north-east", "north", "north-west", "west", "south-west"]
const SCENES := {
	"main": "res://scenes/Main.tscn",
	"battle": "res://scenes/battle/Battle.tscn",
}

var units: Dictionary
var waves: Array
var spells: Dictionary
var sprites: Dictionary          # name -> {cell, source}

# Run state (survives scene changes; checkpoint = start of wave)
var wave: int = 0
var magic: float = 0.0
var magic_ever: float = 0.0      # relic thresholds read this
var score: int = 0               # juggle kills: enemies that die in the air
var kills: int = 0               # every enemy felled this run; the end tally reads it
var relics: Array[String] = []
var hero_hp: float = 100.0

const HEROES := ["hero_turret"]  # main build ships the tank turret only
var hero: String = HEROES[0]     # sprite name of the hero the player is running as

var _fade: ColorRect


func _ready() -> void:
	units = _json("res://data/units.json")
	waves = _json("res://data/waves.json")
	spells = _json("res://data/spells.json")
	sprites = _json("res://assets/sprites/manifest.json")
	# every Label takes the pixel body font from assets/ui/kindled.tres (project.godot gui/theme/custom)
	Input.set_custom_mouse_cursor(Ui.tex("cursor"), Input.CURSOR_ARROW, Vector2(3, 2))
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
	score = 0
	kills = 0
	relics.clear()
	hero_hp = 100.0


var _atlas: Texture2D


## Sprite2D for a packed 8-direction strip. `facing` is an index into DIRS.
## Every sprite is a region of one shared atlas so the canvas batcher draws the whole crowd in a few calls.
func make_sprite(name: String, facing: int = 0) -> Sprite2D:
	if _atlas == null:
		_atlas = load("res://assets/sprites/atlas.png")
	var s := Sprite2D.new()
	var at := AtlasTexture.new()
	at.atlas = _atlas
	var cell_px: int = int(sprites[name]["cell"])
	at.region = Rect2(int(sprites[name].get("x", 0)), int(sprites[name]["y"]), cell_px * 8, cell_px)
	s.texture = at
	s.hframes = 8
	s.frame = facing
	s.centered = true
	# feet on the origin: cell is square, sprite bottom-aligned in the cell
	var cell: int = int(sprites[name]["cell"])
	s.offset = Vector2(0, -cell / 2.0)
	return s


## A one-shot effect strip from the atlas (manifest fx_* rows): hframes = clip length, frame 0, centred.
func make_fx(name: String) -> Sprite2D:
	if _atlas == null:
		_atlas = load("res://assets/sprites/atlas.png")
	var s := Sprite2D.new()
	var at := AtlasTexture.new()
	at.atlas = _atlas
	var cell: int = int(sprites[name]["cell"])
	var n: int = int(sprites[name]["frames"])
	at.region = Rect2(int(sprites[name].get("x", 0)), int(sprites[name]["y"]), cell * n, cell)
	s.texture = at
	s.hframes = n
	s.frame = 0
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


func _notification(what: int) -> void:
	## Mute everything while the window (or browser tab) is unfocused.
	match what:
		NOTIFICATION_APPLICATION_FOCUS_OUT:
			AudioServer.set_bus_mute(0, true)
		NOTIFICATION_APPLICATION_FOCUS_IN:
			AudioServer.set_bus_mute(0, false)
