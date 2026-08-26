shader_type canvas_item;
// Post-process preset stack, one screen-space pass. `preset` picks the look; 0 = off
// (Post.gd hides the ColorRect entirely at 0 so this never runs then).
uniform int preset : hint_range(0, 5) = 0;
uniform float steps : hint_range(2.0, 8.0) = 6.0;   // preset 1: banding levels per channel
uniform sampler2D screen_tex : hint_screen_texture, filter_nearest, repeat_disable;

// GL Compatibility canvas_item shaders don't expose SCREEN_PIXEL_SIZE to helper functions;
// viewport is fixed at 960x540 (stretch mode "viewport" in project.godot), so hardcode it.
const vec2 PX = vec2(1.0 / 960.0, 1.0 / 540.0);

const float BAYER4[16] = float[16](
	0.0, 8.0, 2.0, 10.0,
	12.0, 4.0, 14.0, 6.0,
	3.0, 11.0, 1.0, 9.0,
	15.0, 7.0, 13.0, 5.0
);

float luma(vec3 c) { return dot(c, vec3(0.299, 0.587, 0.114)); }

// preset 1: banded values + 4x4 ordered Bayer dither so the band edges don't stair-step.
vec3 p_dither(vec2 uv, vec3 col) {
	ivec2 p = ivec2(mod(uv / PX, 4.0));
	float threshold = (BAYER4[p.y * 4 + p.x] + 0.5) / 16.0;
	vec3 scaled = col * steps;
	vec3 base = floor(scaled);
	vec3 frac = scaled - base;
	return (base + step(threshold, frac)) / steps;
}

// preset 2: bright-pass glow (12-tap ring blur of over-threshold pixels, added back) + vignette.
vec3 p_glow(vec2 uv, vec3 col) {
	vec3 bloom = vec3(0.0);
	float threshold = 0.65;
	for (int i = 0; i < 12; i++) {
		float a = float(i) * 0.5236; // 2*PI/12
		vec2 off = vec2(cos(a), sin(a)) * PX * 3.0;
		vec3 s = texture(screen_tex, uv + off).rgb;
		bloom += max(s - threshold, 0.0);
	}
	bloom /= 12.0;
	vec3 lit = col + bloom * 1.4;
	vec2 centered = uv - 0.5;
	float vig = clamp(1.0 - dot(centered, centered) * 1.1, 0.35, 1.0);
	return lit * vig;
}

// preset 3: cool-green shadows (necromancer glow), warm lift near frame centre (the hero).
vec3 p_grade(vec2 uv, vec3 col) {
	float l = luma(col);
	vec3 cool = vec3(0.75, 1.0, 0.85);
	vec3 warm = vec3(1.12, 1.0, 0.85);
	vec3 shadow_tint = mix(cool, vec3(1.0), smoothstep(0.0, 0.5, l));
	float center_w = 1.0 - clamp(length(uv - 0.5) * 1.6, 0.0, 1.0);
	return mix(col * shadow_tint, col * warm, center_w * 0.5);
}

// preset 4: Sobel luminance edge-detect, darkens edges as an ink outline readability test.
vec3 p_edges(vec2 uv, vec3 col) {
	vec2 px = PX;
	float tl = luma(texture(screen_tex, uv + vec2(-px.x, -px.y)).rgb);
	float t  = luma(texture(screen_tex, uv + vec2(0.0, -px.y)).rgb);
	float tr = luma(texture(screen_tex, uv + vec2(px.x, -px.y)).rgb);
	float l  = luma(texture(screen_tex, uv + vec2(-px.x, 0.0)).rgb);
	float r  = luma(texture(screen_tex, uv + vec2(px.x, 0.0)).rgb);
	float bl = luma(texture(screen_tex, uv + vec2(-px.x, px.y)).rgb);
	float b  = luma(texture(screen_tex, uv + vec2(0.0, px.y)).rgb);
	float br = luma(texture(screen_tex, uv + vec2(px.x, px.y)).rgb);
	float gx = -tl - 2.0 * l - bl + tr + 2.0 * r + br;
	float gy = -tl - 2.0 * t - tr + bl + 2.0 * b + br;
	float edge = clamp(length(vec2(gx, gy)), 0.0, 1.0);
	return col * (1.0 - edge * 0.85);
}

// preset 5: CRT scanlines + mild barrel curvature.
vec3 p_crt(vec2 uv) {
	vec2 c = uv * 2.0 - 1.0;
	c *= 1.0 + 0.06 * dot(c, c);
	vec2 buv = (c + 1.0) * 0.5;
	vec3 col = texture(screen_tex, clamp(buv, 0.0, 1.0)).rgb;
	col *= 0.85 + 0.15 * sin(buv.y / PX.y * 3.14159);
	float outside = max(abs(c.x), abs(c.y));
	col *= 1.0 - smoothstep(0.96, 1.0, outside);
	return col;
}

void fragment() {
	vec2 uv = SCREEN_UV;
	vec3 col;
	if (preset == 5) {
		col = p_crt(uv);
	} else {
		col = texture(screen_tex, uv).rgb;
		if (preset == 1) col = p_dither(uv, col);
		else if (preset == 2) col = p_glow(uv, col);
		else if (preset == 3) col = p_grade(uv, col);
		else if (preset == 4) col = p_edges(uv, col);
	}
	COLOR = vec4(col, 1.0);
}
