shader_type spatial;
render_mode unshaded, cull_disabled, depth_draw_opaque, specular_disabled, shadows_disabled;
// Unit billboard: atlas region (Sprite3D bakes the frame into UV) x modulate, with a hit flash
// that mixes the whole silhouette to white. Sprite3D clamps modulate to 0..1 in the vertex
// colour, so the over-1 pulse Unit.take() sets cannot be read here — Unit._place pushes it as
// `flash`. A material_override gets no texture from Sprite3D, so the atlas is a uniform.
uniform sampler2D texture_albedo : source_color, filter_nearest;
uniform float flash : hint_range(0.0, 1.0) = 0.0;
uniform float scissor = 0.35;
// Persistent wounds: circles in the sprite's LOCAL quad space (stable across animation
// frames — UV is atlas-space and would make a hole jump on every frame change).
// xy = centre in local units, z = radius; z = 0 means unused. Unit.take() fills these
// as wear chunks tear off, so the flying slab and the missing piece are the same piece.
uniform vec3 hole[4];

varying vec2 body;

void vertex() {
	body = VERTEX.xy;
}

void fragment() {
	vec4 tex = texture(texture_albedo, UV);
	float keep = 1.0;
	for (int i = 0; i < 4; i++) {
		if (hole[i].z > 0.0 && distance(body, hole[i].xy) < hole[i].z) {
			keep = 0.0;
		}
	}
	ALBEDO = mix(tex.rgb * COLOR.rgb, vec3(1.0), flash);
	ALPHA = tex.a * COLOR.a * keep;
	ALPHA_SCISSOR_THRESHOLD = scissor;
}
