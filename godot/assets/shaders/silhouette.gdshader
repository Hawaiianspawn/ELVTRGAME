shader_type canvas_item;
// Flat cutout: sprite alpha, one colour. Used for the far horde band.
uniform vec4 tint : source_color = vec4(0.40, 0.41, 0.41, 1.0);
void fragment() {
	COLOR = vec4(tint.rgb, texture(TEXTURE, UV).a * COLOR.a);
}
