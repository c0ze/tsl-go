// Package boot assembles a playable Game from loaded content — the shared
// constructor behind both the terminal and the browser front-ends.
package boot

import (
	"fmt"

	"github.com/c0ze/tsl-go/internal/behaviors"
	"github.com/c0ze/tsl-go/internal/content"
	"github.com/c0ze/tsl-go/internal/game"
	"github.com/c0ze/tsl-go/internal/gen"
	"github.com/c0ze/tsl-go/internal/rng"
)

// NewGame builds the dungeon graph from content, seeds the generator, and
// places the player on the start level.
func NewGame(c *content.Content, seed uint32) (*game.Game, error) {
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
	const startHP, startEP = 20, 10
	g := &game.Game{
		Content:   c,
		Dungeon:   dungeon,
		Level:     dungeon.Current(),
		RNG:       r,
		PlayerHP:  startHP,
		PlayerMax: startHP,
		EP:        startEP,
		EPMax:     startEP,
		Behaviors: behaviors.Registry(),
	}
	g.EnterStart()
	equipStartingKit(g, c)
	g.AssignAppearances() // shuffle which appearance hides each potion/scroll/wand type
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
