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
	sc.drawView(v, v.Player)
	sc.s.Show()
}

// footerRows is what the map must leave free below it: the status line, up to
// four messages, and the aim prompt.
const footerRows = 6

// viewport returns the map's top-left offset so a focus point stays on a
// screen smaller than the map (an 80x24 terminal can't fit a 60x24 level plus
// its HUD): the window centres on focus and stops at the map's edges.
func viewport(v ui.View, focus game.Pos, sw, sh int) (ox, oy, rows int) {
	rows = min(v.H, max(sh-footerRows, 1))
	cols := min(v.W, max(sw, 1))
	ox = min(max(focus.X-cols/2, 0), v.W-cols)
	oy = min(max(focus.Y-rows/2, 0), v.H-rows)
	return ox, oy, rows
}

// drawView paints the map window around focus, then the status line and
// messages beneath it (without flushing). It returns the map offset and the
// row the footer starts on, for the targeting cursor and aim prompt.
func (sc *Screen) drawView(v ui.View, focus game.Pos) (ox, oy, footer int) {
	sc.s.Clear()
	sw, sh := sc.s.Size()
	ox, oy, rows := viewport(v, focus, sw, sh)
	for y := 0; y < rows; y++ {
		for x := 0; x+ox < v.W; x++ {
			c := v.At(x+ox, y+oy)
			st := tc.StyleDefault.Foreground(cellColor(*c))
			sc.s.SetContent(x, y, c.Glyph, nil, st)
		}
	}
	drawString(sc.s, 0, rows, v.Status)
	for i, msg := range v.Messages {
		drawString(sc.s, 0, rows+1+i, msg)
	}
	return ox, oy, rows
}

func drawString(s tc.Screen, x, y int, str string) {
	for i, r := range []rune(str) {
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
			sc.Render(sc.last) // re-fit the map window to the new size
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
	case tc.KeyRune:
		if ev.Modifiers()&(tc.ModAlt|tc.ModCtrl) != 0 {
			return ui.Action{}, false // Alt/Ctrl chords aren't game keys
		}
		return ui.ActionForRune(ev.Rune())
	}
	return ui.Action{}, false
}

// keyName normalises a tcell key event to the shared ui key names.
func keyName(ev *tc.EventKey) string {
	switch ev.Key() {
	case tc.KeyEnter:
		return ui.KeyEnter
	case tc.KeyEscape:
		return ui.KeyEscape
	case tc.KeyUp:
		return ui.KeyUp
	case tc.KeyDown:
		return ui.KeyDown
	case tc.KeyLeft:
		return ui.KeyLeft
	case tc.KeyRight:
		return ui.KeyRight
	case tc.KeyRune:
		if ev.Modifiers()&(tc.ModAlt|tc.ModCtrl) == 0 {
			return string(ev.Rune())
		}
	}
	return ""
}

// Menu presents a blocking list (ui.MenuKey: letters pick, arrows/jk move,
// Enter picks, Esc/q cancels), scrolling it when it outgrows the screen.
func (sc *Screen) Menu(m ui.MenuSpec) (int, bool) {
	if len(m.Items) == 0 {
		return 0, false
	}
	sel := 0
	for {
		sc.s.Clear()
		drawString(sc.s, 0, 0, m.Title)
		_, sh := sc.s.Size()
		rows := max(sh-1, 1)
		top := min(max(sel-rows/2, 0), max(len(m.Items)-rows, 0))
		for i := top; i < len(m.Items) && i-top < rows; i++ {
			prefix := "  "
			if i == sel {
				prefix = "> "
			}
			drawString(sc.s, 0, i-top+1, fmt.Sprintf("%s%c) %s", prefix, 'a'+i, m.Items[i]))
		}
		sc.s.Show()
		ev, ok := sc.s.PollEvent().(*tc.EventKey)
		if !ok {
			continue
		}
		var res ui.PromptResult
		if sel, res = ui.MenuKey(keyName(ev), sel, len(m.Items)); res != ui.PromptContinue {
			return sel, res == ui.PromptPick
		}
	}
}

// Target lets the player move a cursor over the last-rendered map and pick a
// tile (ui.TargetKey: hjklyubn/arrows move, Enter confirms, Esc/q cancels).
func (sc *Screen) Target(origin game.Pos) (game.Pos, bool) {
	v := sc.last
	cur := ui.ClampPos(origin, v.W, v.H)
	cursorStyle := tc.StyleDefault.Foreground(tc.ColorYellow).Reverse(true)
	for {
		ox, oy, footer := sc.drawView(v, cur)
		drawString(sc.s, 0, footer+1+len(v.Messages), "Aim: move cursor, Enter to fire, Esc to cancel")
		sc.s.SetContent(cur.X-ox, cur.Y-oy, '*', nil, cursorStyle)
		sc.s.Show()
		ev, ok := sc.s.PollEvent().(*tc.EventKey)
		if !ok {
			continue
		}
		var res ui.PromptResult
		if cur, res = ui.TargetKey(keyName(ev), cur, v.W, v.H); res != ui.PromptContinue {
			return cur, res == ui.PromptPick
		}
	}
}
