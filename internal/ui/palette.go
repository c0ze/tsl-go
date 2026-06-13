package ui

import "github.com/c0ze/tsl-go/internal/content"

// RGB is a 24-bit colour the front-ends render from.
type RGB struct{ R, G, B uint8 }

// Palette maps each named content colour to a 24-bit hue. It is the single
// source of truth the terminal front-end renders from; the web front-end
// mirrors these values in its CSS (web/index.html) — keep the two in sync.
var Palette = map[content.Color]RGB{
	content.ColorNormal:  {0xc8, 0xbe, 0xa5}, // warm stone
	content.ColorBrown:   {0xb0, 0x7a, 0x3a},
	content.ColorBlue:    {0x4a, 0x90, 0xd9},
	content.ColorRed:     {0xd6, 0x50, 0x4a},
	content.ColorGreen:   {0x6f, 0xae, 0x4a},
	content.ColorCyan:    {0x3f, 0xb6, 0xc4},
	content.ColorMagenta: {0xc0, 0x60, 0xc0},
	content.ColorBlack:   {0x28, 0x2c, 0x30},
}

// ColorRGB returns the base hue for c, defaulting to the normal colour for any
// value not in the palette.
func ColorRGB(c content.Color) RGB {
	if v, ok := Palette[c]; ok {
		return v
	}
	return Palette[content.ColorNormal]
}

// Lit scales a base hue by a tile's light level (0..1), with a faint warm cast
// where the light is strongest — the glow of a carried torch. A fully-lit tile
// (light 1) keeps its base hue; an unlit one floors at ~30% so a visible tile
// never falls to pure black.
func Lit(base RGB, light float64) RGB {
	if light < 0 {
		light = 0
	}
	if light > 1 {
		light = 1
	}
	br := 0.30 + 0.70*light
	warm := 0.12 * light
	return RGB{
		R: scale(base.R, br*(1+warm)),
		G: scale(base.G, br),
		B: scale(base.B, br*(1-warm)),
	}
}

// Remembered renders a tile the player has seen but cannot currently see: dark
// and cool — the colour of memory rather than torchlight.
func Remembered(base RGB) RGB {
	return RGB{
		R: scale(base.R, 0.28),
		G: scale(base.G, 0.31),
		B: lift(scale(base.B, 0.38), 16),
	}
}

func scale(v uint8, f float64) uint8 {
	r := float64(v) * f
	if r < 0 {
		r = 0
	}
	if r > 255 {
		r = 255
	}
	return uint8(r)
}

func lift(v uint8, n int) uint8 {
	r := int(v) + n
	if r > 255 {
		r = 255
	}
	if r < 0 {
		r = 0
	}
	return uint8(r)
}
