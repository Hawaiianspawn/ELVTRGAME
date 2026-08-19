class_name Unit
extends Node2D
## One combatant. Static 8-dir art: "animation" is a walk bob and a hit flash.

signal died(unit: Unit)

const ALLY := 0
const ENEMY := 1

var type: String
var team: int
var lane: int
var hp: float
var max_hp: float
var dmg: float
var rng: float
var speed: float
var cooldown: float
var counters: Array
var hold_y: float            # allies stop here; enemies push past it toward the hero
var rooted_until := 0.0
var _cd := 0.0
var _t := 0.0
var _moving := false
var battle: Node2D
var sprite: Sprite2D


func setup(p_type: String, p_team: int, p_lane: int, p_battle: Node2D) -> void:
	type = p_type
	team = p_team
	lane = p_lane
	battle = p_battle
	var d: Dictionary = Game.units[type]
	var hp_mult := 1.0 + (Game.relic_bonus("hp") if team == ALLY else 0.0)
	max_hp = float(d["hp"]) * hp_mult
	hp = max_hp
	dmg = float(d["dmg"])
	rng = float(d["range"])
	speed = float(d["speed"])
	cooldown = float(d["cooldown"])
	counters = d["counters"]
	sprite = Game.make_sprite(d["sprite"], 4 if team == ALLY else 0)
	add_child(sprite)


func _process(delta: float) -> void:
	_t += delta
	_cd -= delta
	if Time.get_ticks_msec() / 1000.0 < rooted_until:
		sprite.modulate = Color(0.6, 1.0, 0.6)
		return
	sprite.modulate = sprite.modulate.lerp(Color.WHITE, delta * 8.0)
	var target: Node2D = battle.find_target(self)
	_moving = false
	if target and position.distance_to(target.position) <= rng:
		if _cd <= 0.0:
			_cd = cooldown
			battle.hit(self, target)
	else:
		var goal := Vector2(battle.lane_x(lane), hold_y)
		if team == ENEMY and battle.lane_open(lane):
			goal = battle.hero.position
		if position.distance_to(goal) > 2.0:
			var v := (goal - position).normalized()
			position += v * speed * delta
			_moving = true
			if team == ENEMY:
				sprite.frame = Game.facing_from(v)
	sprite.position.y = -absf(sin(_t * 12.0)) * 3.0 if _moving else 0.0
	scale = Vector2.ONE * battle.depth_scale(position.y)


func take(amount: float) -> void:
	hp -= amount
	sprite.modulate = Color(2.0, 2.0, 2.0)
	if hp <= 0.0:
		died.emit(self)
		queue_free()
