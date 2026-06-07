// Package tcell renders the game to a terminal using gdamore/tcell and maps
// key events to UI actions. It is the only package that imports tcell.
package tcell

import (
	"fmt"

	tc "github.com/gdamore/tcell/v2"

	"github.com/c0ze/tsl/internal/content"
	"github.com/c0ze/tsl/internal/game"
	"github.com/c0ze/tsl/internal/ui"
)

// Screen is a tcell-backed ui.Renderer and ui.Prompter.
type Screen struct {
	s tc.Screen
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

var colorMap = map[content.Color]tc.Color{
	content.ColorNormal:  tc.ColorWhite,
	content.ColorBrown:   tc.ColorMaroon,
	content.ColorBlue:    tc.ColorBlue,
	content.ColorRed:     tc.ColorRed,
	content.ColorGreen:   tc.ColorGreen,
	content.ColorCyan:    tc.ColorTeal,
	content.ColorMagenta: tc.ColorFuchsia,
	content.ColorBlack:   tc.ColorBlack,
}

// Render draws the view (map then message lines) and flushes it.
func (sc *Screen) Render(v ui.View) {
	sc.s.Clear()
	for y := 0; y < v.H; y++ {
		for x := 0; x < v.W; x++ {
			c := v.At(x, y)
			st := tc.StyleDefault.Foreground(colorMap[c.Color])
			if c.Dim {
				st = st.Dim(true)
			}
			sc.s.SetContent(x, y, c.Glyph, nil, st)
		}
	}
	drawString(sc.s, 0, v.H, v.Status)
	for i, msg := range v.Messages {
		drawString(sc.s, 0, v.H+1+i, msg)
	}
	sc.s.Show()
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
	switch ev.Rune() {
	case 'h':
		return ui.Action{Kind: ui.ActMove, Dir: game.DirW}, true
	case 'j':
		return ui.Action{Kind: ui.ActMove, Dir: game.DirS}, true
	case 'k':
		return ui.Action{Kind: ui.ActMove, Dir: game.DirN}, true
	case 'l':
		return ui.Action{Kind: ui.ActMove, Dir: game.DirE}, true
	case 'y':
		return ui.Action{Kind: ui.ActMove, Dir: game.DirNW}, true
	case 'u':
		return ui.Action{Kind: ui.ActMove, Dir: game.DirNE}, true
	case 'b':
		return ui.Action{Kind: ui.ActMove, Dir: game.DirSW}, true
	case 'n':
		return ui.Action{Kind: ui.ActMove, Dir: game.DirSE}, true
	case 'q':
		return ui.Action{Kind: ui.ActQuit}, true
	case 'g':
		return ui.Action{Kind: ui.ActPickup}, true
	case 'i':
		return ui.Action{Kind: ui.ActInventory}, true
	case '>':
		return ui.Action{Kind: ui.ActDescend}, true
	}
	return ui.Action{}, false
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
