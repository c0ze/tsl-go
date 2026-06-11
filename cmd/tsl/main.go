// Command tsl is the console front-end for the Go port of The Slimy Lichmummy.
package main

import (
	"errors"
	"fmt"
	"os"
	"path/filepath"
	"time"

	"github.com/c0ze/tsl-go/data"
	"github.com/c0ze/tsl-go/internal/behaviors"
	"github.com/c0ze/tsl-go/internal/content"
	"github.com/c0ze/tsl-go/internal/game"
	"github.com/c0ze/tsl-go/internal/gen"
	"github.com/c0ze/tsl-go/internal/rng"
	"github.com/c0ze/tsl-go/internal/ui"
	tcellui "github.com/c0ze/tsl-go/internal/ui/tcell"
)

// version is stamped by the release build (-ldflags "-X main.version=...").
var version = "dev"

func main() {
	if len(os.Args) > 1 && (os.Args[1] == "--version" || os.Args[1] == "-v") {
		fmt.Println("tsl-go", version)
		return
	}
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
	g, err := loadFrom(savePath(), c)
	if err != nil {
		return "", err // a corrupt savefile aborts without deleting it (C saveload_abort)
	}
	if g == nil { // no savefile: a fresh descent
		if g, err = newGame(c, uint32(time.Now().UnixNano())); err != nil {
			return "", err
		}
	}
	screen, err := tcellui.New()
	if err != nil {
		return "", err
	}
	defer screen.Close()
	for {
		err := ui.Run(g, screen, screen)
		if errors.Is(err, ui.ErrSaveRequested) {
			if serr := saveTo(savePath(), g); serr != nil {
				// Never crash a live game over a failed save (C: resume play).
				g.Messages = append(g.Messages, "Couldn't save game!")
				continue
			}
			return "Game saved.", nil
		}
		if err != nil {
			return "", err
		}
		break
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

// savePath is where the savefile lives (the C's SAVE_FILENAME in the home
// directory; the working directory when no home is known).
func savePath() string {
	home, err := os.UserHomeDir()
	if err != nil {
		return ".tsl-save.json"
	}
	return filepath.Join(home, ".tsl-save.json")
}

// saveTo writes the game to path (save-and-quit's file half).
func saveTo(path string, g *game.Game) error {
	f, err := os.Create(path)
	if err != nil {
		return err
	}
	if err := g.Save(f); err != nil {
		f.Close()
		return err
	}
	return f.Close()
}

// loadFrom resumes a saved game if path exists, deleting the savefile on a
// successful restore (no save-scumming, C delete_savefile). No savefile is
// not an error: (nil, nil) means start fresh. A corrupt file errors without
// deletion so nothing is silently lost.
func loadFrom(path string, c *content.Content) (*game.Game, error) {
	f, err := os.Open(path)
	if os.IsNotExist(err) {
		return nil, nil
	}
	if err != nil {
		return nil, err
	}
	var g *game.Game
	build := func(def *content.LevelDef) (*game.Level, error) {
		return gen.LevelFromDef(g.RNG, c, def) // bound after load; called lazily on first entry
	}
	g, err = game.LoadGame(f, c, behaviors.Registry(), build)
	f.Close() // before the delete: Windows refuses to remove an open file
	if err != nil {
		return nil, fmt.Errorf("savefile %s: %w", path, err)
	}
	if err := os.Remove(path); err != nil {
		return nil, err
	}
	return g, nil
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
