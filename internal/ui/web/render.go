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

// StatusHTML lays the HUD out as coloured spans, one per segment, with the
// exact text and spacing of ui.HUD.Line: HP classed green→amber→red by ratio,
// EP cyan, the location amber, gear labels muted, active effects highlighted.
func StatusHTML(v ui.View) string {
	h := v.HUD
	var b strings.Builder
	fmt.Fprintf(&b, `<span class="%s">HP %d/%d</span>`, hpClass(h.HP, h.HPMax), h.HP, h.HPMax)
	if h.EPMax > 0 {
		fmt.Fprintf(&b, `   <span class="s-ep">EP %d/%d</span>`, h.EP, h.EPMax)
	}
	fmt.Fprintf(&b, `   <span class="s-loc">%s</span>`, html.EscapeString(h.Location))
	fmt.Fprintf(&b, `   <span class="s-label">Wield:</span> %s`, html.EscapeString(h.Wield))
	fmt.Fprintf(&b, `   <span class="s-label">Wear:</span> %s`, html.EscapeString(h.Wear))
	if h.Worn != "" {
		fmt.Fprintf(&b, `   <span class="s-label">Worn:</span> %s`, html.EscapeString(h.Worn))
	}
	if h.Effects != "" {
		fmt.Fprintf(&b, `   <span class="s-eff">[%s]</span>`, html.EscapeString(h.Effects))
	}
	return b.String()
}

// hpClass buckets the HP ratio into the web HUD's traffic-light classes.
func hpClass(hp, max int) string {
	if max <= 0 || float64(hp)/float64(max) > 2.0/3 {
		return "s-hp-good"
	}
	if float64(hp)/float64(max) > 1.0/3 {
		return "s-hp-warn"
	}
	return "s-hp-crit"
}

// badPhrases marks a log line as harm the player suffers (rendered red).
var badPhrases = []string{
	" you for ", // "The X hits/blasts you for N."
	"You die",   // "You die.", "You die...", "...You die."
	"You drown",
	"burned", // "You get burned (by lava)!"
	"You land in lava",
	"vile fumes", // poison gas
	"overcomes you",
	"bites into your hand",
	"wracks you with agony",
	"You are blinded",
	"Darkness falls",
	"stuck in a web",
	"You trigger a trap",
	"You step on", // "...a polymorph trap!", "...an electrified plate!"
	"You fall asleep",
	"You stagger",
	// monsterAttacks' effect-application follow-up, "The X <verb> you." — the
	// player-caused wand variant phrases "...the <monster>.", never "you.".
	"poisons you.",
	"slows you.",
	"confuses you.",
	"frightens you.",
	"afflicts you.", // effectVerb's fallback for unlisted effects
}

// goodPhrases marks a log line as a gain — pickups, heals, victory (rendered
// green). Creature deaths stay neutral: killCreature's "The X dies." names no
// killer, so a hostile felling the player's ally reads the same as a kill.
var goodPhrases = []string{
	"You pick up",
	"recover", // "...and recover N HP."
	"wounds begin",
	"surge with vitality",
	"mind sharpens",
	"fresh charges",
	"You learn",
	"You win",
	"turn and flee",
}

// messageClass buckets a log line by severity for the web message log: "m-bad"
// for harm the player suffers, "m-good" for gains, "" for everything else.
// Keyword-based and presentation-only — a miss just keeps the default colour.
func messageClass(msg string) string {
	for _, p := range badPhrases {
		if strings.Contains(msg, p) {
			return "m-bad"
		}
	}
	for _, p := range goodPhrases {
		if strings.Contains(msg, p) {
			return "m-good"
		}
	}
	return ""
}

// MessagesHTML lays the message log out one line per message, each wrapped in
// its severity class (none for neutral lines), text HTML-escaped.
func MessagesHTML(msgs []string) string {
	var b strings.Builder
	for _, m := range msgs {
		if class := messageClass(m); class != "" {
			fmt.Fprintf(&b, `<span class="%s">%s</span>`, class, html.EscapeString(m))
		} else {
			b.WriteString(html.EscapeString(m))
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
