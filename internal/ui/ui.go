// Package ui defines the rendering/input boundary between the engine and any
// front-end. The engine never imports a concrete UI; front-ends implement
// Prompter and Renderer.
package ui

import (
	"errors"
	"fmt"

	"github.com/c0ze/tsl-go/internal/game"
)

// ActionKind enumerates player intents.
type ActionKind int

const (
	ActNone ActionKind = iota
	ActMove
	ActQuit
	ActPickup
	ActInventory
	ActTravel
	ActEat
	ActZap
	ActRead
	ActFire
	ActCast
	ActTalk
	ActSave
	ActClose
)

// Action is a decoded player intent. Dir is meaningful only when Kind==ActMove.
type Action struct {
	Kind ActionKind
	Dir  game.Direction
}

// MenuSpec describes a selectable list for the front-end to present.
type MenuSpec struct {
	Title string
	Items []string
}

// Prompter supplies player actions, menu selections, and targeting.
type Prompter interface {
	NextAction() (Action, error)
	Menu(MenuSpec) (index int, ok bool)
	Target(origin game.Pos) (game.Pos, bool)
}

// Renderer draws a View.
type Renderer interface {
	Render(View)
}

// ActionForRune decodes a key rune into a player action — the single binding
// table shared by every front-end (front-ends map their arrow keys onto hjkl first).
func ActionForRune(r rune) (Action, bool) {
	switch r {
	case 'h':
		return Action{Kind: ActMove, Dir: game.DirW}, true
	case 'l':
		return Action{Kind: ActMove, Dir: game.DirE}, true
	case 'k':
		return Action{Kind: ActMove, Dir: game.DirN}, true
	case 'j':
		return Action{Kind: ActMove, Dir: game.DirS}, true
	case 'y':
		return Action{Kind: ActMove, Dir: game.DirNW}, true
	case 'u':
		return Action{Kind: ActMove, Dir: game.DirNE}, true
	case 'b':
		return Action{Kind: ActMove, Dir: game.DirSW}, true
	case 'n':
		return Action{Kind: ActMove, Dir: game.DirSE}, true
	case 'Q': // the C binds quit to capital Q (keymap.c:170) and asks first
		return Action{Kind: ActQuit}, true
	case 'g':
		return Action{Kind: ActPickup}, true
	case 'i':
		return Action{Kind: ActInventory}, true
	case 'z':
		return Action{Kind: ActZap}, true
	case 'e':
		return Action{Kind: ActEat}, true
	case 'r':
		return Action{Kind: ActRead}, true
	case 'f':
		return Action{Kind: ActFire}, true
	case 'c':
		return Action{Kind: ActCast}, true
	case 't':
		return Action{Kind: ActTalk}, true
	case 'S':
		return Action{Kind: ActSave}, true
	case 'O': // the C's own default keymap binds action_close here (keymap.c:219)
		return Action{Kind: ActClose}, true
	case '>':
		return Action{Kind: ActTravel}, true
	}
	return Action{}, false
}

// QuitPrompt is the confirmation Q asks before ending the run.
const QuitPrompt = "Really quit?"

// ErrSaveRequested is returned by Run when the player asks to save: the ui
// layer never touches files, so the front-end's owner (cmd) saves and quits
// — the C's save-is-quitting (saveload.c try_to_save_game).
var ErrSaveRequested = errors.New("save requested")

// Run is the core game loop: recompute FOV, render, get an action, apply it.
func Run(g *game.Game, p Prompter, r Renderer) error {
	for {
		g.UpdateFOV()
		r.Render(BuildView(g))
		g.Sounds = nil // cues were drained into the rendered View; reset for the next turn
		if g.Dead || g.Won {
			return nil
		}
		a, err := p.NextAction()
		if err != nil {
			return err
		}
		switch a.Kind {
		case ActQuit:
			// The C's "Really quit?" (player.c:501): a quit ends the run for good.
			if idx, ok := p.Menu(MenuSpec{Title: QuitPrompt, Items: []string{"Yes", "No"}}); ok && idx == 0 {
				return nil
			}
		case ActMove:
			g.PlayerStep(a.Dir)
			if pos, ok := g.TakeLockedBump(); ok {
				promptLockedDoor(g, p, pos)
			}
			if pos, ok := g.TakeLavaBump(); ok {
				if idx, ok := p.Menu(MenuSpec{Title: "Step into the lava?", Items: []string{"Yes", "No"}}); ok && idx == 0 {
					g.EnterLava(pos)
				}
			}
		case ActPickup:
			g.PlayerPickup()
		case ActInventory:
			if it := chooseItem(g, p, "Inventory", g.Inventory); it != nil {
				g.PlayerUse(it)
			}
		case ActTravel:
			g.Travel()
		case ActTalk:
			g.Talk()
		case ActClose:
			closeDoorPrompt(g, p)
		case ActSave:
			return ErrSaveRequested
		case ActEat:
			food := g.EdibleInventory()
			if len(food) == 0 {
				g.Messages = append(g.Messages, "You have nothing to eat.")
				break
			}
			if it := chooseItem(g, p, "Eat what?", food); it != nil {
				g.PlayerUse(it)
			}
		case ActRead:
			scrolls := g.ReadableInventory()
			if len(scrolls) == 0 {
				g.Messages = append(g.Messages, "You have nothing to read.")
				break
			}
			if it := chooseItem(g, p, "Read what?", scrolls); it != nil {
				g.PlayerUse(it)
			}
		case ActZap:
			wands := g.WandInventory()
			if len(wands) == 0 {
				g.Messages = append(g.Messages, "You have no wand to zap.")
				break
			}
			names := make([]string, len(wands))
			for i, it := range wands {
				names[i] = fmt.Sprintf("%s (%d charges)", g.DisplayName(it), it.Charges)
			}
			idx, ok := p.Menu(MenuSpec{Title: "Zap which wand?", Items: names})
			if !ok || idx < 0 || idx >= len(wands) {
				break
			}
			if target, ok := p.Target(g.Player); ok {
				g.ZapWand(wands[idx], target)
			}
		case ActFire:
			if !g.WieldedRanged() {
				g.Messages = append(g.Messages, "You have no ranged weapon to fire.")
				break
			}
			if target, ok := p.Target(g.Player); ok {
				g.FireWeapon(target)
			}
		case ActCast:
			spells := g.SpellInventory()
			if len(spells) == 0 {
				g.Messages = append(g.Messages, "You know no spells to cast.")
				break
			}
			names := make([]string, len(spells))
			for i, it := range spells {
				names[i] = fmt.Sprintf("%s (%d EP)", g.DisplayName(it), it.Def.Cost)
			}
			idx, ok := p.Menu(MenuSpec{Title: "Cast which spell?", Items: names})
			if !ok || idx < 0 || idx >= len(spells) {
				break
			}
			spell := spells[idx]
			if spell.Def != nil && (spell.Def.Ranged > 0 || spell.Def.Breath != "" || spell.Def.Deathspell) { // a targeted or directed spell
				if target, ok := p.Target(g.Player); ok {
					g.CastSpellAt(spell, target)
				}
			} else {
				g.CastSpell(spell)
			}
		}
	}
}

// chooseItem offers items by display name and returns the one picked, or nil
// when the list is empty or the menu was cancelled.
func chooseItem(g *game.Game, p Prompter, title string, items []*game.Item) *game.Item {
	if len(items) == 0 {
		return nil
	}
	names := make([]string, len(items))
	for i, it := range items {
		names[i] = g.DisplayName(it)
	}
	if idx, ok := p.Menu(MenuSpec{Title: title, Items: names}); ok && idx >= 0 && idx < len(items) {
		return items[idx]
	}
	return nil
}

// promptLockedDoor drives the C's locked-door bump chain (doors.c:275): offer
// to spend a key, then offer to break the door; declining both is the C's
// "Never mind, then." (and costs nothing).
func promptLockedDoor(g *game.Game, p Prompter, pos game.Pos) {
	if n := g.KeyCount(); n > 0 {
		word := "keys"
		if n == 1 {
			word = "key"
		}
		if idx, ok := p.Menu(MenuSpec{Title: fmt.Sprintf("Unlock it (%d %s)?", n, word), Items: []string{"Yes", "No"}}); ok && idx == 0 {
			g.UnlockDoor(pos)
			return
		}
	}
	title := "Attempt to break it?"
	if g.HasCrowbar() {
		title = "Attempt to break it with your crowbar?"
	}
	if idx, ok := p.Menu(MenuSpec{Title: title, Items: []string{"Yes", "No"}}); ok && idx == 0 {
		g.ForceDoor(pos)
		return
	}
	g.Messages = append(g.Messages, "Never mind, then.")
}

// closeDirs orders the close-door menu the way the C's direction prompt reads.
var closeDirs = []struct {
	name   string
	dx, dy int
}{

	{"North", 0, -1}, {"South", 0, 1}, {"West", -1, 0}, {"East", 1, 0},
	{"Northwest", -1, -1}, {"Northeast", 1, -1}, {"Southwest", -1, 1}, {"Southeast", 1, 1},
}

// closeDoorPrompt is the close verb (the C's close_door): close the adjacent
// open door, asking "Close which door?" only when several qualify.
func closeDoorPrompt(g *game.Game, p Prompter) {
	if g.RefuseDoors("close") { // asked before any prompt, as the C does
		return
	}
	var names []string
	var spots []game.Pos
	alreadyClosed := false
	for _, d := range closeDirs {
		pos := game.Pos{X: g.Player.X + d.dx, Y: g.Player.Y + d.dy}
		if !g.Level.InBounds(pos) {
			continue
		}
		def := g.Level.At(pos).Def
		switch {
		case def.ClosesTo != "":
			names = append(names, d.name)
			spots = append(spots, pos)
		case def.OpensTo != "":
			alreadyClosed = true
		}
	}
	switch {
	case len(spots) == 0 && alreadyClosed:
		g.Messages = append(g.Messages, "It is already closed.")
	case len(spots) == 0:
		g.Messages = append(g.Messages, "There is no door there.")
	case len(spots) == 1:
		g.CloseDoor(spots[0])
	default:
		if idx, ok := p.Menu(MenuSpec{Title: "Close which door?", Items: names}); ok && idx >= 0 && idx < len(spots) {
			g.CloseDoor(spots[idx])
		}
	}
}
