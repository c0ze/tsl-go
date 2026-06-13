// Package tcell renders the game to a terminal using gdamore/tcell and maps
// key events to UI actions. It is the only package that imports tcell.
package tcell

import (
	"fmt"

	tc "github.com/gdamore/tcell/v2"

	"github.com/c0ze/tsl-go/internal/game"
	"github.com/c0ze/tsl-go/internal/ui"
)

// Screen is a tcell-backed ui.Renderer and ui.Prompter.
type Screen struct {
	s    tc.Screen
	last ui.View // most recently rendered view, reused by Target
}

// New creates and initializes a real terminal screen.
func New() (*Screen, error) {
	s, err := tc.NewScreen()
	if err != nil {
		return nil, err
	}
	if err := s.Init(); err != nil {
		return nil, err
	}
	return &Screen{s: s}, nil
}

// NewWith wraps an existing tcell.Screen (used in tests with a SimulationScreen).
func NewWith(s tc.Screen) *Screen { return &Screen{s: s} }

// Close restores the terminal.
func (sc *Screen) Close() { sc.s.Fini() }

// cellColor resolves a cell's final 24-bit terminal colour: a remembered tile
// shows in the cool, dark colour of memory; a visible one takes its palette hue
// scaled by the tile's torch-light level.
func cellColor(c ui.Cell) tc.Color {
	base := ui.ColorRGB(c.Color)
	out := ui.Lit(base, c.Light)
	if c.Dim {
		out = ui.Remembered(base)
	}
	return tc.NewRGBColor(int32(out.R), int32(out.G), int32(out.B))
}

// Render draws the view (map then message lines) and flushes it.
func (sc *Screen) Render(v ui.View) {
	sc.last = v
	sc.drawView(v)
	sc.s.Show()
}

// drawView paints the map, status line, and messages (without flushing).
func (sc *Screen) drawView(v ui.View) {
	sc.s.Clear()
	for y := 0; y < v.H; y++ {
		for x := 0; x < v.W; x++ {
			c := v.At(x, y)
			st := tc.StyleDefault.Foreground(cellColor(*c))
			sc.s.SetContent(x, y, c.Glyph, nil, st)
		}
	}
	drawString(sc.s, 0, v.H, v.Status)
	for i, msg := range v.Messages {
		drawString(sc.s, 0, v.H+1+i, msg)
	}
}

func drawString(s tc.Screen, x, y int, str string) {
	for i, r := range str {
		s.SetContent(x+i, y, r, nil, tc.StyleDefault)
	}
}

// NextAction blocks for a key event and maps it to a ui.Action.
func (sc *Screen) NextAction() (ui.Action, error) {
	for {
		switch ev := sc.s.PollEvent().(type) {
		case *tc.EventKey:
			if a, ok := keyToAction(ev); ok {
				return a, nil
			}
		case *tc.EventResize:
			sc.s.Sync()
		}
	}
}

func keyToAction(ev *tc.EventKey) (ui.Action, bool) {
	switch ev.Key() {
	case tc.KeyUp:
		return ui.Action{Kind: ui.ActMove, Dir: game.DirN}, true
	case tc.KeyDown:
		return ui.Action{Kind: ui.ActMove, Dir: game.DirS}, true
	case tc.KeyLeft:
		return ui.Action{Kind: ui.ActMove, Dir: game.DirW}, true
	case tc.KeyRight:
		return ui.Action{Kind: ui.ActMove, Dir: game.DirE}, true
	}
	return ui.ActionForRune(ev.Rune())
}

// Menu presents a blocking list; arrows/jk move, Enter/letter selects, Esc/q cancels.
func (sc *Screen) Menu(m ui.MenuSpec) (int, bool) {
	if len(m.Items) == 0 {
		return 0, false
	}
	sel := 0
	for {
		sc.s.Clear()
		drawString(sc.s, 0, 0, m.Title)
		for i, it := range m.Items {
			prefix := "  "
			if i == sel {
				prefix = "> "
			}
			drawString(sc.s, 0, i+1, fmt.Sprintf("%s%c) %s", prefix, 'a'+i, it))
		}
		sc.s.Show()
		ev, ok := sc.s.PollEvent().(*tc.EventKey)
		if !ok {
			continue
		}
		switch {
		case ev.Key() == tc.KeyUp || ev.Rune() == 'k':
			sel = (sel - 1 + len(m.Items)) % len(m.Items)
		case ev.Key() == tc.KeyDown || ev.Rune() == 'j':
			sel = (sel + 1) % len(m.Items)
		case ev.Key() == tc.KeyEnter:
			return sel, true
		case ev.Key() == tc.KeyEscape || ev.Rune() == 'q':
			return 0, false
		case ev.Rune() >= 'a' && int(ev.Rune()-'a') < len(m.Items):
			return int(ev.Rune() - 'a'), true
		}
	}
}

// Target lets the player move a cursor over the last-rendered map and pick a
// tile. Arrows/hjkl move the cursor; Enter confirms; Esc/q cancels.
func (sc *Screen) Target(origin game.Pos) (game.Pos, bool) {
	v := sc.last
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
	cur := clamp(origin)
	cursorStyle := tc.StyleDefault.Foreground(tc.ColorYellow).Reverse(true)
	for {
		sc.drawView(v)
		drawString(sc.s, 0, v.H+1+len(v.Messages), "Aim: move cursor, Enter to fire, Esc to cancel")
		sc.s.SetContent(cur.X, cur.Y, '*', nil, cursorStyle)
		sc.s.Show()
		ev, ok := sc.s.PollEvent().(*tc.EventKey)
		if !ok {
			continue
		}
		switch {
		case ev.Key() == tc.KeyEnter:
			return cur, true
		case ev.Key() == tc.KeyEscape || ev.Rune() == 'q':
			return game.Pos{}, false
		case ev.Key() == tc.KeyUp || ev.Rune() == 'k':
			cur = clamp(game.Pos{X: cur.X, Y: cur.Y - 1})
		case ev.Key() == tc.KeyDown || ev.Rune() == 'j':
			cur = clamp(game.Pos{X: cur.X, Y: cur.Y + 1})
		case ev.Key() == tc.KeyLeft || ev.Rune() == 'h':
			cur = clamp(game.Pos{X: cur.X - 1, Y: cur.Y})
		case ev.Key() == tc.KeyRight || ev.Rune() == 'l':
			cur = clamp(game.Pos{X: cur.X + 1, Y: cur.Y})
		}
	}
}
