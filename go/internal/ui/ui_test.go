package ui

import (
	"testing"

	"github.com/c0ze/tsl/internal/content"
	"github.com/c0ze/tsl/internal/game"
)

func testGame(t *testing.T, rows []string) *game.Game {
	t.Helper()
	c := &content.Content{Tiles: map[string]*content.TileDef{
		"floor": {ID: "floor", Glyph: ".", Color: content.ColorNormal, Passable: true, Transparent: true},
		"wall":  {ID: "wall", Glyph: "#", Color: content.ColorNormal, Passable: false, Transparent: false},
	}}
	lvl, start, err := game.ParseLevel(c, rows, map[rune]string{'.': "floor", '#': "wall", '@': "floor"})
	if err != nil {
		t.Fatal(err)
	}
	return &game.Game{Content: c, Level: lvl, Player: start}
}

// scriptPrompter returns a fixed sequence of actions, then ActQuit forever.
type scriptPrompter struct {
	actions []Action
	i       int
}

func (s *scriptPrompter) NextAction() (Action, error) {
	if s.i >= len(s.actions) {
		return Action{Kind: ActQuit}, nil
	}
	a := s.actions[s.i]
	s.i++
	return a, nil
}

type nullRenderer struct{ frames int }

func (n *nullRenderer) Render(View) { n.frames++ }

func TestBuildViewPlacesPlayer(t *testing.T) {
	g := testGame(t, []string{"...", ".@.", "..."})
	g.UpdateFOV()
	v := BuildView(g)
	if v.W != 3 || v.H != 3 {
		t.Fatalf("view size = %dx%d, want 3x3", v.W, v.H)
	}
	if got := v.At(1, 1).Glyph; got != '@' {
		t.Errorf("center glyph = %q, want '@'", got)
	}
	if got := v.At(0, 0).Glyph; got != '.' {
		t.Errorf("corner glyph = %q, want '.'", got)
	}
}

func TestBuildViewVisibilityStates(t *testing.T) {
	g := testGame(t, []string{"...", ".@.", "..."})
	for y := 0; y < g.Level.H; y++ {
		for x := 0; x < g.Level.W; x++ {
			tl := g.Level.At(game.Pos{X: x, Y: y})
			tl.Visible, tl.Seen = false, false
		}
	}
	g.Level.At(game.Pos{X: 1, Y: 1}).Visible = true // player tile
	vis := g.Level.At(game.Pos{X: 0, Y: 1})
	vis.Visible, vis.Seen = true, true
	g.Level.At(game.Pos{X: 0, Y: 0}).Seen = true // remembered only
	// (2,2) stays fully unseen

	v := BuildView(g)
	if c := v.At(1, 1); c.Glyph != '@' {
		t.Errorf("player cell = %q, want '@'", c.Glyph)
	}
	if c := v.At(0, 1); c.Glyph != '.' || c.Dim {
		t.Errorf("visible floor = %q dim=%v, want '.' bright", c.Glyph, c.Dim)
	}
	if c := v.At(0, 0); c.Glyph != '.' || !c.Dim {
		t.Errorf("remembered floor = %q dim=%v, want '.' dim", c.Glyph, c.Dim)
	}
	if c := v.At(2, 2); c.Glyph != ' ' {
		t.Errorf("unseen cell = %q, want blank space", c.Glyph)
	}
}

func TestRunAppliesActionsUntilQuit(t *testing.T) {
	g := testGame(t, []string{".....", ".@...", "....."})
	p := &scriptPrompter{actions: []Action{
		{Kind: ActMove, Dir: game.DirE},
		{Kind: ActMove, Dir: game.DirE},
		{Kind: ActQuit},
	}}
	r := &nullRenderer{}
	if err := Run(g, p, r); err != nil {
		t.Fatalf("Run: %v", err)
	}
	if g.Player != (game.Pos{X: 3, Y: 1}) {
		t.Errorf("player = %v, want {3 1}", g.Player)
	}
	if r.frames == 0 {
		t.Error("expected at least one rendered frame")
	}
}
