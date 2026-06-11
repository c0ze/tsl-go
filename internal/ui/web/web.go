//go:build js && wasm

package web

import (
	"syscall/js"

	"github.com/c0ze/tsl-go/internal/game"
	"github.com/c0ze/tsl-go/internal/ui"
)

// Screen is the browser front-end: keydown events feed a channel and the
// blocking Prompter calls drain it (Go's wasm scheduler yields to the JS
// event loop while goroutines block), Render writes innerHTML.
type Screen struct {
	keys   chan string
	last   ui.View
	doc    js.Value
	screen js.Value // <pre id="screen">
	status js.Value // <div id="status">
	msgs   js.Value // <div id="messages">
	over   js.Value // <pre id="overlay">
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

// Render draws the view: map grid, status line, last messages.
func (sc *Screen) Render(v ui.View) {
	sc.last = v
	sc.over.Set("hidden", true)
	sc.screen.Set("innerHTML", RenderHTML(v, nil))
	sc.status.Set("textContent", v.Status)
	msgs := ""
	for _, m := range v.Messages {
		msgs += m + "\n"
	}
	sc.msgs.Set("textContent", msgs)
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
