shader_type spatial;
render_mode unshaded, world_vertex_coords, cull_disabled, specular_disabled;
// Hall floor/walls: nearest-filtered tile times tint times vertex colour, bent OutRun-style.
// The bend is the closed-form integral of curvature curve_a * sin((d + phase) / curve_l) from the
// camera (d = 0) to the vertex depth, so the near end never moves. Same formula as Hall3D.curve_dx.
uniform sampler2D tex : source_color, filter_nearest, repeat_enable;
uniform vec4 tint : source_color = vec4(1.0);
uniform vec2 uv_offset = vec2(0.0);
uniform float curve_a = 0.0;
uniform float curve_l = 460.0;
uniform float phase = 0.0;
// Walls only (floor mats leave this 0): a smaller second wave, opposite sign per side, on its own
// wavelength — the two walls breathe against each other and drift in/out of step with the main
// bend instead of swaying as one rigid tube. Fades in over depth so the near frame never shears.
uniform float ripple_a = 0.0;
const float RIPPLE_L = 230.0;

void vertex() {
	float d = -VERTEX.z;
	float side = sign(VERTEX.x);   // before the bend shift: near the wave-4 amplitudes the shifted x can cross 0
	VERTEX.x += curve_a * curve_l * (cos(phase / curve_l) - cos((d + phase) / curve_l));
	VERTEX.x += side * ripple_a * sin((d + phase * 0.8 + 400.0) / RIPPLE_L) * min(d / 300.0, 1.0);
}

void fragment() {
	vec4 c = texture(tex, UV + uv_offset);
	ALBEDO = c.rgb * tint.rgb * COLOR.rgb;
}
