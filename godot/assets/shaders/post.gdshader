shader_type canvas_item;
// Post-process preset stack, one screen-space pass. `preset` picks the look; 0 = off
// (Post.gd hides the ColorRect entirely at 0 so this never runs then).
uniform int preset : hint_range(0, 6) = 0;
uniform sampler2D screen_tex : hint_screen_texture, filter_nearest, repeat_disable;

// GL Compatibility canvas_item shaders don't expose SCREEN_PIXEL_SIZE to helper functions;
// viewport is fixed at 960x540 (stretch mode "viewport" in project.godot), so hardcode it.
const vec2 PX = vec2(1.0 / 960.0, 1.0 / 540.0);

const float BAYER8[64] = float[64](
	 0.0, 32.0,  8.0, 40.0,  2.0, 34.0, 10.0, 42.0,
	48.0, 16.0, 56.0, 24.0, 50.0, 18.0, 58.0, 26.0,
	12.0, 44.0,  4.0, 36.0, 14.0, 46.0,  6.0, 38.0,
	60.0, 28.0, 52.0, 20.0, 62.0, 30.0, 54.0, 22.0,
	 3.0, 35.0, 11.0, 43.0,  1.0, 33.0,  9.0, 41.0,
	51.0, 19.0, 59.0, 27.0, 49.0, 17.0, 57.0, 25.0,
	15.0, 47.0,  7.0, 39.0, 13.0, 45.0,  5.0, 37.0,
	63.0, 31.0, 55.0, 23.0, 61.0, 29.0, 53.0, 21.0
);

// demichrome-4 ramp (docs/data/art/palette.json): dark, steel, bone, pale + their
// Rec.709 gamma-space lumas, the quantize model that file locks in.
const vec3 DEMI[4] = vec3[4](
	vec3(0.129, 0.118, 0.125), vec3(0.333, 0.333, 0.408),
	vec3(0.627, 0.627, 0.545), vec3(0.914, 0.937, 0.925)
);
const float DEMI_L[4] = float[4](0.1207, 0.3387, 0.6216, 0.9314);

float luma(vec3 c) { return dot(c, vec3(0.299, 0.587, 0.114)); }

// preset 1: 8x8 ordered dither snapped to the demichrome-4 ramp by Rec.709 luma --
// the 2-bit look the game shipped under before the full-colour amendment.
vec3 p_dither(vec2 uv, vec3 col) {
	float l = dot(col, vec3(0.2126, 0.7152, 0.0722));
	ivec2 p = ivec2(mod(uv / PX, 8.0));
	float t = (BAYER8[p.y * 8 + p.x] + 0.5) / 64.0;
	int i = 0;
	if (l > DEMI_L[1]) i = 1;
	if (l > DEMI_L[2]) i = 2;
	float f = clamp((l - DEMI_L[i]) / (DEMI_L[i + 1] - DEMI_L[i]), 0.0, 1.0);
	return DEMI[i + int(step(t, f))];
}

// dusty4 ramp (lospec.com/palette-list/dusty4, by ink): night purple, slate blue,
// sage green, bone white + Rec.709 gamma-space lumas, same quantize model as demichrome.
// Dark step deepened from the published #372a51 to #241b36 (owner: darker floor).
const vec3 DUSTY[4] = vec3[4](
	vec3(0.141, 0.106, 0.212), vec3(0.227, 0.314, 0.408),
	vec3(0.353, 0.561, 0.471), vec3(0.961, 0.965, 0.875)
);
const float DUSTY_L[4] = float[4](0.1211, 0.3024, 0.5103, 0.9577);

// preset 6: the dither preset on the dusty4 ramp instead of demichrome-4.
vec3 p_dusty(vec2 uv, vec3 col) {
	float l = dot(col, vec3(0.2126, 0.7152, 0.0722));
	ivec2 p = ivec2(mod(uv / PX, 8.0));
	float t = (BAYER8[p.y * 8 + p.x] + 0.5) / 64.0;
	int i = 0;
	if (l > DUSTY_L[1]) i = 1;
	if (l > DUSTY_L[2]) i = 2;
	float f = clamp((l - DUSTY_L[i]) / (DUSTY_L[i + 1] - DUSTY_L[i]), 0.0, 1.0);
	return DUSTY[i + int(step(t, f))];
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
	vec3 cool = vec3(0.66, 1.05, 0.74);   // owner 2026-08-31: greener (was 0.75, 1.0, 0.85)
	vec3 warm = vec3(1.12, 1.0, 0.85);
	vec3 shadow_tint = mix(cool, vec3(1.0), smoothstep(0.0, 0.6, l));
	float center_w = 1.0 - clamp(length(uv - 0.5) * 1.6, 0.0, 1.0);
	return mix(col * shadow_tint, col * warm, center_w * 0.5);
}

// (the Sobel "edges" preset was cut 2026-08-31 — owner call, it webbed the packed crowd)

// preset 4: CRT scanlines + mild barrel curvature + aperture-grille phosphor mask.
vec3 p_crt(vec2 uv) {
	vec2 c = uv * 2.0 - 1.0;
	c *= 1.0 + 0.06 * dot(c, c);
	vec2 buv = (c + 1.0) * 0.5;
	vec3 col = texture(screen_tex, clamp(buv, 0.0, 1.0)).rgb;
	col *= 0.85 + 0.15 * sin(buv.y / PX.y * 3.14159);
	// RGB triad columns; 1.25 gain pays back the mask's brightness loss
	float m = mod(floor(buv.x / PX.x), 3.0);
	vec3 mask = m < 1.0 ? vec3(1.0, 0.75, 0.75) : (m < 2.0 ? vec3(0.75, 1.0, 0.75) : vec3(0.75, 0.75, 1.0));
	col *= mask * 1.25;
	float outside = max(abs(c.x), abs(c.y));
	col *= 1.0 - smoothstep(0.96, 1.0, outside);
	return col;
}

// preset 5: the whole cabinet -- barrel curvature, chromatic aberration at the edges,
// demichrome-4 dither per channel (offsets re-fringe the quantized greys), scanlines,
// phosphor mask. One sample chain, still single-pass.
vec3 p_retro(vec2 uv) {
	vec2 c = uv * 2.0 - 1.0;
	c *= 1.0 + 0.06 * dot(c, c);
	vec2 buv = (c + 1.0) * 0.5;
	vec2 ab = c * 1.5 * PX;   // aberration offset grows toward the frame edge
	vec3 col = vec3(
		p_dither(buv, texture(screen_tex, clamp(buv + ab, vec2(0.0), vec2(1.0))).rgb).r,
		p_dither(buv, texture(screen_tex, clamp(buv, vec2(0.0), vec2(1.0))).rgb).g,
		p_dither(buv, texture(screen_tex, clamp(buv - ab, vec2(0.0), vec2(1.0))).rgb).b);
	col *= 0.85 + 0.15 * sin(buv.y / PX.y * 3.14159);
	float m = mod(floor(buv.x / PX.x), 3.0);
	vec3 mask = m < 1.0 ? vec3(1.0, 0.75, 0.75) : (m < 2.0 ? vec3(0.75, 1.0, 0.75) : vec3(0.75, 0.75, 1.0));
	col *= mask * 1.25;
	float outside = max(abs(c.x), abs(c.y));
	col *= 1.0 - smoothstep(0.96, 1.0, outside);
	return col;
}

void fragment() {
	vec2 uv = SCREEN_UV;
	vec3 col;
	if (preset == 4) {
		col = p_crt(uv);
	} else if (preset == 5) {
		col = p_retro(uv);
	} else {
		col = texture(screen_tex, uv).rgb;
		if (preset == 1) col = p_dither(uv, col);
		else if (preset == 2) col = p_glow(uv, col);
		else if (preset == 3) col = p_grade(uv, col);
		else if (preset == 6) col = p_dusty(uv, col);
	}
	COLOR = vec4(col, 1.0);
}
