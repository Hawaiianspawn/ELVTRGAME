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

void vertex() {
	float d = -VERTEX.z;
	VERTEX.x += curve_a * curve_l * (cos(phase / curve_l) - cos((d + phase) / curve_l));
}

void fragment() {
	vec4 c = texture(tex, UV + uv_offset);
	ALBEDO = c.rgb * tint.rgb * COLOR.rgb;
}
