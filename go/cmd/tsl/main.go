// Command tsl is the console front-end for the Go port of The Slimy Lichmummy.
package main

import (
	"fmt"
	"os"
	"time"

	"github.com/c0ze/tsl/data"
	"github.com/c0ze/tsl/internal/behaviors"
	"github.com/c0ze/tsl/internal/content"
	"github.com/c0ze/tsl/internal/game"
	"github.com/c0ze/tsl/internal/gen"
	"github.com/c0ze/tsl/internal/rng"
	"github.com/c0ze/tsl/internal/ui"
	tcellui "github.com/c0ze/tsl/internal/ui/tcell"
)

const (
	mapW = 60
	mapH = 24
)

func main() {
	outcome, err := run()
	if err != nil {
		fmt.Fprintln(os.Stderr, "tsl:", err)
		os.Exit(1)
	}
	if outcome != "" {
		fmt.Println(outcome)
	}
}

func run() (string, error) {
	c, err := content.Load(data.Files)
	if err != nil {
		return "", err
	}
	g, err := newGame(c, uint32(time.Now().UnixNano()))
	if err != nil {
		return "", err
	}
	screen, err := tcellui.New()
	if err != nil {
		return "", err
	}
	defer screen.Close()
	if err := ui.Run(g, screen, screen); err != nil {
		return "", err
	}
	switch {
	case g.Won:
		return "You escaped the dungeon victorious!", nil
	case g.Dead:
		if err := os.WriteFile("morgue.txt", []byte(g.MorgueText()), 0o644); err != nil {
			return fmt.Sprintf("You have died. (Failed to write morgue: %v)", err), nil
		}
		return "You have died. A morgue was written to morgue.txt.", nil
	default:
		return "You leave the dungeon. Farewell.", nil
	}
}

// newGame builds a fresh, procedurally generated dungeon seeded by seed, and
// wires a level generator so the player can descend.
func newGame(c *content.Content, seed uint32) (*game.Game, error) {
	r := rng.NewWithSeed(seed)
	lvl, start, _, err := gen.Rooms(r, c, mapW, mapH)
	if err != nil {
		return nil, err
	}
	const startHP = 20
	g := &game.Game{
		Content:   c,
		Level:     lvl,
		Player:    start,
		RNG:       r,
		PlayerHP:  startHP,
		PlayerMax: startHP,
		Behaviors: behaviors.Registry(),
		Depth:     1,
	}
	if err := game.ValidateItemUses(c, g.Behaviors); err != nil {
		return nil, err
	}
	g.NewLevelFn = func(depth int) (*game.Level, game.Pos, error) {
		l, s, _, err := gen.Rooms(r, c, mapW, mapH)
		return l, s, err
	}
	return g, nil
}
