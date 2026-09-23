package tcell

import (
	"testing"

	tc "github.com/gdamore/tcell/v2"

	"github.com/c0ze/tsl-go/internal/content"
	"github.com/c0ze/tsl-go/internal/game"
	"github.com/c0ze/tsl-go/internal/ui"
)

func TestRenderToSimulationScreen(t *testing.T) {
	sim := tc.NewSimulationScreen("")
	if err := sim.Init(); err != nil {
		t.Fatal(err)
	}
	defer sim.Fini()
	sim.SetSize(3, 3+footerRows)

	sc := NewWith(sim)
	v := ui.View{W: 3, H: 3, Cells: make([]ui.Cell, 9)}
	for i := range v.Cells {
		v.Cells[i] = ui.Cell{Glyph: '.', Color: content.ColorNormal}
	}
	*v.At(1, 1) = ui.Cell{Glyph: '@', Color: content.ColorNormal}
	sc.Render(v)

	cells, w, _ := sim.GetContents()
	if got := cells[1*w+1].Runes[0]; got != '@' {
		t.Errorf("cell (1,1) = %q, want '@'", got)
	}
	if got := cells[0].Runes[0]; got != '.' {
		t.Errorf("cell (0,0) = %q, want '.'", got)
	}
}

func TestKeyToAction(t *testing.T) {
	cases := []struct {
		ev   *tc.EventKey
		want ui.Action
		ok   bool
	}{
		{tc.NewEventKey(tc.KeyRune, 'l', tc.ModNone), ui.Action{Kind: ui.ActMove, Dir: game.DirE}, true},
		{tc.NewEventKey(tc.KeyRune, 'k', tc.ModNone), ui.Action{Kind: ui.ActMove, Dir: game.DirN}, true},
		{tc.NewEventKey(tc.KeyLeft, 0, tc.ModNone), ui.Action{Kind: ui.ActMove, Dir: game.DirW}, true},
		{tc.NewEventKey(tc.KeyRune, 'Q', tc.ModNone), ui.Action{Kind: ui.ActQuit}, true},
		{tc.NewEventKey(tc.KeyRune, 'q', tc.ModNone), ui.Action{}, false},
		{tc.NewEventKey(tc.KeyRune, 'r', tc.ModAlt), ui.Action{}, false},
		{tc.NewEventKey(tc.KeyRune, 'e', tc.ModNone), ui.Action{Kind: ui.ActEat}, true},
		{tc.NewEventKey(tc.KeyRune, 'z', tc.ModNone), ui.Action{Kind: ui.ActZap}, true},
		{tc.NewEventKey(tc.KeyRune, 'r', tc.ModNone), ui.Action{Kind: ui.ActRead}, true},
		{tc.NewEventKey(tc.KeyRune, 'f', tc.ModNone), ui.Action{Kind: ui.ActFire}, true},
		{tc.NewEventKey(tc.KeyRune, 'c', tc.ModNone), ui.Action{Kind: ui.ActCast}, true},
		{tc.NewEventKey(tc.KeyRune, 't', tc.ModNone), ui.Action{Kind: ui.ActTalk}, true},
		{tc.NewEventKey(tc.KeyRune, 'S', tc.ModNone), ui.Action{Kind: ui.ActSave}, true},
		{tc.NewEventKey(tc.KeyRune, 'X', tc.ModNone), ui.Action{}, false},
	}
	for _, tt := range cases {
		got, ok := keyToAction(tt.ev)
		if ok != tt.ok || got != tt.want {
			t.Errorf("keyToAction(%v) = (%v,%v), want (%v,%v)", tt.ev.Name(), got, ok, tt.want, tt.ok)
		}
	}
}

// On an 80x24 terminal a 60x24 level can't fit with its HUD: the map window
// must scroll to keep the player on screen and leave the status line visible.
func TestRenderScrollsMapOnShortTerminal(t *testing.T) {
	sim := tc.NewSimulationScreen("")
	if err := sim.Init(); err != nil {
		t.Fatal(err)
	}
	defer sim.Fini()
	sim.SetSize(80, 24)

	sc := NewWith(sim)
	v := ui.View{W: 60, H: 24, Cells: make([]ui.Cell, 60*24), Status: "HP 20/20", Player: game.Pos{X: 5, Y: 22}}
	for i := range v.Cells {
		v.Cells[i] = ui.Cell{Glyph: '.', Color: content.ColorNormal}
	}
	*v.At(5, 22) = ui.Cell{Glyph: '@', Color: content.ColorNormal}
	sc.Render(v)

	cells, w, h := sim.GetContents()
	found := false
	for y := 0; y < h-footerRows; y++ {
		if cells[y*w+5].Runes[0] == '@' {
			found = true
		}
	}
	if !found {
		t.Error("player scrolled off the map window")
	}
	if got := cells[(h-footerRows)*w].Runes[0]; got != 'H' {
		t.Errorf("status line not drawn at row %d (got %q)", h-footerRows, got)
	}
}
