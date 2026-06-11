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

// RenderHTML lays the View out as the inner HTML of a <pre>: one span per
// run of identically-styled cells (color class per the terminal palette,
// "dim" for remembered tiles), glyphs HTML-escaped. A non-nil cursor inverts
// its cell — the targeting crosshair.
func RenderHTML(v ui.View, cursor *game.Pos) string {
	var b strings.Builder
	for y := 0; y < v.H; y++ {
		var runClass string
		open := false
		for x := 0; x < v.W; x++ {
			c := v.At(x, y)
			class := "c-" + string(c.Color)
			if c.Dim {
				class += " dim"
			}
			if cursor != nil && cursor.X == x && cursor.Y == y {
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
