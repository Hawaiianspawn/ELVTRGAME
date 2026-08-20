extends Node3D
## Spike: pixel sprites as Sprite3D billboards in a real 3D world. Same atlas, same (wx, wd) sim coords.
## Proves the look on gl_compatibility (WebGL2) before the battle scenes move off View.gd.

const SCALE := 0.02            # world units per sprite pixel (88px cell = 1.76 m tall)

var cam: Camera3D
var sprites3d: Array[Sprite3D] = []
var wander: Array[Vector2] = []   # per-unit (wx, wd) velocity, just to exercise facing


func _ready() -> void:
	var env := WorldEnvironment.new()
	var e := Environment.new()
	e.background_mode = Environment.BG_COLOR
	e.background_color = Color("#3a3b3d")
	e.fog_enabled = true
	e.fog_light_color = Color.BLACK
	e.fog_density = 0.035
	e.ambient_light_source = Environment.AMBIENT_SOURCE_COLOR
	e.ambient_light_color = Color.WHITE
	e.ambient_light_energy = 1.0
	env.environment = e
	add_child(env)

	var ground := MeshInstance3D.new()
	var pm := PlaneMesh.new()
	pm.size = Vector2(200, 200)
	ground.mesh = pm
	var gm := StandardMaterial3D.new()
	gm.albedo_color = Color("#2a2a2b")
	gm.shading_mode = BaseMaterial3D.SHADING_MODE_UNSHADED
	ground.material_override = gm
	ground.position.z = -60
	add_child(ground)
	# rank lines on the ground, same read as View.draw_ground
	for i in range(30):
		var line := MeshInstance3D.new()
		var bm := BoxMesh.new()
		bm.size = Vector3(200, 0.01, 0.04)
		line.mesh = bm
		var lm := StandardMaterial3D.new()
		lm.albedo_color = Color(0, 0, 0, 0.25)
		lm.transparency = BaseMaterial3D.TRANSPARENCY_ALPHA
		lm.shading_mode = BaseMaterial3D.SHADING_MODE_UNSHADED
		line.material_override = lm
		line.position = Vector3(0, 0.005, -i * 2.0)
		add_child(line)

	cam = Camera3D.new()
	cam.position = Vector3(0, 3.0, 4.0)
	cam.rotation_degrees.x = -18
	cam.fov = 55
	add_child(cam)

	var rng := RandomNumberGenerator.new()
	rng.seed = 7
	var names := ["veteran", "halberdier", "hammer", "vet_ranged", "mage"]
	for row in range(12):
		for col in range(-8, 9):
			var name: String = names[(row + col) % names.size() if (row + col) >= 0 else 0]
			var s := make_sprite3d(name)
			s.position = Vector3(col * 1.1 + rng.randf_range(-0.2, 0.2), 0, -2.0 - row * 1.6 + rng.randf_range(-0.3, 0.3))
			add_child(s)
			sprites3d.append(s)
			wander.append(Vector2(rng.randf_range(-0.3, 0.3), rng.randf_range(-0.3, 0.3)))


## Sprite3D for a packed 8-direction strip. Billboards on Y only so feet stay on the ground.
func make_sprite3d(name: String) -> Sprite3D:
	var s := Sprite3D.new()
	var at := AtlasTexture.new()
	at.atlas = load("res://assets/sprites/atlas.png")
	var cell: int = int(Game.sprites[name]["cell"])
	at.region = Rect2(0, int(Game.sprites[name]["y"]), cell * 8, cell)
	s.texture = at
	s.hframes = 8
	s.pixel_size = SCALE
	s.billboard = BaseMaterial3D.BILLBOARD_FIXED_Y
	s.texture_filter = BaseMaterial3D.TEXTURE_FILTER_NEAREST
	s.alpha_cut = SpriteBase3D.ALPHA_CUT_DISCARD
	s.shaded = false
	s.centered = true
	s.offset = Vector2(0, cell / 2.0)   # feet on origin
	return s


func _process(delta: float) -> void:
	var t := Time.get_ticks_msec() / 1000.0
	for i in sprites3d.size():
		var s := sprites3d[i]
		var v := wander[i]
		s.position.x += v.x * delta
		s.position.z += v.y * delta
		# bob like Unit.gd
		s.position.y = absf(sin(t * 6.0 + i)) * 0.03
		# 8-dir frame from the unit's facing relative to the camera's view direction
		var facing := Vector3(v.x, 0, v.y).normalized()
		var to_cam := (cam.global_position - s.global_position)
		to_cam.y = 0
		to_cam = to_cam.normalized()
		# angle of facing relative to "toward camera" (= south frame), clockwise
		var ang := rad_to_deg(atan2(facing.cross(to_cam).y, facing.dot(to_cam)))
		s.frame = Game.facing_from(Vector2(sin(deg_to_rad(ang)), cos(deg_to_rad(ang))))
