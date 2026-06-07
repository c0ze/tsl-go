package main

import (
	"testing"

	"github.com/c0ze/tsl/internal/content"
	"github.com/c0ze/tsl/internal/game"
	"github.com/c0ze/tsl/internal/ui"
)

type scriptPrompter struct {
	actions []ui.Action
	i       int
}

func (s *scriptPrompter) NextAction() (ui.Action, error) {
	if s.i >= len(s.actions) {
		return ui.Action{Kind: ui.ActQuit}, nil
	}
	a := s.actions[s.i]
	s.i++
	return a, nil
}

type nullRenderer struct{}

func (nullRenderer) Render(ui.View) {}

func TestDemoGameWalk(t *testing.T) {
	c := &content.Content{Tiles: map[string]*content.TileDef{
		"floor": {ID: "floor", Glyph: ".", Color: content.ColorNormal, Passable: true, Transparent: true},
		"wall":  {ID: "wall", Glyph: "#", Color: content.ColorNormal, Passable: false, Transparent: false},
	}}
	g, err := demoGame(c)
	if err != nil {
		t.Fatalf("demoGame: %v", err)
	}
	// '@' starts at (6,3). Move east twice to (8,3); a third east hits the wall at (9,3).
	p := &scriptPrompter{actions: []ui.Action{
		{Kind: ui.ActMove, Dir: game.DirE},
		{Kind: ui.ActMove, Dir: game.DirE},
		{Kind: ui.ActMove, Dir: game.DirE},
		{Kind: ui.ActQuit},
	}}
	if err := ui.Run(g, p, nullRenderer{}); err != nil {
		t.Fatalf("Run: %v", err)
	}
	if g.Player != (game.Pos{X: 8, Y: 3}) {
		t.Errorf("player = %v, want {8 3}", g.Player)
	}
}
