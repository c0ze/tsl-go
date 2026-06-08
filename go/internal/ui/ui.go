// Package ui defines the rendering/input boundary between the engine and any
// front-end. The engine never imports a concrete UI; front-ends implement
// Prompter and Renderer.
package ui

import (
	"fmt"

	"github.com/c0ze/tsl/internal/content"
	"github.com/c0ze/tsl/internal/game"
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
			switch {
			case t.Visible:
				*v.At(x, y) = Cell{Glyph: t.Def.Rune(), Color: t.Def.Color}
			case t.Seen:
				*v.At(x, y) = Cell{Glyph: t.Def.Rune(), Color: t.Def.Color, Dim: true}
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
		if l.InBounds(m.Pos) && l.At(m.Pos).Visible {
			*v.At(m.Pos.X, m.Pos.Y) = Cell{Glyph: m.Def.Rune(), Color: m.Def.Color}
		}
	}
	if l.InBounds(g.Player) {
		*v.At(g.Player.X, g.Player.Y) = Cell{Glyph: PlayerGlyph, Color: PlayerColor}
	}
	v.Status = statusLine(g)
	v.Messages = lastN(g.Messages, 4)
	return v
}

// statusLine summarises the player's vitals and gear for the HUD.
func statusLine(g *game.Game) string {
	wield, wear := "none", "none"
	if g.Weapon != nil && g.Weapon.Def != nil {
		wield = g.Weapon.Def.Name
	}
	if g.Armor != nil && g.Armor.Def != nil {
		wear = g.Armor.Def.Name
	}
	loc := g.LocationName()
	if loc == "" {
		loc = "the dungeon"
	}
	s := fmt.Sprintf("HP %d/%d   %s   Wield: %s   Wear: %s",
		g.PlayerHP, g.PlayerMax, loc, wield, wear)
	if eff := g.EffectsSummary(); eff != "" {
		s += "   [" + eff + "]"
	}
	return s
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
		case ActPickup:
			g.PlayerPickup()
		case ActInventory:
			if len(g.Inventory) > 0 {
				names := make([]string, len(g.Inventory))
				for i, it := range g.Inventory {
					names[i] = it.Def.Name
				}
				if idx, ok := p.Menu(MenuSpec{Title: "Inventory", Items: names}); ok && idx >= 0 && idx < len(g.Inventory) {
					g.PlayerUse(g.Inventory[idx])
				}
			}
		case ActTravel:
			g.Travel()
		case ActEat:
			food := g.EdibleInventory()
			if len(food) == 0 {
				g.Messages = append(g.Messages, "You have nothing to eat.")
				break
			}
			names := make([]string, len(food))
			for i, it := range food {
				names[i] = it.Def.Name
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
				names[i] = it.Def.Name
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
				names[i] = fmt.Sprintf("%s (%d charges)", it.Def.Name, it.Charges)
			}
			idx, ok := p.Menu(MenuSpec{Title: "Zap which wand?", Items: names})
			if !ok || idx < 0 || idx >= len(wands) {
				break
			}
			if target, ok := p.Target(g.Player); ok {
				g.ZapWand(wands[idx], target)
			}
		}
	}
}
