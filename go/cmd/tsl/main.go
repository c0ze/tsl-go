// Command tsl is the console front-end for the Go port of The Slimy Lichmummy.
package main

import (
	"fmt"
	"os"

	"github.com/c0ze/tsl/internal/content"
	"github.com/c0ze/tsl/internal/game"
	"github.com/c0ze/tsl/internal/ui"
	tcellui "github.com/c0ze/tsl/internal/ui/tcell"
)

func main() {
	if err := run(); err != nil {
		fmt.Fprintln(os.Stderr, "tsl:", err)
		os.Exit(1)
	}
}

func run() error {
	c, err := content.Load("data")
	if err != nil {
		return err
	}
	g, err := demoGame(c)
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

// demoGame builds a small hand-made room to walk around in. This is a Plan 1
// placeholder; dungeon generation replaces it in Plan 2.
func demoGame(c *content.Content) (*game.Game, error) {
	rows := []string{
		"##########",
		"#........#",
		"#..####..#",
		"#..#..@..#",
		"#..####..#",
		"#........#",
		"##########",
	}
	legend := map[rune]string{'#': "wall", '.': "floor", '@': "floor"}
	lvl, start, err := game.ParseLevel(c, rows, legend)
	if err != nil {
		return nil, err
	}
	return &game.Game{Content: c, Level: lvl, Player: start}, nil
}
