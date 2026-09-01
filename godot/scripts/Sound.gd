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

# Mix hierarchy (owner, 2026-08-30): stingers/reads ~-3, loud one-shots -7..-3, crowd foley -12..-8.
# Cue-class offsets for unit combat foley — the 100+ crowd is the cacophony, hero cues stay on top.
const CUE_DB := {"attack": -10.0, "hit": -12.0, "death": -8.0, "ability": -4.0}
# Named one-shots and prefixes resolved in play(); first prefix match wins, unlisted files stay 0.
const FILE_DB := {"gatling_shot.wav": -5.0, "cannon_fire.wav": -3.0, "cannon_explosion.wav": -3.0,
	"bow_release_arrow.wav": -4.0, "stinger_": -3.0, "ui_": -3.0}

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
	# Runtime AudioServer.add_bus() silences ALL web audio on the single-threaded export
	# (engine outputs pure zeros; measured via analyser tap, 2026-09-01). Master-bus effects
	# are fine. So: web plays bare voices on Master with no stereo pan; desktop keeps the rig.
	# ponytail: web loses left/right panning — revisit if Godot fixes web add_bus.
	var web := OS.has_feature("web")
	# 12 voices of -10dB foley still sum ~+10dB over one; a brickwall on Master eats the stack
	AudioServer.add_bus_effect(0, AudioEffectHardLimiter.new())
	for i in range(POOL_SIZE):
		var p := AudioStreamPlayer.new()
		if not web:
			var bus_name := "SFX%d" % i
			AudioServer.add_bus()
			var idx := AudioServer.get_bus_count() - 1
			AudioServer.set_bus_name(idx, bus_name)
			AudioServer.set_bus_send(idx, "Master")
			var panner := AudioEffectPanner.new()
			AudioServer.add_bus_effect(idx, panner)
			_panners.append(panner)
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
	var base := 0.0
	for k: String in FILE_DB:
		if file == k or (k.ends_with("_") and file.begins_with(k)):
			base = FILE_DB[k]
			break
	p.volume_db = base + db
	var spread := PITCH_SPREAD_TONAL if (file.begins_with("stinger_") or file.begins_with("ui_")) else PITCH_SPREAD
	p.pitch_scale = randf_range(1.0 - spread, 1.0 + spread)
	if i < _panners.size():
		_panners[i].pan = clampf(x_lateral / HALL_HALF, -1.0, 1.0)
	p.play()
	probe_counts[file] = int(probe_counts.get(file, 0)) + 1


var _amb: AudioStreamPlayer


## Ambient bed on its own looping voice, outside the SFX pool; "" stops it. Menu drone sits
## well under the foley mix. Survives scene changes (autoload), so scenes opt in/out.
func ambient(file: String, db := -14.0) -> void:
	if _amb == null:
		_amb = AudioStreamPlayer.new()
		add_child(_amb)
	if file == "":
		_amb.stop()
		return
	var s := _load(file)
	if s == null:
		return
	if s is AudioStreamWAV:
		s.loop_mode = AudioStreamWAV.LOOP_FORWARD
		# frames via length*rate — data.size()/2 undercounts on compressed imports (QOA cut
		# the 24s menu drone to a ~5s loop)
		s.loop_end = int(s.get_length() * s.mix_rate)
	_amb.stream = s
	_amb.volume_db = db
	_amb.play()


## Resolve a unit type's cue (attack/hit/death/ability) through units.json and play it.
func unit(u_type: String, cue: String, x_lateral := 0.0, team := Unit.ALLY) -> void:
	var d: Dictionary = Game.units.get(u_type, {})
	var sfx: Dictionary = d.get("sfx", {})
	var db: float = CUE_DB.get(cue, 0.0) + (ENEMY_DB if team == Unit.ENEMY else 0.0)
	play(str(sfx.get(cue, "")), x_lateral, db)


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
