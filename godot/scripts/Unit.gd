class_name Unit
extends Node2D
## One combatant in world space (wx lateral, wd depth). Static 8-dir art: walk bob + hit flash.

signal died(unit: Unit)

const ALLY := 0
const ENEMY := 1
const RUSH := 10.0
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
var _moving := false
var _lunge := Vector2.ZERO
var _recoil := Vector2.ZERO
var dead := false
var opening := false        # fires the swap-in ability when it reaches the line
var guard_until := 0.0
var rush := false            # ADVANCE/RETREAT at rush speed (swaps route behind the camera)
var _leg := 0                # RETREAT: 0 = run back past the camera, 1 = walk into the vacated slot
var battle: Node2D
var sprite: Sprite2D


func setup(p_type: String, p_team: int, p_battle: Node2D) -> void:
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
	sprite = Game.make_sprite(d["sprite"], 4 if team == ALLY else 0)
	add_child(sprite)


func _process(delta: float) -> void:
	if dead:
		return
	_t += delta
	_cd -= delta
	_moving = false
	if Time.get_ticks_msec() / 1000.0 < rooted_until:
		sprite.modulate = Color(0.6, 1.0, 0.6)
	else:
		sprite.modulate = sprite.modulate.lerp(battle.view.fog(wd), delta * 8.0)
		match state:
			State.RANK:
				_moving = battle.advancing
				# live in the ranks: spring back to the home slot, shoved by neighbours and the hero
				var here := Vector2(wx, wd)
				var push: Vector2 = battle.separation(self)
				here += (home - here) * minf(1.0, 3.0 * delta) + push * delta
				wx = here.x
				wd = here.y
				# anything that breaks through gets fought where it stands
				var foe: Unit = battle.near_enemy(self, 90.0)
				if foe:
					sprite.frame = Game.facing_from(Vector2(foe.wx - wx, -(foe.wd - wd)))
					if _dist(foe) <= maxf(rng, 45.0) and _cd <= 0.0:
						_cd = cooldown
						battle.hit(self, foe)
				elif not _moving:
					sprite.frame = 4
			State.RETREAT:
				if _leg == 0:
					_step(Vector2(wx, 45.0), delta)
					if wd < 70.0:
						_leg = 1
						wx = home.x
						wd = 55.0
				else:
					_step(home, delta)
					if not _moving:
						state = State.RANK
						rush = false
						sprite.frame = 4
			State.ADVANCE:
				_step(Vector2(battle.lane_x(lane), hold_d), delta)
				if not _moving:
					state = State.FIGHT
					if opening:
						opening = false
						battle.arrived(self)
			State.FIGHT:
				var target: Node2D = battle.find_target(self)
				if target and _dist(target) <= rng:
					if _cd <= 0.0:
						_cd = cooldown
						battle.hit(self, target)
					if team == ENEMY:
						sprite.frame = Game.facing_from(Vector2(target.wx - wx, -(target.wd - wd)) if target is Unit else Vector2(0, 1))
				else:
					var goal: Vector2
					if team == ENEMY:
						# not on rails: head for whoever is nearest, else the hero; shoulder past your own kind
						goal = Vector2(target.wx, target.wd) if target is Unit else (Vector2(battle.hero_wx, battle.hero_wd) if target == battle.hero else Vector2(wx, hold_d))
						goal += battle.separation(self) * 0.35
					else:
						goal = Vector2(battle.lane_x(lane), hold_d)
					_step(goal, delta)
	if team == ENEMY:
		wd -= battle.CREEP * delta          # treadmill: the army pushes up, so the field slides toward the camera
	_place()


func _dist(o: Node2D) -> float:
	return Vector2(wx, wd).distance_to(Vector2(o.wx, o.wd))


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
		sprite.frame = Game.facing_from(Vector2(v.x, -v.y))


func _place() -> void:
	visible = wd - battle.view.cam_d > 40.0
	position = battle.view.project(wx, wd)
	scale = Vector2.ONE * battle.view.sprite_scale(wd)
	_lunge = _lunge.lerp(Vector2.ZERO, 0.18)
	_recoil = _recoil.lerp(Vector2.ZERO, 0.2)
	var bob := -absf(sin(_t * 12.0 * _gait)) * 3.0 if _moving else 0.0
	sprite.position = _lunge + _recoil + Vector2(0, bob)


func lunge(v: Vector2) -> void:
	_lunge = v / maxf(scale.x, 0.01)


func take(amount: float, push := Vector2.ZERO) -> void:
	if dead:
		return
	hp -= amount
	sprite.modulate = Color(2.0, 2.0, 2.0)
	_recoil = push / maxf(scale.x, 0.01)
	if hp <= 0.0:
		dead = true
		died.emit(self)
		# fall: topple away from the blow, sink, fade
		var side := 1.0 if push.x >= 0.0 else -1.0
		var tw := create_tween().set_parallel(true)
		tw.tween_property(sprite, "rotation", side * PI / 2.0, 0.35).set_trans(Tween.TRANS_QUAD).set_ease(Tween.EASE_IN)
		tw.tween_property(sprite, "modulate", Color(0.4, 0.4, 0.4, 0.0), 1.1).set_delay(0.3)
		tw.chain().tween_callback(queue_free)
