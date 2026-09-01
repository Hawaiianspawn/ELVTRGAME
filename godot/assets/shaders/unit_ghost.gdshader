shader_type spatial;
render_mode unshaded, cull_disabled, depth_draw_opaque, specular_disabled, shadows_disabled;
// unit_sprite.gdshader's blended twin for spectral units (units.json "alpha" < 1): same flash
// and torn-hole wounds, but the body renders semi-transparent. The hard silhouette edge is kept
// by discarding thin texture alpha; only the modulate alpha blends, so the sprite stays crisp.
// Blended pass writes no depth, so a ghost sorts against the opaque crowd per pixel and against
// other ghosts by centre distance — fine for the handful of wraiths on screen.
uniform sampler2D texture_albedo : source_color, filter_nearest;
uniform float flash : hint_range(0.0, 1.0) = 0.0;
uniform float scissor = 0.35;
uniform vec3 hole[4];
uniform float px = 2.2;

varying vec2 body;

void vertex() {
	body = VERTEX.xy;
}

void fragment() {
	vec4 tex = texture(texture_albedo, UV);
	float keep = 1.0;
	vec2 blk = floor(body / px);
	vec2 bq = (blk + 0.5) * px;
	float jag = fract(sin(dot(blk, vec2(12.9898, 78.233))) * 43758.5453);
	for (int i = 0; i < 4; i++) {
		if (hole[i].z > 0.0 && distance(bq, hole[i].xy) < hole[i].z * (0.7 + 0.6 * jag)) {
			keep = 0.0;
		}
	}
	if (tex.a * keep < scissor) {
		discard;
	}
	ALBEDO = mix(tex.rgb * COLOR.rgb, vec3(1.0), flash);
	ALPHA = COLOR.a;
}
