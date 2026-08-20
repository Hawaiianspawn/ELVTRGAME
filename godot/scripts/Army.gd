class_name Army
extends RefCounted
## Builds the company block: packed ranks of real units from the front line down past the camera.
## `host` must expose `view`, `advancing`, `CREEP`, `lane_x()` (Unit reads them); Battle, Charge and Cavalry all do.

const RANK_X := 36.0
const RANK_STEP := 24.0
const TYPES := ["veteran", "halberdier", "hammer", "vet_ranged"]


## Interleave a {type: count} mix into a repeating pattern so every rank reads as a mixed line.
static func pattern(mix: Dictionary) -> Array[String]:
	var out: Array[String] = []
	var left := mix.duplicate()
	while true:
		var any := false
		for t in TYPES:
			if int(left.get(t, 0)) > 0:
				out.append(t)
				left[t] = int(left[t]) - 1
				any = true
		if not any:
			break
	return out


## Slot positions for `rows` ranks, `half` world units either side of centre, first rank at depth `d0`.
## `cols` > 0 fixes the file count and spreads it across `half*2`; 0 packs at RANK_X. `step` is the rank depth spacing.
static func slots(half: float, rows: int, d0: float, rng: RandomNumberGenerator, cols := 0, step := RANK_STEP) -> Array[Vector2]:
	var per_row := cols if cols > 0 else int(floor(half * 2.0 / RANK_X)) + 1
	var dx := half * 2.0 / maxf(per_row - 1, 1) if cols > 0 else RANK_X
	var out: Array[Vector2] = []
	for k in range(per_row * rows):
		var row := k / per_row
		var col := k % per_row
		out.append(Vector2(-half + col * dx + (row % 2) * dx * 0.5 + rng.randf_range(-3, 3), d0 - row * step + rng.randf_range(-2, 2)))
	return out


## Fill a block with real units (cutscenes): mixed types, standing in rank.
static func block(host: Node2D, parent: Node2D, mix: Dictionary, half: float, rows: int, d0: float, rng: RandomNumberGenerator, d_offset := 0.0) -> Array[Unit]:
	var pat := pattern(mix)
	var out: Array[Unit] = []
	var k := 0
	for sl in slots(half, rows, d0, rng):
		var u := Unit.new()
		u.setup(pat[k % pat.size()], Unit.ALLY, host)
		u.state = Unit.State.RANK
		u.wx = sl.x
		u.wd = sl.y + d_offset
		u.hold_d = u.wd
		u.home = Vector2(u.wx, u.wd)
		parent.add_child(u)
		out.append(u)
		k += 1
	return out
