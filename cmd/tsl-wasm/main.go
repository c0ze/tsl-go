//go:build js && wasm

// Command tsl-wasm is the browser front-end: the same engine and content,
// rendered into the DOM, with saves in localStorage.
package main

import (
	"errors"
	"strings"
	"syscall/js"
	"time"

	"github.com/c0ze/tsl-go/data"
	"github.com/c0ze/tsl-go/internal/behaviors"
	"github.com/c0ze/tsl-go/internal/boot"
	"github.com/c0ze/tsl-go/internal/content"
	"github.com/c0ze/tsl-go/internal/game"
	"github.com/c0ze/tsl-go/internal/gen"
	"github.com/c0ze/tsl-go/internal/ui"
	"github.com/c0ze/tsl-go/internal/ui/web"
)

const saveKey = "tsl-go-save"

func main() {
	sc := web.New()
	if err := play(sc); err != nil {
		sc.Overlay("tsl-go error: " + err.Error())
	}
	select {} // keep the wasm runtime alive for the final overlay
}

func play(sc *web.Screen) error {
	c, err := content.Load(data.Files)
	if err != nil {
		return err
	}
	g, err := resume(c)
	if err != nil {
		return err
	}
	if g == nil {
		if g, err = boot.NewGame(c, uint32(time.Now().UnixNano())); err != nil {
			return err
		}
	}
	for {
		err := ui.Run(g, sc, sc)
		if errors.Is(err, ui.ErrSaveRequested) {
			var b strings.Builder
			if serr := g.Save(&b); serr != nil {
				g.Messages = append(g.Messages, "Couldn't save game!")
				continue // never crash a live game over a failed save
			}
			localStorage().Call("setItem", saveKey, b.String())
			sc.Overlay("Game saved.\n\nReload the page to resume.")
			return nil
		}
		if err != nil {
			return err
		}
		break
	}
	switch {
	case g.Won:
		sc.Overlay("You escaped the dungeon victorious!\n\nReload to descend again.")
	case g.Dead:
		sc.Overlay(g.MorgueText() + "\nReload to try again.")
	default:
		sc.Overlay("You leave the dungeon. Farewell.\n\nReload to descend again.")
	}
	return nil
}

// resume restores a localStorage save and deletes it (the no-scumming rule),
// or returns nil for a fresh descent.
func resume(c *content.Content) (*game.Game, error) {
	raw := localStorage().Call("getItem", saveKey)
	if raw.IsNull() || raw.String() == "" {
		return nil, nil
	}
	var g *game.Game
	build := func(def *content.LevelDef) (*game.Level, error) {
		return gen.LevelFromDef(g.RNG, c, def)
	}
	g, err := game.LoadGame(strings.NewReader(raw.String()), c, behaviors.Registry(), build)
	if err != nil {
		return nil, err
	}
	localStorage().Call("removeItem", saveKey)
	return g, nil
}

func localStorage() js.Value { return js.Global().Get("localStorage") }
