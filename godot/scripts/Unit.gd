class_name Unit
extends Node3D
## One combatant in world space (wx lateral, wd depth), standing at Vector3(wx, 0, -wd). Static
## 8-dir art on a Sprite3D: walk bob + hit flash. Heights and nudges (air_h, bob, lunge, _pivot)
## stay in SPRITE PIXELS, the unit every constant here is tuned in; Hall3D.PIXEL converts to world.

signal died(unit: Unit)

const ALLY := 0
const ENEMY := 1
const RUSH := 10.0
const GRAV := 125.0          # one gravity, up and down: a 250 throw peaks at 250 and hangs ~4s, slowing into the apex
const JUGGLE_MULT := 2.0     # damage bonus while airborne
const POP := 45.0            # upward impulse added by every juggle hit: extends the hang, doesn't relaunch
const FLOAT_V := 50.0        # |air_v| under this = the apex: gravity eases so they hang there
const FLOAT_G := 0.35
const AGGRO := 150.0         # ranks wake up to enemies this close (was 90: only the first row ever fought)
const BOSS_GIB_HP := 300.0   # max_hp at or above this = boss-tier: sheds gib chunks on airborne hits, not just death
const WALL_INSET := 32.0     # world units the clamp keeps a unit's centre off the wall face: ~half a visible body (14-15 sprite px) so billboards don't sink into the brick. ponytail: flat inset, per-sprite half-width if big cells still clip
const SUPPORT_DMG := 0.5     # rank hits beyond own weapon reach land at this fraction
enum State { RANK, ADVANCE, FIGHT, RETREAT }

var type: String
var team: int
var lane: int = -1
var state := State.FIGHT
var wx := 0.0
var wd := 0.0
var hp: float
var max_hp: float
var dmg: float
var rng: float
var speed: float
var cooldown: float
var counters: Array
var hold_d := 0.0            # allies stop here; enemies push past it toward the hero
var home := Vector2.ZERO     # rank slot this ally came from and returns to when swapped out
var rooted_until := 0.0
var _cd := 0.0
var _t := randf() * 10.0           # desynced walk phase
var _gait := randf_range(0.85, 1.2)
var _mirror := false                # enemy-only: sprite flipped left/right so a crowd of one look reads as two
var _moving := false
var _lunge := Vector2.ZERO
var _recoil := Vector2.ZERO
var _flash_hold := 0.0             # seconds left at full hit-flash before it decays
const FLASH_PEAK := 0.7            # white blend on hit: 0.7 keeps the sprite readable, 1.0 blanked it
const FLASH_HOLD := 0.09           # the beat the flash sits before the 8/s lerp takes it home
var dead := false
var opening := false        # fires the swap-in ability when it reaches the line
var guard_until := 0.0
var spin_until := 0.0        # whirl: spinning dodge rolls until then, i-frames in take()
var spin_phase := 0.0        # 0 or 1: staggers the burst/breather beat so the field alternates
var _roll_goal := Vector2.ZERO
var charge_to := 0.0         # halberdier charge: run to this depth launching everyone passed, then fall back
var _charge_from := Vector2.ZERO
var _charge_hit := {}        # enemy -> its wd at the moment first hit (for the arrival blow's throw distance)
var _charge_back := false    # leg: false = out, true = home
var _charge_dur := -1.0      # this charge's synced-arrival duration (T); -1 = not yet armed
var _charge_wd0 := 0.0       # this charger's own wd when the out leg started
var _charge_end := 0.0       # this charger's own out-leg landing depth (rows keep their gap, not stack on charge_to)
var _charge_t0 := 0.0        # wall-clock time the out leg started
var _charge_thrown := false  # true while this unit is mid-flight from a charge blow (skip a second throw)
var _charge_hold_until := -1.0   # wall-clock: charger stands at the line until this, then falls back; -1 = not holding
const CHARGE_SPEED := 4.0    # x walk speed out; back at rush
const CHARGE_R := 34.0       # reach either side of the charging halberd
const CHARGE_HOLD := 0.15    # seconds the charger freezes at the line before the fall-back leg starts
const CHARGE_KEEP := 1.0     # 1.0 = rows land with the same gap they started with; lower closes it up
const PILE_HOLD := 2.0       # seconds a bulldozed enemy stays rooted on the pile line
var rush := false            # ADVANCE/RETREAT at rush speed (swaps route behind the camera)
var sky_slam := false        # hammer entrance: dropped from the sky, slams the ground on landing
var air_h := 0.0             # height above the ground; > 0 = airborne, helpless, juggleable
var air_v := 0.0             # vertical speed
var air_vx := 0.0            # lateral drift while airborne: scattered on every launch, biased toward screen centre
var air_vd := 0.0            # depth drift while airborne: the charge blow throws with this, aimed at a landing depth
var _spin := 0.0             # rad/s tumble while airborne
var _pivot := 0.0            # px from feet up to the center of mass; spin pivots here
var _base_offset := Vector2.ZERO   # make_sprite's feet-on-origin offset, restored on landing
var _juggles := 0            # hits taken while airborne; scales the air-kill score
var _leg := 0                # RETREAT: 0 = run back past the camera, 1 = walk into the vacated slot
var battle: Node3D
var sprite: Sprite3D
const ATK_DT := 0.06               # seconds per attack frame
var _atk_t := -1.0                 # time into the active clip, <0 = idle
var _atk := {}                     # manifest "attack" entry: {frames, y}; empty = no clip packed
var _slam := {}                    # manifest "slam" entry: sky-drop landing clip
var _death := {}                   # manifest "death" entry: collapse, holds its last frame
var _walk := {}                    # manifest "walk" entry: loop while _moving, replaces the bob
var _walking := false
var _clip := {}                    # whichever clip is playing
var _hurt: Array = []              # manifest hurt1..hurt7 (+ bare "hurt") clips; empty = no hit-reactions
var _hurt_combo := 0               # cycles through _hurt while hits land inside the combo window
var _last_hurt_t := -10.0          # wall-clock of the last hurt clip; combo resets past 0.8s idle
var _last_gib_t := -10.0           # wall-clock of the last juggle-gib burst (boss-tier only)
var _atk_hold := 0.0               # units.json attack_hold: seconds the strike frame lingers before guard
var _rot_region: Rect2
var _cell := 48.0                  # sprite cell size in px


func setup(p_type: String, p_team: int, p_battle: Node3D) -> void:
	type = p_type
	team = p_team
	battle = p_battle
	var d: Dictionary = Game.units[type]
	var hp_mult := 1.0 + (Game.relic_bonus("hp") if team == ALLY else 0.0)
	max_hp = float(d["hp"]) * hp_mult
	hp = max_hp
	dmg = float(d["dmg"])
	rng = float(d["range"])
	speed = float(d["speed"]) * 2.2
	cooldown = float(d["cooldown"])
	counters = d["counters"]
	sprite = Hall3D.make_sprite3d(d["sprite"], 4 if team == ALLY else 0)
	if team == ENEMY and randf() < 0.5:
		_mirror = true
		sprite.flip_h = true
	add_child(sprite)
	_rot_region = (sprite.texture as AtlasTexture).region
	_cell = _rot_region.size.y
	_base_offset = sprite.offset
	# alpha-centroid height above the feet, measured at pack time (godot_pack.py "com")
	_pivot = float(Game.sprites[d["sprite"]].get("com", _rot_region.size.y * 0.42))
	_atk = Game.sprites[d["sprite"]].get("attack", {})
	_slam = Game.sprites[d["sprite"]].get("slam", {})
	_death = Game.sprites[d["sprite"]].get("death", {})
	_walk = Game.sprites[d["sprite"]].get("walk", {})
	_atk_hold = float(d.get("attack_hold", 0.0))
	var sd: Dictionary = Game.sprites[d["sprite"]]
	var i := 1
	while sd.has("hurt%d" % i):
		_hurt.append(sd["hurt%d" % i])
		i += 1
	if sd.has("hurt"):
		_hurt.append(sd["hurt"])


## Play the packed attack clip (allies north, enemies south); the static frame comes back when it ends.
func attack_anim() -> void:
	if not _atk.is_empty():
		_clip = _atk
		_atk_t = 0.0
	Sound.unit(type, "attack", wx, team)


## Play the packed north-facing slam clip (sky-drop landing).
func slam_anim() -> void:
	if not _slam.is_empty():
		_clip = _slam
		_atk_t = 0.0


func _tick_attack(delta: float) -> void:
	if _atk_t < 0.0:
		return
	_atk_t += delta
	var n := int(_clip["frames"])
	var i := mini(int(_atk_t / ATK_DT), n - 1)   # last frame is the strike: it holds for attack_hold
	if _clip == _death:
		Hall3D.clip_frame(sprite, _rot_region, _clip, i)   # a corpse holds its last frame
	elif _atk_t >= n * ATK_DT + _atk_hold:
		_atk_t = -1.0
		Hall3D.rotation_frame(sprite, _rot_region, 4 if team == ALLY else 0)
	else:
		Hall3D.clip_frame(sprite, _rot_region, _clip, i)


func _process(delta: float) -> void:
	if dead:
		_tick_attack(delta)   # the death clip keeps playing under the fade
		return
	_t += delta
	_cd -= delta
	_moving = false
	if Time.get_ticks_msec() / 1000.0 < rooted_until:
		sprite.modulate = Color(0.6, 1.0, 0.6)
	elif air_h > 0.0 or air_v > 0.0:
		# airborne: gravity only — no walking, no swinging, just tumble and be juggled
		# sky-dropped hammers plummet; juggled units keep the floaty cap
		# plain ballistics: sky-dropped hammers plummet under heavy gravity, everything else floats the same
		var g := 6.0 if sky_slam else (FLOAT_G if absf(air_v) < FLOAT_V else 1.0)
		air_v -= GRAV * g * delta
		air_h = maxf(0.0, air_h + air_v * delta)
		wx += air_vx * delta
		wd += air_vd * delta
		# near the lens the hall is wider than the frame: bounce off the screen edge when it's
		# tighter than the wall, so thrown units never sail out of frame (lateral twin of the
		# air_h top-edge cap below). curve_dx shifts the visible centre when the hall bends.
		var lim: float = battle.HALL_HALF - WALL_INSET
		var scr: float = 0.5 * battle.camera.get_viewport().get_visible_rect().size.x / Hall3D.screen_scale(battle.camera, wd) - WALL_INSET
		var dx := Hall3D.curve_dx(wd)
		var lo := maxf(-lim, -scr - dx)
		var hi := minf(lim, scr - dx)
		if wx < lo or wx > hi:
			wx = clampf(wx, lo, hi)
			air_vx = -air_vx * 0.5   # ricochet, damped, instead of smearing along the edge
		var cap := _air_cap()
		if air_h > cap and not sky_slam:
			air_h = cap   # juggle as high as you like, but never out of the frame: bounce off the top edge
			air_v = minf(air_v, 0.0)
		sprite.rotation.z -= _spin * delta   # +z is counter-clockwise on screen: negate to tumble the old way
		if air_h == 0.0:
			air_v = 0.0
			air_vx = 0.0
			air_vd = 0.0
			_spin = 0.0
			sprite.rotation.z = 0.0
			sprite.offset = _base_offset
			if _charge_thrown:
				_charge_thrown = false
				rooted_until = Time.get_ticks_msec() / 1000.0 + PILE_HOLD
			if sky_slam:
				sky_slam = false
				slam_anim()
				battle.sky_landing(self)
		sprite.modulate = sprite.modulate.lerp(Color.WHITE, delta * 8.0)   # fog is the Environment's job now
	elif charge_to > 0.0:
		sprite.modulate = sprite.modulate.lerp(Color.WHITE, delta * 8.0)   # fog is the Environment's job now
		if not _charge_back:
			# out: straight down-range, halberd levelled, everything passed goes up
			if _charge_dur < 0.0:
				# first out-leg frame: sync to the lead of this same charge (same type/team/target)
				# so every row's block compresses to zero depth and lands the blow together
				var lead_wd := wd
				for o2 in battle.units:
					if o2 != self and o2.team == team and o2.type == type and o2.charge_to == charge_to and not o2.dead:
						lead_wd = maxf(lead_wd, o2.wd)
				_charge_dur = maxf(0.15, (charge_to - lead_wd) / (speed * CHARGE_SPEED))
				_charge_wd0 = wd
				_charge_end = charge_to - (lead_wd - wd) * CHARGE_KEEP
				_charge_t0 = Time.get_ticks_msec() / 1000.0
			var ct := clampf((Time.get_ticks_msec() / 1000.0 - _charge_t0) / _charge_dur, 0.0, 1.0)
			wd = lerpf(_charge_wd0, _charge_end, ct)
			var arrived := wd >= _charge_end
			_moving = true
			sprite.frame = 4
			for o in battle.units:
				if o.team != team and not o.dead and absf(o.wx - wx) < CHARGE_R and o.wd >= wd - 4.0 and o.wd < wd + CHARGE_R:
					if not _charge_hit.has(o):
						_charge_hit[o] = o.wd   # depth at first contact: how the arrival blow throws it
						attack_anim()
						battle.hit(self, o, 2.0)   # one blow on contact
					# then bulldozed: carried on the blade to the pile line, pulled into the file, held there
					o.wd = wd + CHARGE_R
					o.wx = move_toward(o.wx, wx, 80.0 * delta)
					o.rooted_until = Time.get_ticks_msec() / 1000.0 + PILE_HOLD
			if arrived and _charge_hold_until < 0.0:
				# freeze at the line for a beat so the block is still there when the blow lands,
				# instead of already peeling off into the fall-back leg
				_charge_hold_until = Time.get_ticks_msec() / 1000.0 + CHARGE_HOLD
				_deliver_blow()
			if _charge_hold_until >= 0.0 and Time.get_ticks_msec() / 1000.0 >= _charge_hold_until:
				_charge_back = true
				_charge_hold_until = -1.0
		else:
			# back: rush home, then the normal state takes over
			var saved := rush
			rush = true
			_step(_charge_from, delta)
			rush = saved
			if not _moving:
				charge_to = 0.0
				_charge_hit.clear()
				sprite.frame = 4
	elif Time.get_ticks_msec() / 1000.0 < spin_until and (state == State.FIGHT or state == State.RANK):
		# whirl: 1s spin bursts with 1s breathers. Spinning = hard lateral darts angled slightly
		# back, i-frames in take(), blade clips anyone in reach. Breather = plant, face N or S.
		sprite.modulate = sprite.modulate.lerp(Color.WHITE, delta * 8.0)   # fog is the Environment's job now
		if spinning():
			# erratic: short darts, ~1.5 mid-dart rethinks a second; exponential lerp = fast
			# launch easing out into the stop, the dodge-roll pop
			if _roll_goal == Vector2.ZERO or Vector2(wx, wd).distance_to(_roll_goal) < 4.0 or randf() < 1.5 * delta:
				var side := 1.0 if randf() < 0.5 else -1.0
				var roll_lim: float = minf(320.0, battle.HALL_HALF - WALL_INSET)
				_roll_goal = Vector2(clampf(wx + side * randf_range(30.0, 90.0), -roll_lim, roll_lim), clampf(wd + randf_range(-28.0, 6.0), hold_d - 40.0, hold_d + 20.0))   # darts drift, but never off the slot or into the wall
			var here := Vector2(wx, wd).lerp(_roll_goal, 1.0 - exp(-7.0 * delta))
			wx = here.x
			wd = here.y
			_moving = true
			if _atk_t < 0.0:
				sprite.frame = 4   # the sheet's attack clip is the only swing art; no rotation twirl
			var foe: Unit = battle.near_enemy(self, 45.0)
			if foe and _cd <= 0.0:
				_cd = cooldown
				battle.hit(self, foe)
		else:
			_roll_goal = Vector2.ZERO
			if _atk_t < 0.0:
				sprite.frame = 4
	else:
		sprite.modulate = sprite.modulate.lerp(Color.WHITE, delta * 8.0)   # fog is the Environment's job now
		match state:
			State.RANK:
				_moving = battle.advancing
				# live in the ranks: spring back to the home slot, shoved by neighbours and the hero
				var here := Vector2(wx, wd)
				var push: Vector2 = battle.separation(self)
				here += (home - here) * minf(1.0, 3.0 * delta) + push * delta
				wx = here.x
				wd = here.y
				# anything that breaks through gets fought where it stands; rows further back
				# join in with half-strength support hits over the shoulders of the row ahead
				# long weapons wake up further out: reach scales the aggro bubble
				var aggro := maxf(AGGRO, rng + 60.0)
				var foe: Unit = battle.near_enemy(self, aggro)
				if foe:
					if _atk_t < 0.0:   # a playing clip owns the frame index (clips are 5 wide, facings 8)
						_face(Vector2(foe.wx - wx, -(foe.wd - wd)))
					if _cd <= 0.0:
						var d_foe := _dist(foe)
						if d_foe <= maxf(rng, 45.0):
							_cd = cooldown
							battle.hit(self, foe)
						elif d_foe <= aggro:
							_cd = cooldown
							battle.hit(self, foe, SUPPORT_DMG)
				elif not _moving:
					sprite.frame = 4
			State.RETREAT:
				_step(Vector2(wx, 45.0), delta)
				if wd < 70.0:
					battle.recall(self)      # back into its type's pool
					return
			State.ADVANCE:
				_step(Vector2(battle.lane_x(lane), hold_d) if lane >= 0 else home, delta)
				if not _moving:
					rush = false
					if lane >= 0:
						state = State.FIGHT
						if opening:
							opening = false
							battle.arrived(self)
					else:
						state = State.RANK
						sprite.frame = 4
			State.FIGHT:
				var target: Node3D = battle.find_target(self)
				if target and _dist(target) <= rng:
					if _cd <= 0.0:
						_cd = cooldown
						battle.hit(self, target)
					if team == ENEMY and _atk_t < 0.0:   # a playing clip owns the frame index
						_face(Vector2(target.wx - wx, -(target.wd - wd)) if target is Unit else Vector2(0, 1))
				else:
					var goal: Vector2
					if team == ENEMY:
						# not on rails: head for whoever is nearest, else the hero; shoulder past your own kind
						goal = Vector2(target.wx, target.wd) if target is Unit else (Vector2(battle.hero_wx, battle.hero_wd) if target == battle.hero else Vector2(wx, hold_d))
						goal += battle.separation(self) * 0.35
					else:
						goal = Vector2(battle.lane_x(lane), hold_d)
					_step(goal, delta)
					# parked on the line: run in place while the hall pushes forward, same as the ranks.
					# A playing attack clip owns the sprite, so it never bobs mid-swing.
					if not _moving and _atk_t < 0.0:
						_moving = battle.advancing
	if team == ENEMY:
		wd = clampf(wd - battle.CREEP * delta, battle.ENEMY_MIN_D, battle.SPAWN_D)   # treadmill toward the camera, floored at the hero's feet, capped at the spawn line (blast throws)
	wx = clampf(wx, -battle.HALL_HALF + WALL_INSET, battle.HALL_HALF - WALL_INSET)   # the walls are walls, whatever shoved you
	_tick_attack(delta)
	_place()


func _dist(o: Node3D) -> float:
	# the hero target is a plain Node3D; its world coords live on the battle
	var p := Vector2(o.wx, o.wd) if o is Unit else Vector2(battle.hero_wx, battle.hero_wd)
	return Vector2(wx, wd).distance_to(p)


func _step(goal: Vector2, delta: float) -> void:
	var here := Vector2(wx, wd)
	if here.distance_to(goal) > 2.0:
		var v := (goal - here).normalized()
		var sp := speed * (RUSH if rush else 1.0)
		if here.distance_to(goal) < sp * delta:
			here = goal
		else:
			here += v * sp * delta
		wx = here.x
		wd = here.y
		_moving = true
		# facing: +y in world is away from camera == screen north
		_face(Vector2(v.x, -v.y))


## Rotation frame toward screen-space `v`; a mirrored sprite swaps east/west so flip_h still faces the target.
func _face(v: Vector2) -> void:
	if sprite.hframes != 8:
		return   # a playing clip row owns the frame index; 0..7 would overrun its shorter strip
	var f := Game.facing_from(v)
	sprite.frame = (8 - f) % 8 if _mirror else f


func _place() -> void:
	visible = wd > 40.0
	position = Hall3D.to_world(wx, wd)   # carries the hall bend; wx/wd stay straight
	if _flash_hold > 0.0:
		_flash_hold -= get_process_delta_time()
		sprite.modulate = Color(1.0 + FLASH_PEAK, 1.0 + FLASH_PEAK, 1.0 + FLASH_PEAK)   # hold the peak; lerps resume after
	Hall3D.set_flash(sprite, clampf(sprite.modulate.r - 1.0, 0.0, 1.0))   # take() sets 1+FLASH_PEAK, _process lerps it home
	_lunge = _lunge.lerp(Vector2.ZERO, 0.18)
	_recoil = _recoil.lerp(Vector2.ZERO, 0.2)
	var bob := 0.0
	if _atk_t < 0.0:
		if _moving and not _walk.is_empty():
			Hall3D.clip_frame(sprite, _rot_region, _walk, int(_t * 10.0 * _gait) % int(_walk["frames"]))   # packed walk loop owns the frame
			_walking = true
		elif _walking:
			Hall3D.rotation_frame(sprite, _rot_region, 4 if team == ALLY else 0)
			_walking = false
		elif _moving:
			bob = absf(sin(_t * 12.0 * _gait)) * 3.0   # +y is up in 3D: the step lifts, it doesn't dig
	var lift := air_h
	if _spin != 0.0:
		# spin around the center of mass: origin moves up to it, and the offset re-centres the
		# texture so the mass point (not the cell centre) sits ON the rotation pivot
		lift += _pivot
		sprite.offset = Vector2(0, _cell * 0.5 - _pivot)
	# lunge/recoil were screen nudges toward the target; same magnitude, now on the ground plane
	var nudge := _lunge + _recoil
	sprite.position = Vector3(nudge.x, bob + lift, -nudge.y) * Hall3D.PIXEL


## Sprite height above the feet in world units, the point Hall3D.sprite_top_screen tracks.
## _pivot is folded in so a tumbling sprite's swung-out corners stay inside the frame too.
func top_world() -> float:
	return (_cell + _pivot) * Hall3D.PIXEL


## Highest air_h that keeps the whole sprite inside the frame at this depth.
func _air_cap() -> float:
	return Hall3D.air_cap(battle.camera, wd, top_world()) / Hall3D.PIXEL


## Ground-plane nudge toward a target, in sprite pixels.
func lunge(v: Vector2) -> void:
	_lunge = v


## Halberdier opener: charge to depth `to`, launching every enemy passed, then rush back to `from` (default: here).
func charge(to: float, from := Vector2(INF, INF)) -> void:
	if charge_to > 0.0 or air_h > 0.0:
		return
	charge_to = to
	_charge_back = false
	_charge_dur = -1.0
	_charge_hold_until = -1.0
	_charge_from = Vector2(wx, wd) if from.x == INF else from
	_charge_hit.clear()


## Arrival blow: every enemy this charger carried gets launched into a real arc that lands
## past the pile line, the ones it picked up earliest (farthest from the line) thrown farthest
## and highest so the cluster visibly fans out. Every charger in the same synced charge reaches
## this the same frame, so the whole pile goes up together.
func _deliver_blow() -> void:
	for o in _charge_hit:
		if not is_instance_valid(o) or o.dead or o._charge_thrown:
			continue
		var hit_wd: float = _charge_hit[o]
		var target_wd := minf(charge_to + (charge_to - hit_wd), battle.SPAWN_D)
		var dist: float = target_wd - o.wd
		o._charge_thrown = true
		# scale the impulse with throw distance so the fling is visible, not just a hop; hang time
		# from GRAV's own "peaks at V, hangs ~2V/GRAV" (see the GRAV comment) aims the depth speed
		# at landing on target_wd when it comes down. ponytail: ignores the floaty apex easing
		# (FLOAT_G), so it's an aim not a guarantee — ok for a spectacle throw, not physics sim.
		var v := clampf(220.0 + dist * (100.0 / 600.0), 220.0, 320.0)
		var hang := 2.0 * v / GRAV
		o.launch(v, dist / hang)


## Knock into the air: an impulse, added to whatever it already has. Height builds next frame,
## so the launching hit itself isn't a juggle. `vd` aims a depth throw (the charge blow); other
## callers land wherever the walk/creep drags them, same as before.
func launch(v: float, vd := 0.0) -> void:
	if not dead:
		air_v = minf(air_v + v, 400.0)
		_spin = randf_range(6.0, 11.0) * (1.0 if randf() < 0.5 else -1.0)
		# scatter sideways; the further from centre, the likelier the throw goes back toward it
		var inward := -signf(wx) if wx != 0.0 else (1.0 if randf() < 0.5 else -1.0)
		var dir := inward if randf() < 0.5 + 0.5 * absf(wx) / battle.HALL_HALF else -inward
		air_vx = dir * randf_range(25.0, 70.0)
		air_vd = vd


## Whirl phase: 1s on / 1s off across the spin window; i-frames and the blade only while on.
## spin_phase flips the beat so alternating units cover each other's breathers.
func spinning() -> bool:
	var left := spin_until - Time.get_ticks_msec() / 1000.0
	return left > 0.0 and fmod(left + spin_phase, 2.0) > 1.0


func take(amount: float, push := Vector2.ZERO, quiet := false) -> void:
	if dead:
		return
	if spinning():
		return   # i-frames: the roll dodges it clean
	if air_h > 0.0:
		amount *= JUGGLE_MULT
		_juggles += 1
		air_v = minf(maxf(air_v, 0.0) + POP, 400.0)   # impulse, not a reset: every hit adds
	hp -= amount
	sprite.modulate = Color(1.0 + FLASH_PEAK, 1.0 + FLASH_PEAK, 1.0 + FLASH_PEAK)
	_flash_hold = FLASH_HOLD
	_recoil = push
	if hp <= 0.0:
		Sound.unit(type, "death", wx, team)
		dead = true
	else:
		if not quiet:
			Sound.unit(type, "hit", wx, team)   # gatling passes quiet: 12/s of hit clatter buried its own shot cue
		if not _hurt.is_empty():
			var now := Time.get_ticks_msec() / 1000.0
			_hurt_combo = (_hurt_combo + 1) if now - _last_hurt_t <= 0.8 else 0
			_last_hurt_t = now
			_clip = _hurt[_hurt_combo % _hurt.size()]
			_atk_t = 0.0
		# boss-tier meat sheds while juggled: every airborne hit knocks a couple of chunks out,
		# rate-capped so the gatling's 12/s doesn't turn the boss into a confetti fountain
		if team == ENEMY and max_hp >= BOSS_GIB_HP and (air_h > 0.0 or air_v > 0.0):
			var gt := Time.get_ticks_msec() / 1000.0
			if gt - _last_gib_t > 0.15:
				_last_gib_t = gt
				_gib(push, 2)
	if dead:
		if (air_h > 0.0 or air_v > 0.0) and team == ENEMY:
			Game.score += int(max_hp * (1.0 + 0.5 * _juggles))
		died.emit(self)
		_gib(push)
		# fall: topple away from the blow, sink, fade
		var side := 1.0 if push.x >= 0.0 else -1.0
		var tw := create_tween().set_parallel(true)
		var hang := 0.3
		if air_v > 0.0:
			# killed on the launch frame (cannon blast): _process stops ticking a corpse, so the
			# ride it was owed plays as a tween -- up on the throw, tumbling and drifting, then down
			var peak := (air_h + _pivot + air_v * 0.45) * Hall3D.PIXEL
			sprite.offset = Vector2(0, _cell * 0.5 - _pivot)   # spin about the mass point, as _place does
			var up := create_tween()
			up.tween_property(sprite, "position:y", peak, 0.35).set_trans(Tween.TRANS_QUAD).set_ease(Tween.EASE_OUT)
			up.tween_property(sprite, "position:y", 0.0, 0.5).set_trans(Tween.TRANS_QUAD).set_ease(Tween.EASE_IN)
			tw.tween_property(sprite, "position:x", sprite.position.x + air_vx * 0.85 * Hall3D.PIXEL, 0.85)
			tw.tween_property(sprite, "rotation:z", sprite.rotation.z - side * TAU * 1.5, 0.85)
			hang = 0.85
		elif air_h > 0.0:
			tw.tween_property(sprite, "position:y", 0.0, 0.3).set_trans(Tween.TRANS_QUAD).set_ease(Tween.EASE_IN)
		if air_v > 0.0:
			pass   # the tumble owns the rotation
		elif _death.is_empty():
			tw.tween_property(sprite, "rotation:z", -side * PI / 2.0, 0.35).set_trans(Tween.TRANS_QUAD).set_ease(Tween.EASE_IN)
		else:
			_clip = _death   # packed collapse instead of the topple
			_atk_t = 0.0
		tw.tween_property(sprite, "modulate", Color(0.4, 0.4, 0.4, 0.0), 1.1).set_delay(hang)
		tw.chain().tween_callback(queue_free)


## Gibs: chunks torn from this sprite's own pixels, flung away from the blow, gravity pulls them
## down. They spray in the camera plane, so only the lateral half of `push` steers them.
func _gib(push: Vector2, count := 8) -> void:
	var at: AtlasTexture = sprite.texture
	var cell := at.region.size.y
	var k := Hall3D.PIXEL
	var host := get_parent()
	for i in range(count):
		var c := Sprite3D.new()
		var sub := AtlasTexture.new()
		sub.atlas = at.atlas
		var sz := randf_range(4.0, 8.0)
		var ox := randf_range(cell * 0.3, cell * 0.7 - sz)
		var oy := randf_range(cell * 0.25, cell * 0.9 - sz)
		sub.region = Rect2(at.region.position + Vector2(sprite.frame * cell + ox, oy), Vector2(sz, sz))
		c.texture = sub
		c.pixel_size = k
		c.billboard = BaseMaterial3D.BILLBOARD_DISABLED
		c.texture_filter = BaseMaterial3D.TEXTURE_FILTER_NEAREST
		c.alpha_cut = SpriteBase3D.ALPHA_CUT_DISCARD
		c.alpha_scissor_threshold = 0.35
		c.shaded = false
		var p0 := global_position + Vector3(ox - cell / 2.0, cell - oy, 0.0) * k
		var v := Vector3(randf_range(-70.0, 70.0) + push.x * 0.35, randf_range(60.0, 160.0), 0.0) * k
		var spin := randf_range(-8.0, 8.0)
		host.add_child(c)
		var tw := create_tween()
		tw.tween_method(func(t: float):
			c.position = p0 + v * t - Vector3(0.0, 320.0 * k, 0.0) * t * t
			c.rotation.z = spin * t, 0.0, 1.0, 0.8)
		tw.tween_property(c, "modulate:a", 0.0, 0.4)
		tw.tween_callback(c.queue_free)
