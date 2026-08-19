class_name Unit
extends Node2D
## One combatant in world space (wx lateral, wd depth). Static 8-dir art: walk bob + hit flash.

signal died(unit: Unit)

const ALLY := 0
const ENEMY := 1
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
var _t := 0.0
var _moving := false
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
	speed = float(d["speed"]) * 1.5
	cooldown = float(d["cooldown"])
	counters = d["counters"]
	sprite = Game.make_sprite(d["sprite"], 4 if team == ALLY else 0)
	add_child(sprite)


func _process(delta: float) -> void:
	_t += delta
	_cd -= delta
	_moving = false
	if Time.get_ticks_msec() / 1000.0 < rooted_until:
		sprite.modulate = Color(0.6, 1.0, 0.6)
	else:
		sprite.modulate = sprite.modulate.lerp(battle.view.fog(wd), delta * 8.0)
		match state:
			State.RANK:
				_moving = battle.advancing      # the whole company marches; ranks bob in step
			State.RETREAT:
				_step(home, delta)
				if not _moving:
					state = State.RANK
					sprite.frame = 4
			State.ADVANCE:
				_step(Vector2(battle.lane_x(lane), hold_d), delta)
				if not _moving:
					state = State.FIGHT
			State.FIGHT:
				var target: Node2D = battle.find_target(self)
				if target and _dist(target) <= rng:
					if _cd <= 0.0:
						_cd = cooldown
						battle.hit(self, target)
				else:
					var goal := Vector2(battle.lane_x(lane), hold_d)
					if team == ENEMY and battle.lane_open(lane):
						goal = Vector2(battle.hero_wx, battle.hero_wd)
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
		here += v * speed * delta
		wx = here.x
		wd = here.y
		_moving = true
		# facing: +y in world is away from camera == screen north
		sprite.frame = Game.facing_from(Vector2(v.x, -v.y))


func _place() -> void:
	position = battle.view.project(wx, wd)
	scale = Vector2.ONE * battle.view.sprite_scale(wd)
	sprite.position.y = -absf(sin(_t * 12.0)) * 3.0 if _moving else 0.0


func take(amount: float) -> void:
	hp -= amount
	sprite.modulate = Color(2.0, 2.0, 2.0)
	if hp <= 0.0:
		died.emit(self)
		queue_free()
