// Package web renders ui.Views as HTML for the browser front-end. This file
// is build-tag-free and pure so the rendering is unit-testable; the js/wasm
// glue lives behind the js && wasm tag.
package web

import (
	"fmt"
	"html"
	"strings"

	"github.com/c0ze/tsl-go/internal/game"
	"github.com/c0ze/tsl-go/internal/ui"
)

// lightBuckets is how many brightness levels the ASCII renderer quantises
// ui.Cell.Light into. Bucket b stands for light b/(lightBuckets-1); its CSS
// opacity mirrors the terminal's ui.Lit ramp (0.30 + 0.70·light — the page
// background is black, so opacity multiplies the glyph colour exactly like
// the terminal's brightness scaling).
const lightBuckets = 5

// lightClass returns the torch-light class (" lt0" darkest … " lt3") for a
// visible cell, or "" at full brightness — the pool of torchlight around the
// player fading toward the edge of sight, matching the terminal renderer.
func lightClass(light float64) string {
	if light < 0 {
		light = 0
	}
	if light > 1 {
		light = 1
	}
	b := int(light*float64(lightBuckets-1) + 0.5)
	if b >= lightBuckets-1 {
		return "" // full torchlight: the base palette colour as-is
	}
	return fmt.Sprintf(" lt%d", b)
}

// RenderHTML lays the View out as the inner HTML of a <pre>: one span per
// run of identically-styled cells (color class per the terminal palette,
// "lt0".."lt3" for the torch-light falloff, "dim" for remembered tiles),
// glyphs HTML-escaped. A non-nil cursor inverts its cell — the targeting
// crosshair — and skips the light class so the crosshair stays full-bright.
func RenderHTML(v ui.View, cursor *game.Pos) string {
	var b strings.Builder
	for y := 0; y < v.H; y++ {
		var runClass string
		open := false
		for x := 0; x < v.W; x++ {
			c := v.At(x, y)
			class := "c-" + string(c.Color)
			isCursor := cursor != nil && cursor.X == x && cursor.Y == y
			switch {
			case c.Dim:
				class += " dim"
			case !isCursor:
				class += lightClass(c.Light)
			}
			if isCursor {
				class += " cursor"
			}
			if !open || class != runClass {
				if open {
					b.WriteString("</span>")
				}
				fmt.Fprintf(&b, `<span class="%s">`, class)
				runClass, open = class, true
			}
			b.WriteString(html.EscapeString(string(c.Glyph)))
		}
		if open {
			b.WriteString("</span>")
		}
		b.WriteString("\n")
	}
	return b.String()
}

// MenuHTML lays a menu out the way the terminal draws it: the title, then
// one "  a) item" line per entry with "> " marking the selection.
func MenuHTML(m ui.MenuSpec, sel int) string {
	var b strings.Builder
	b.WriteString(html.EscapeString(m.Title))
	b.WriteString("\n")
	for i, it := range m.Items {
		prefix := "  "
		if i == sel {
			prefix = "&gt; "
		}
		fmt.Fprintf(&b, "%s%c) %s\n", prefix, 'a'+i, html.EscapeString(it))
	}
	return b.String()
}
