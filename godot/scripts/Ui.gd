class_name Ui
## Builders over the PixelLab UI kit in assets/ui (pieces landed by Scripts/art/ui_land.py).
## Everything is 1:1 canvas pixels on the 960x540 viewport; pixel fonts only at 16 / 32.

const DIR := "res://assets/ui/"
const COL_TEXT := Color("#ddd3b6")     # parchment bright
const COL_DIM := Color("#a0a08b")
const COL_EMBER := Color("#e8a23c")
const COL_HOT := Color("#ffcf6b")
const COL_GREEN := Color("#6fd46a")


static func tex(name: String) -> Texture2D:
	return load(DIR + name + ".png")


## Pixel font resource with AA / hinting / subpixel off so glyphs stay on the grid.
static func font(name: String) -> FontFile:
	var f: FontFile = load(DIR + "fonts/" + name + ".ttf")
	f.antialiasing = TextServer.FONT_ANTIALIASING_NONE
	f.hinting = TextServer.HINTING_NONE
	f.subpixel_positioning = TextServer.SUBPIXEL_POSITIONING_DISABLED
	f.generate_mipmaps = false
	return f


static func nine(name: String, margin: int, tile_center := false) -> StyleBoxTexture:
	var sb := StyleBoxTexture.new()
	sb.texture = tex(name)
	sb.set_texture_margin_all(margin)
	if tile_center:
		sb.axis_stretch_horizontal = StyleBoxTexture.AXIS_STRETCH_MODE_TILE
	return sb


static func sprite(parent: Node, name: String, pos: Vector2) -> TextureRect:
	var r := TextureRect.new()
	r.texture = tex(name)
	r.position = pos
	r.mouse_filter = Control.MOUSE_FILTER_IGNORE
	parent.add_child(r)
	return r


## Nine-patch node; `m` is [left, top, right, bottom] in source pixels.
static func patch(parent: Node, name: String, pos: Vector2, size: Vector2, m: Array) -> NinePatchRect:
	var n := NinePatchRect.new()
	n.texture = tex(name)
	n.position = pos
	n.size = size
	n.patch_margin_left = m[0]
	n.patch_margin_top = m[1]
	n.patch_margin_right = m[2]
	n.patch_margin_bottom = m[3]
	n.mouse_filter = Control.MOUSE_FILTER_IGNORE
	parent.add_child(n)
	return n


static func label(parent: Node, text: String, pos: Vector2, col := COL_TEXT, size := 16, width := 0.0) -> Label:
	var l := Label.new()
	l.text = text
	l.position = pos
	l.add_theme_font_size_override("font_size", size)
	l.add_theme_color_override("font_color", col)
	l.mouse_filter = Control.MOUSE_FILTER_IGNORE
	if width > 0.0:
		l.size.x = width
		l.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	parent.add_child(l)
	return l


## A kit slot (chip / socket) with a centred 32px icon. Returns {frame, icon}.
static func slot(parent: Node, frame_name: String, icon_name: String, pos: Vector2) -> Dictionary:
	var f := sprite(parent, frame_name, pos)
	var inset := (f.texture.get_width() - 32) / 2.0
	var i := sprite(f, "icons/" + icon_name, Vector2(inset, inset))
	return {"frame": f, "icon": i}


## Kit medallion with the hero's 64px bust centred in the ring. Returns the medallion so the bust
## can be swapped (child 0) when the hero changes.
static func portrait(parent: Node, hero: String, pos: Vector2) -> TextureRect:
	var m := sprite(parent, "medallion", pos)
	sprite(m, "portraits/" + hero, Vector2(17, 17))
	return m


## Horizontal bar: kit housing + a fill ColorRect inside the trough. Returns {frame, fill, label}.
static func bar(parent: Node, pos: Vector2, col: Color, text: String) -> Dictionary:
	var f := sprite(parent, "bar", pos)
	var fill := ColorRect.new()
	fill.color = col
	fill.position = Vector2(10, 9)
	fill.size = Vector2(261, 11)
	fill.mouse_filter = Control.MOUSE_FILTER_IGNORE
	f.add_child(fill)
	var l := label(f, text, Vector2(0, 6), COL_TEXT, 16, 281.0)
	return {"frame": f, "fill": fill, "label": l}


static func set_bar(b: Dictionary, frac: float, text: String) -> void:
	b["fill"].size.x = 261.0 * clampf(frac, 0.0, 1.0)
	b["label"].text = text
