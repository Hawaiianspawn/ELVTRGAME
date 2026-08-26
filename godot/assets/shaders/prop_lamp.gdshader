shader_type spatial;
render_mode unshaded, cull_disabled, depth_draw_opaque, specular_disabled, shadows_disabled;
// Wall lamp billboard: unit_sprite.gdshader plus a top-down fade to black, so the cage and chain
// dissolve into darkness instead of cutting off hard. UV is atlas-space (Sprite3D bakes the frame
// into UV), so the fade fraction comes from VERTEX.y instead: it's local/object-space here (the
// node's own scale/position haven't been applied yet), 0 at the sprite's feet and top_y at its
// top edge — see Hall3D.make_sprite3d (offset = cell/2, feet on origin).
uniform sampler2D texture_albedo : source_color, filter_nearest;
uniform float flash : hint_range(0.0, 1.0) = 0.0;
uniform float scissor = 0.35;
uniform float fade_top : hint_range(0.0, 1.0) = 0.45;
uniform float top_y = 1.0;

varying float vy;

void vertex() {
	vy = VERTEX.y / top_y;
}

void fragment() {
	vec4 tex = texture(texture_albedo, UV);
	ALBEDO = mix(tex.rgb * COLOR.rgb, vec3(1.0), flash) * smoothstep(1.0, 1.0 - fade_top, vy);
	ALPHA = tex.a * COLOR.a;
	ALPHA_SCISSOR_THRESHOLD = scissor;
}
