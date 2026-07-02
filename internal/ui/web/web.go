//go:build js && wasm

package web

import (
	"strings"
	"syscall/js"

	"github.com/c0ze/tsl-go/internal/game"
	"github.com/c0ze/tsl-go/internal/ui"
)

// Screen is the browser front-end: keydown events feed a channel and the
// blocking Prompter calls drain it (Go's wasm scheduler yields to the JS
// event loop while goroutines block), Render writes innerHTML.
type Screen struct {
	keys      chan string
	last      ui.View
	lastLevel string // level id last announced to the JS music controller
	doc       js.Value
	screen    js.Value // <pre id="screen">
	status    js.Value // <div id="status">
	msgs      js.Value // <div id="messages">
	over      js.Value // <pre id="overlay">
}

// New wires the DOM and the key listener.
func New() *Screen {
	doc := js.Global().Get("document")
	sc := &Screen{
		keys:   make(chan string, 16),
		doc:    doc,
		screen: doc.Call("getElementById", "screen"),
		status: doc.Call("getElementById", "status"),
		msgs:   doc.Call("getElementById", "messages"),
		over:   doc.Call("getElementById", "overlay"),
	}
	doc.Call("addEventListener", "keydown", js.FuncOf(func(this js.Value, args []js.Value) any {
		ev := args[0]
		key := ev.Get("key").String()
		if key == " " || len(key) == 1 || key == "Enter" || key == "Escape" ||
			key == "ArrowUp" || key == "ArrowDown" || key == "ArrowLeft" || key == "ArrowRight" {
			ev.Call("preventDefault")
			select {
			case sc.keys <- key:
			default: // drop keys if the player outruns the loop
			}
		}
		return nil
	}))
	return sc
}

// arrows maps browser arrow keys onto the vi runes of the shared binding table.
var arrows = map[string]rune{"ArrowUp": 'k', "ArrowDown": 'j', "ArrowLeft": 'h', "ArrowRight": 'l'}

func (sc *Screen) NextAction() (ui.Action, error) {
	for {
		key := <-sc.keys
		if r, ok := arrows[key]; ok {
			if a, ok := ui.ActionForRune(r); ok {
				return a, nil
			}
		}
		if len(key) == 1 {
			if a, ok := ui.ActionForRune(rune(key[0])); ok {
				return a, nil
			}
		}
	}
}

// Render draws the view: map grid, colour-coded status line, last messages
// (severity-classed).
func (sc *Screen) Render(v ui.View) {
	sc.last = v
	sc.over.Set("hidden", true)
	sc.screen.Set("innerHTML", RenderHTML(v, nil))
	sc.status.Set("innerHTML", StatusHTML(v))
	sc.msgs.Set("innerHTML", MessagesHTML(v.Messages))
	sc.announceLevel(v.LevelID)
	sc.playSounds(v.Sounds)
	sc.sendGrid(v, -1, -1)
}

// sendGrid hands the raw cell grid (glyphs + colour/light/dim) to the JS tile
// renderer (window.tslGrid) so it can draw graphic tiles on a canvas. The ASCII
// <pre> path above is untouched; the front-end shows whichever the user picked.
// cx,cy is the targeting cursor, or -1,-1 when none.
func (sc *Screen) sendGrid(v ui.View, cx, cy int) {
	fn := js.Global().Get("tslGrid")
	if fn.Type() != js.TypeFunction {
		return
	}
	n := v.W * v.H
	color := make([]byte, n)
	bcolor := make([]byte, n)
	light := make([]byte, n)
	dim := make([]byte, n)
	var top, base strings.Builder
	for i := range v.Cells {
		c := v.Cells[i]
		top.WriteRune(c.Glyph)
		color[i] = byte(ui.ColorIndex(c.Color))
		l := c.Light
		if l < 0 {
			l = 0
		} else if l > 1 {
			l = 1
		}
		light[i] = byte(l * 255)
		if c.Dim {
			dim[i] = 1
		}
		b := c // the terrain beneath; falls back to the top cell when no base layer
		if i < len(v.Base) {
			b = v.Base[i]
		}
		base.WriteRune(b.Glyph)
		bcolor[i] = byte(ui.ColorIndex(b.Color))
	}
	fn.Invoke(v.W, v.H, top.String(), toU8(color), base.String(), toU8(bcolor), toU8(light), toU8(dim), cx, cy)
}

// toU8 copies a Go byte slice into a fresh JS Uint8Array.
func toU8(b []byte) js.Value {
	a := js.Global().Get("Uint8Array").New(len(b))
	js.CopyBytesToJS(a, b)
	return a
}

// playSounds fires each queued sound-effect cue at the JS synth
// (window.tslPlaySfx); a no-op if the page hasn't defined it.
func (sc *Screen) playSounds(ids []string) {
	if len(ids) == 0 {
		return
	}
	fn := js.Global().Get("tslPlaySfx")
	if fn.Type() != js.TypeFunction {
		return
	}
	for _, id := range ids {
		fn.Invoke(id)
	}
}

// announceLevel tells the JS music controller (window.tslSetLevel) which level
// is on screen, but only when it changes — so the per-level loop keeps playing
// across ordinary frames and only switches on travel.
func (sc *Screen) announceLevel(id string) {
	if id == sc.lastLevel {
		return
	}
	sc.lastLevel = id
	if fn := js.Global().Get("tslSetLevel"); fn.Type() == js.TypeFunction {
		fn.Invoke(id)
	}
}

// Menu mirrors the terminal: j/k/arrows move, Enter/letter picks, Esc/q cancels.
func (sc *Screen) Menu(m ui.MenuSpec) (int, bool) {
	if len(m.Items) == 0 {
		return 0, false
	}
	sel := 0
	for {
		sc.over.Set("hidden", false)
		sc.over.Set("innerHTML", MenuHTML(m, sel))
		key := <-sc.keys
		switch {
		case key == "ArrowUp" || key == "k":
			sel = (sel - 1 + len(m.Items)) % len(m.Items)
		case key == "ArrowDown" || key == "j":
			sel = (sel + 1) % len(m.Items)
		case key == "Enter":
			sc.over.Set("hidden", true)
			return sel, true
		case key == "Escape" || key == "q":
			sc.over.Set("hidden", true)
			return 0, false
		case len(key) == 1 && key[0] >= 'a' && int(key[0]-'a') < len(m.Items):
			sc.over.Set("hidden", true)
			return int(key[0] - 'a'), true
		}
	}
}

// Target moves a crosshair over the last-rendered map: hjkl/arrows steer,
// Enter confirms, Esc/q cancels — the terminal's exact UX.
func (sc *Screen) Target(origin game.Pos) (game.Pos, bool) {
	v := sc.last
	cur := origin
	clamp := func(p game.Pos) game.Pos {
		if p.X < 0 {
			p.X = 0
		}
		if p.Y < 0 {
			p.Y = 0
		}
		if v.W > 0 && p.X >= v.W {
			p.X = v.W - 1
		}
		if v.H > 0 && p.Y >= v.H {
			p.Y = v.H - 1
		}
		return p
	}
	dirs := map[string][2]int{
		"h": {-1, 0}, "l": {1, 0}, "k": {0, -1}, "j": {0, 1},
		"y": {-1, -1}, "u": {1, -1}, "b": {-1, 1}, "n": {1, 1},
		"ArrowLeft": {-1, 0}, "ArrowRight": {1, 0}, "ArrowUp": {0, -1}, "ArrowDown": {0, 1},
	}
	for {
		sc.screen.Set("innerHTML", RenderHTML(v, &cur))
		sc.sendGrid(v, cur.X, cur.Y)
		key := <-sc.keys
		if d, ok := dirs[key]; ok {
			cur = clamp(game.Pos{X: cur.X + d[0], Y: cur.Y + d[1]})
			continue
		}
		switch key {
		case "Enter":
			sc.screen.Set("innerHTML", RenderHTML(v, nil))
			return cur, true
		case "Escape", "q":
			sc.screen.Set("innerHTML", RenderHTML(v, nil))
			return game.Pos{}, false
		}
	}
}

// Overlay shows a terminal-style full message (save confirmation, the morgue).
func (sc *Screen) Overlay(text string) {
	sc.over.Set("hidden", false)
	sc.over.Set("textContent", text)
}
