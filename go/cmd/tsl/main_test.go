package main

import (
	"testing"

	"github.com/c0ze/tsl/data"
	"github.com/c0ze/tsl/internal/content"
	"github.com/c0ze/tsl/internal/game"
)

func testTiles() *content.Content {
	return &content.Content{
		Tiles: map[string]*content.TileDef{
			"floor":       {ID: "floor", Glyph: ".", Color: content.ColorNormal, Passable: true, Transparent: true},
			"wall":        {ID: "wall", Glyph: "#", Color: content.ColorNormal, Passable: false, Transparent: false},
			"stairs_down": {ID: "stairs_down", Glyph: ">", Color: content.ColorNormal, Passable: true, Transparent: true},
		},
		Items: map[string]*content.ItemDef{},
		Levels: map[string]*content.LevelDef{
			"dungeon": {ID: "dungeon", Name: "the Dungeon", W: 60, H: 24, Start: true},
		},
	}
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

func TestNewGameHasBehaviors(t *testing.T) {
	g, err := newGame(testTiles(), 1)
	if err != nil {
		t.Fatalf("newGame: %v", err)
	}
	if g.Behaviors["heal"] == nil {
		t.Error("expected heal behavior wired into the game")
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

// TestNewGameFromShippedData boots the game on the real embedded content,
// proving the dungeon graph + generator wire up end-to-end.
func TestNewGameFromShippedData(t *testing.T) {
	c, err := content.Load(data.Files)
	if err != nil {
		t.Fatalf("load shipped content: %v", err)
	}
	g, err := newGame(c, 42)
	if err != nil {
		t.Fatalf("newGame on shipped data: %v", err)
	}
	if g.Dungeon == nil || g.Level == nil {
		t.Fatal("dungeon/level not wired")
	}
	if !g.Level.Passable(g.Player) {
		t.Errorf("player start %v not passable", g.Player)
	}
	if g.Weapon == nil || g.Armor == nil {
		t.Error("player should start with a dagger + leather armor equipped")
	}
	// Consumables start unidentified: a healing potion reads by its appearance.
	p := c.Items["healing_potion"]
	if p == nil {
		t.Fatal("shipped content should include healing_potion")
	}
	if got := g.DisplayName(&game.Item{Def: p}); got == p.Name {
		t.Errorf("healing potion should start unidentified, but reads as %q", got)
	}
}

func TestNewGameStartsOnStartLevel(t *testing.T) {
	g, err := newGame(testTiles(), 1)
	if err != nil {
		t.Fatalf("newGame: %v", err)
	}
	if g.Dungeon == nil {
		t.Fatal("dungeon should be wired")
	}
	if g.LocationName() != "the Dungeon" {
		t.Errorf("location = %q, want the Dungeon", g.LocationName())
	}
	if g.Player != g.Level.Start {
		t.Errorf("player at %v, want start level's Start %v", g.Player, g.Level.Start)
	}
}
