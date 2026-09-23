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
	final     bool     // an end screen is up: Enter reloads the page
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
		altGr := ev.Call("getModifierState", "AltGraph").Bool() // reports as Ctrl+Alt on Windows
		if !altGr && (ev.Get("ctrlKey").Bool() || ev.Get("metaKey").Bool() || ev.Get("altKey").Bool()) {
			return nil // leave browser shortcuts (Ctrl/Cmd+R, …) to the browser
		}
		key := ev.Get("key").String()
		if sc.final {
			// A fresh press only: the auto-repeat of an Enter still held from
			// the confirming menu pick must not skip past the death screen.
			if key == "Enter" && !ev.Get("repeat").Bool() {
				ev.Call("preventDefault")
				js.Global().Get("location").Call("reload")
			}
			return nil
		}
		if _, arrow := arrows[key]; arrow && sc.over.Get("hidden").Bool() {
			if t := ev.Get("target"); t.Truthy() && t.Get("tagName").String() == "INPUT" {
				return nil // the focused volume slider keeps its arrows — except in a menu
			}
		}
		if len(key) == 1 || key == "Enter" || key == "Escape" ||
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

// Menu mirrors the terminal (ui.MenuKey: letters pick, j/k/arrows move,
// Enter picks, Esc/q cancels).
func (sc *Screen) Menu(m ui.MenuSpec) (int, bool) {
	if len(m.Items) == 0 {
		return 0, false
	}
	sel := 0
	for {
		sc.over.Set("hidden", false)
		sc.over.Set("innerHTML", MenuHTML(m, sel))
		var res ui.PromptResult
		if sel, res = ui.MenuKey(<-sc.keys, sel, m.Items); res != ui.PromptContinue {
			sc.over.Set("hidden", true)
			return sel, res == ui.PromptPick
		}
	}
}

// Target moves a crosshair over the last-rendered map — the terminal's exact
// UX (ui.TargetKey: hjklyubn/arrows steer, Enter confirms, Esc/q cancels).
func (sc *Screen) Target(origin game.Pos) (game.Pos, bool) {
	v := sc.last
	cur := ui.ClampPos(origin, v.W, v.H)
	for {
		sc.screen.Set("innerHTML", RenderHTML(v, &cur))
		sc.sendGrid(v, cur.X, cur.Y)
		var res ui.PromptResult
		if cur, res = ui.TargetKey(<-sc.keys, cur, v.W, v.H); res != ui.PromptContinue {
			sc.screen.Set("innerHTML", RenderHTML(v, nil))
			sc.sendGrid(v, -1, -1)
			return cur, res == ui.PromptPick
		}
	}
}

// OnVisibility calls hidden when the page goes out of sight or is being
// unloaded (tab switch, app switch, close, reload), and shown when it comes
// back — including a restore from the back/forward cache.
func (sc *Screen) OnVisibility(hidden, shown func()) {
	doc, win := sc.doc, js.Global()
	onChange := js.FuncOf(func(js.Value, []js.Value) any {
		if doc.Get("visibilityState").String() == "hidden" {
			hidden()
		} else {
			shown()
		}
		return nil
	})
	doc.Call("addEventListener", "visibilitychange", onChange)
	win.Call("addEventListener", "pagehide", js.FuncOf(func(js.Value, []js.Value) any { hidden(); return nil }))
	win.Call("addEventListener", "pageshow", js.FuncOf(func(this js.Value, args []js.Value) any {
		if len(args) > 0 && args[0].Get("persisted").Bool() {
			shown()
		}
		return nil
	}))
}

// Overlay shows a terminal-style end screen (save confirmation, the morgue);
// the game is over, so Enter now reloads the page.
func (sc *Screen) Overlay(text string) {
	sc.final = true
	sc.over.Set("hidden", false)
	sc.over.Set("textContent", text)
}
