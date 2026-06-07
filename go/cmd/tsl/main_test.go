package main

import (
	"testing"

	"github.com/c0ze/tsl/internal/content"
	"github.com/c0ze/tsl/internal/game"
)

func testTiles() *content.Content {
	return &content.Content{Tiles: map[string]*content.TileDef{
		"floor":       {ID: "floor", Glyph: ".", Color: content.ColorNormal, Passable: true, Transparent: true},
		"wall":        {ID: "wall", Glyph: "#", Color: content.ColorNormal, Passable: false, Transparent: false},
		"stairs_down": {ID: "stairs_down", Glyph: ">", Color: content.ColorNormal, Passable: true, Transparent: true},
	}}
}

func TestNewGamePlayable(t *testing.T) {
	c := testTiles()
	g, err := newGame(c, 12345)
	if err != nil {
		t.Fatalf("newGame: %v", err)
	}
	if !g.Level.Passable(g.Player) {
		t.Errorf("player start %v is not passable", g.Player)
	}
	moved := false
	for _, d := range []game.Direction{game.DirN, game.DirE, game.DirS, game.DirW} {
		probe := &game.Game{Content: c, Level: g.Level, Player: g.Player}
		if probe.Move(d) {
			moved = true
			break
		}
	}
	if !moved {
		t.Error("player cannot move in any cardinal direction from start")
	}
}

func TestNewGameDeterministic(t *testing.T) {
	c := testTiles()
	a, err := newGame(c, 999)
	if err != nil {
		t.Fatalf("newGame(a): %v", err)
	}
	b, err := newGame(c, 999)
	if err != nil {
		t.Fatalf("newGame(b): %v", err)
	}
	if a.Player != b.Player {
		t.Errorf("same seed gave different starts: %v vs %v", a.Player, b.Player)
	}
}

func TestNewGameHasPlayerHPAndRNG(t *testing.T) {
	c := testTiles()
	g, err := newGame(c, 1)
	if err != nil {
		t.Fatalf("newGame: %v", err)
	}
	if g.PlayerHP <= 0 || g.PlayerMax <= 0 {
		t.Errorf("player HP not initialised: hp=%d max=%d", g.PlayerHP, g.PlayerMax)
	}
	if g.RNG == nil {
		t.Error("game RNG not set")
	}
}
