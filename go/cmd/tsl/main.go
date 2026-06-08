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

// newGame builds the dungeon graph from content, seeds the generator, and
// places the player on the start level.
func newGame(c *content.Content, seed uint32) (*game.Game, error) {
	if err := game.ValidateItemUses(c, behaviors.Registry()); err != nil {
		return nil, err
	}
	start, err := startLevelID(c)
	if err != nil {
		return nil, err
	}
	r := rng.NewWithSeed(seed)
	dungeon, err := game.NewDungeon(c.Levels, start, func(def *content.LevelDef) (*game.Level, error) {
		return gen.LevelFromDef(r, c, def)
	})
	if err != nil {
		return nil, err
	}
	const startHP = 20
	g := &game.Game{
		Content:   c,
		Dungeon:   dungeon,
		Level:     dungeon.Current(),
		RNG:       r,
		PlayerHP:  startHP,
		PlayerMax: startHP,
		Behaviors: behaviors.Registry(),
	}
	g.EnterStart()
	equipStartingKit(g, c)
	return g, nil
}

// equipStartingKit gives the player a basic dagger + leather armor, equipped, so
// the early game is playable before better gear is found.
func equipStartingKit(g *game.Game, c *content.Content) {
	if d := c.Items["dagger"]; d != nil && d.Kind == "weapon" {
		it := &game.Item{Def: d}
		g.Inventory = append(g.Inventory, it)
		g.Weapon = it
	}
	if a := c.Items["leather_armor"]; a != nil && a.Kind == "armor" {
		it := &game.Item{Def: a}
		g.Inventory = append(g.Inventory, it)
		g.Armor = it
	}
}

// startLevelID returns the id of the single level flagged start.
func startLevelID(c *content.Content) (string, error) {
	for id, l := range c.Levels {
		if l.Start {
			return id, nil
		}
	}
	return "", fmt.Errorf("no start level defined in levels.toml")
}
