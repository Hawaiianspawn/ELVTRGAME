extends Node
## Autoload (registered after Game — resolves cues through Game.units). Pool of stereo-panned
## AudioStreamPlayer voices for combat SFX. The battle camera is fixed (Battle.gd/World3D.gd), so
## a plain left/right pan stands in for real 3D audio: each voice gets its own bus carrying one
## AudioEffectPanner (AudioStreamPlayer itself has no pan property in Godot 4).

const HALL_HALF := 200.0   # mirrors Battle.gd:21 — Battle has no class_name, unreachable from here
const POOL_SIZE := 12
const PER_FRAME_CAP := 4   # same file at most 4x/frame — the crowd is 100+ units
const PITCH_SPREAD := 0.08   # combat cues: ±8% (~1.3 semitones) — enough to stop a crowd phasing, not enough to change the sound
const PITCH_SPREAD_TONAL := 0.03   # stingers / UI chimes stay on key
const ENEMY_DB := -8.0   # enemy unit cues sit under the retinue — the player's side is the one to read
const DIR := "res://assets/sfx/"

var probe_counts := {}     # file -> total plays this run, printed by Probe at the end

var _players: Array[AudioStreamPlayer] = []
var _panners: Array[AudioEffectPanner] = []
var _next := 0              # round-robin cursor; doubles as "steal the oldest voice"
var _streams := {}          # file -> AudioStream (or null once warned missing)
var _warned := {}           # file -> true, one-time push_warning
var _frame_num := -1
var _frame_counts := {}     # file -> plays counted this frame, reset every frame
var _probe := false


func _ready() -> void:
	for a in OS.get_cmdline_user_args():
		if a.begins_with("--probe="):
			_probe = true
	for i in range(POOL_SIZE):
		var bus_name := "SFX%d" % i
		AudioServer.add_bus()
		var idx := AudioServer.get_bus_count() - 1
		AudioServer.set_bus_name(idx, bus_name)
		AudioServer.set_bus_send(idx, "Master")
		var panner := AudioEffectPanner.new()
		AudioServer.add_bus_effect(idx, panner)
		_panners.append(panner)
		var p := AudioStreamPlayer.new()
		p.bus = bus_name
		add_child(p)
		_players.append(p)


## Play a cue file (relative to assets/sfx/) panned across the hall width. x_lateral is a sim wx.
func play(file: String, x_lateral := 0.0, db := 0.0) -> void:
	if file == "":
		return
	var frame := Engine.get_process_frames()
	if frame != _frame_num:
		_frame_num = frame
		_frame_counts.clear()
	var n := int(_frame_counts.get(file, 0))
	if n >= PER_FRAME_CAP:
		return
	var stream := _load(file)
	if stream == null:
		return
	_frame_counts[file] = n + 1
	var i := _voice()
	var p := _players[i]
	p.stream = stream
	p.volume_db = db
	var spread := PITCH_SPREAD_TONAL if (file.begins_with("stinger_") or file.begins_with("ui_")) else PITCH_SPREAD
	p.pitch_scale = randf_range(1.0 - spread, 1.0 + spread)
	_panners[i].pan = clampf(x_lateral / HALL_HALF, -1.0, 1.0)
	p.play()
	probe_counts[file] = int(probe_counts.get(file, 0)) + 1


## Resolve a unit type's cue (attack/hit/death/ability) through units.json and play it.
func unit(u_type: String, cue: String, x_lateral := 0.0, team := Unit.ALLY) -> void:
	var d: Dictionary = Game.units.get(u_type, {})
	var sfx: Dictionary = d.get("sfx", {})
	play(str(sfx.get(cue, "")), x_lateral, ENEMY_DB if team == Unit.ENEMY else 0.0)


func _voice() -> int:
	for i in range(POOL_SIZE):
		if not _players[i].playing:
			return i
	# ponytail: steal-oldest approximated by round-robin over pool order, not real timestamps —
	# good enough for a 12-voice SFX pool; track last-play time per voice if that ever bites.
	var i := _next
	_next = (_next + 1) % POOL_SIZE
	return i


func _load(file: String) -> AudioStream:
	if _streams.has(file):
		return _streams[file]
	var path := DIR + file
	if not ResourceLoader.exists(path):
		if not _warned.has(file):
			_warned[file] = true
			push_warning("Sound: missing sfx file " + path)
		_streams[file] = null
		return null
	var s := load(path) as AudioStream
	_streams[file] = s
	return s


## Probe.gd's own --probe= detection isn't reachable from here (no class_name, different
## autoload), so this self-detects the same cmdline flag in _ready() and fires on the same
## Node teardown hook Battle.gd already uses for end-of-run cleanup (see Battle.gd:448).
## Format is load-bearing: godot-run.ps1 only echoes lines matching ^PROBE|SCRIPT ERROR|Assertion failed.
func _exit_tree() -> void:
	if _probe:
		for k in probe_counts:
			print("PROBE SFX %s x%d" % [k, int(probe_counts[k])])
