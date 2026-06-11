//go:build js && wasm

// Command tsl-wasm is the browser front-end: the same engine and content,
// rendered into the DOM, with saves in localStorage.
package main

import (
	"errors"
	"fmt"
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

const (
	saveKey       = "tsl-go-save"
	quarantineKey = "tsl-go-save-corrupt" // a bad payload parks here for inspection
)

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
			if !putSave(b.String()) {
				g.Messages = append(g.Messages, "Couldn't save game!")
				continue // blocked storage (private mode etc.): keep playing
			}
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
	raw := getSave()
	if raw == "" {
		return nil, nil
	}
	var g *game.Game
	build := func(def *content.LevelDef) (*game.Level, error) {
		return gen.LevelFromDef(g.RNG, c, def)
	}
	g, err := game.LoadGame(strings.NewReader(raw), c, behaviors.Registry(), build)
	if err != nil {
		// Quarantine, don't destroy: the live slot is freed so a reload
		// starts fresh instead of hitting the same fatal overlay forever,
		// while the payload survives under the debug key (#78's rule that a
		// failed load must not silently lose the save).
		quarantineSave(raw)
		return nil, fmt.Errorf("savefile was corrupt (preserved as %q): %w", quarantineKey, err)
	}
	if !clearSave() {
		// Broken storage: play on, but say the single-use rule is at risk.
		g.Messages = append(g.Messages, "The old savefile could not be removed.")
	}
	return g, nil
}

// The storage helpers recover from JS exceptions (blocked or full storage,
// private browsing): syscall/js panics where a browser would throw, and a
// missing save must never take the runtime down with it.

func getSave() (out string) {
	defer func() { _ = recover() }()
	raw := js.Global().Get("localStorage").Call("getItem", saveKey)
	if raw.IsNull() {
		return ""
	}
	return raw.String()
}

func putSave(s string) (ok bool) {
	defer func() { _ = recover() }()
	js.Global().Get("localStorage").Call("setItem", saveKey, s)
	return true
}

func clearSave() (ok bool) {
	defer func() { _ = recover() }()
	js.Global().Get("localStorage").Call("removeItem", saveKey)
	return true
}

// quarantineSave parks a payload under the debug key and frees the live slot;
// best-effort on both counts (storage may be the thing that is broken).
func quarantineSave(raw string) {
	func() {
		defer func() { _ = recover() }()
		js.Global().Get("localStorage").Call("setItem", quarantineKey, raw)
	}()
	clearSave()
}
