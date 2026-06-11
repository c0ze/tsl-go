// Package ui defines the rendering/input boundary between the engine and any
// front-end. The engine never imports a concrete UI; front-ends implement
// Prompter and Renderer.
package ui

import (
	"errors"
	"fmt"
	"strings"

	"github.com/c0ze/tsl-go/internal/content"
	"github.com/c0ze/tsl-go/internal/game"
)

// Cell is one rendered grid cell.
type Cell struct {
	Glyph rune
	Color content.Color
	Dim   bool // render dimmed (remembered-but-not-currently-visible)
}

// View is a read-only snapshot the front-end draws.
type View struct {
	W, H     int
	Cells    []Cell // len W*H, row-major
	Status   string // HUD line (HP, depth, gear)
	Messages []string
}

// At returns a pointer to the cell at (x, y), which must be in bounds
// (0 <= x < W, 0 <= y < H). All callers in this package satisfy that.
func (v *View) At(x, y int) *Cell { return &v.Cells[y*v.W+x] }

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
// table shared by every front-end (the terminal adds arrows/Enter on top).
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
	case 'q':
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

// ErrSaveRequested is returned by Run when the player asks to save: the ui
// layer never touches files, so the front-end's owner (cmd) saves and quits
// — the C's save-is-quitting (saveload.c try_to_save_game).
var ErrSaveRequested = errors.New("save requested")

// Player avatar rendering (Plan 1; later the player becomes a creature def).
const PlayerGlyph = '@'

// PlayerColor is the player's glyph color.
const PlayerColor = content.ColorNormal

// BuildView produces the View for the current game state: tiles in the player's
// FOV are drawn bright, remembered (Seen) tiles dim, and unseen tiles blank.
func BuildView(g *game.Game) View {
	l := g.Level
	v := View{W: l.W, H: l.H, Cells: make([]Cell, l.W*l.H)}
	for y := 0; y < l.H; y++ {
		for x := 0; x < l.W; x++ {
			t := l.At(game.Pos{X: x, Y: y})
			def := t.Appears() // an unrevealed trap wears its disguise
			switch {
			case t.Visible:
				*v.At(x, y) = Cell{Glyph: def.Rune(), Color: def.Color}
			case t.Seen:
				*v.At(x, y) = Cell{Glyph: def.Rune(), Color: def.Color, Dim: true}
			default:
				*v.At(x, y) = Cell{Glyph: ' ', Color: content.ColorNormal}
			}
		}
	}
	for _, it := range l.Items {
		if l.InBounds(it.Pos) && l.At(it.Pos).Visible {
			*v.At(it.Pos.X, it.Pos.Y) = Cell{Glyph: it.Def.Rune(), Color: it.Def.Color}
		}
	}
	for _, m := range l.Creatures {
		if !l.InBounds(m.Pos) || !l.At(m.Pos).Visible {
			continue
		}
		if m.Disguised && m.DisguiseAs != nil { // a mimic wears its loot glamour
			*v.At(m.Pos.X, m.Pos.Y) = Cell{Glyph: m.DisguiseAs.Rune(), Color: m.DisguiseAs.Color}
			continue
		}
		*v.At(m.Pos.X, m.Pos.Y) = Cell{Glyph: m.Def.Rune(), Color: m.Def.Color}
	}
	if l.InBounds(g.Player) {
		if g.Shape != nil { // a polymorphed player wears the form's glyph
			*v.At(g.Player.X, g.Player.Y) = Cell{Glyph: g.Shape.Rune(), Color: g.Shape.Color}
		} else {
			*v.At(g.Player.X, g.Player.Y) = Cell{Glyph: PlayerGlyph, Color: PlayerColor}
		}
	}
	v.Status = statusLine(g)
	v.Messages = lastN(g.Messages, 4)
	return v
}

// statusLine summarises the player's vitals and gear for the HUD.
func statusLine(g *game.Game) string {
	wield, wear := "none", "none"
	if g.Weapon != nil && g.Weapon.Def != nil {
		wield = g.DisplayName(g.Weapon)
	}
	if g.Armor != nil && g.Armor.Def != nil {
		wear = g.DisplayName(g.Armor)
	}
	loc := g.LocationName()
	if loc == "" {
		loc = "the dungeon"
	}
	s := fmt.Sprintf("HP %d/%d", g.PlayerHP, g.PlayerMax)
	if g.EPMax > 0 {
		s += fmt.Sprintf("   EP %d/%d", g.EP, g.EPMax)
	}
	s += fmt.Sprintf("   %s   Wield: %s   Wear: %s", loc, wield, wear)
	if worn := wornAccessories(g); worn != "" {
		s += "   Worn: " + worn
	}
	if eff := g.EffectsSummary(); eff != "" {
		s += "   [" + eff + "]"
	}
	return s
}

// wornAccessories joins the display names of the worn ring and amulet, returning
// "" when neither slot is filled (so the HUD omits the segment entirely).
func wornAccessories(g *game.Game) string {
	var names []string
	for _, it := range []*game.Item{g.Ring, g.Amulet, g.Boots, g.Head, g.Cloak} {
		if it != nil && it.Def != nil {
			names = append(names, g.DisplayName(it))
		}
	}
	return strings.Join(names, ", ")
}

// lastN returns up to the last n elements of s.
func lastN(s []string, n int) []string {
	if len(s) <= n {
		return s
	}
	return s[len(s)-n:]
}

// Run is the core game loop: recompute FOV, render, get an action, apply it.
func Run(g *game.Game, p Prompter, r Renderer) error {
	for {
		g.UpdateFOV()
		r.Render(BuildView(g))
		if g.Dead || g.Won {
			return nil
		}
		a, err := p.NextAction()
		if err != nil {
			return err
		}
		switch a.Kind {
		case ActQuit:
			return nil
		case ActMove:
			g.PlayerStep(a.Dir)
			if pos, ok := g.TakeLockedBump(); ok {
				promptLockedDoor(g, p, pos)
			}
		case ActPickup:
			g.PlayerPickup()
		case ActInventory:
			if len(g.Inventory) > 0 {
				names := make([]string, len(g.Inventory))
				for i, it := range g.Inventory {
					names[i] = g.DisplayName(it)
				}
				if idx, ok := p.Menu(MenuSpec{Title: "Inventory", Items: names}); ok && idx >= 0 && idx < len(g.Inventory) {
					g.PlayerUse(g.Inventory[idx])
				}
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
			names := make([]string, len(food))
			for i, it := range food {
				names[i] = g.DisplayName(it)
			}
			if idx, ok := p.Menu(MenuSpec{Title: "Eat what?", Items: names}); ok && idx >= 0 && idx < len(food) {
				g.PlayerUse(food[idx])
			}
		case ActRead:
			scrolls := g.ReadableInventory()
			if len(scrolls) == 0 {
				g.Messages = append(g.Messages, "You have nothing to read.")
				break
			}
			names := make([]string, len(scrolls))
			for i, it := range scrolls {
				names[i] = g.DisplayName(it)
			}
			if idx, ok := p.Menu(MenuSpec{Title: "Read what?", Items: names}); ok && idx >= 0 && idx < len(scrolls) {
				g.PlayerUse(scrolls[idx])
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
			if spell.Def != nil && (spell.Def.Ranged > 0 || spell.Def.Breath != "") { // a targeted or directed spell
				if target, ok := p.Target(g.Player); ok {
					g.CastSpellAt(spell, target)
				}
			} else {
				g.CastSpell(spell)
			}
		}
	}
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
