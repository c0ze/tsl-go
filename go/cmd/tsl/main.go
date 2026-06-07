// Command tsl is the console front-end for the Go port of The Slimy Lichmummy.
package main

import (
	"fmt"
	"os"
	"time"

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
	if err := run(); err != nil {
		fmt.Fprintln(os.Stderr, "tsl:", err)
		os.Exit(1)
	}
}

func run() error {
	// content.Load reads ./data relative to the working directory, so tsl must
	// be run from the go/ module root for now. A configurable data path will
	// come with the real game setup in a later plan.
	c, err := content.Load("data")
	if err != nil {
		return err
	}
	g, err := newGame(c, uint32(time.Now().UnixNano()))
	if err != nil {
		return err
	}
	screen, err := tcellui.New()
	if err != nil {
		return err
	}
	defer screen.Close()
	return ui.Run(g, screen, screen)
}

// newGame builds a fresh, procedurally generated dungeon level seeded by seed.
func newGame(c *content.Content, seed uint32) (*game.Game, error) {
	r := rng.NewWithSeed(seed)
	lvl, start, _, err := gen.Rooms(r, c, mapW, mapH)
	if err != nil {
		return nil, err
	}
	const startHP = 20
	return &game.Game{
		Content:   c,
		Level:     lvl,
		Player:    start,
		RNG:       r,
		PlayerHP:  startHP,
		PlayerMax: startHP,
		Behaviors: behaviors.Registry(),
	}, nil
}
