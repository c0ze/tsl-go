// Package ui defines the rendering/input boundary between the engine and any
// front-end. The engine never imports a concrete UI; front-ends implement
// Prompter and Renderer.
package ui

import (
	"github.com/c0ze/tsl/internal/content"
	"github.com/c0ze/tsl/internal/game"
)

// Cell is one rendered grid cell.
type Cell struct {
	Glyph rune
	Color content.Color
}

// View is a read-only snapshot the front-end draws.
type View struct {
	W, H  int
	Cells []Cell // len W*H, row-major
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
)

// Action is a decoded player intent. Dir is meaningful only when Kind==ActMove.
type Action struct {
	Kind ActionKind
	Dir  game.Direction
}

// Prompter supplies player actions to the game loop.
type Prompter interface {
	NextAction() (Action, error)
}

// Renderer draws a View.
type Renderer interface {
	Render(View)
}

// Player avatar rendering (Plan 1; later the player becomes a creature def).
const PlayerGlyph = '@'

// PlayerColor is the player's glyph color.
const PlayerColor = content.ColorNormal

// BuildView produces the View for the current game state.
func BuildView(g *game.Game) View {
	l := g.Level
	v := View{W: l.W, H: l.H, Cells: make([]Cell, l.W*l.H)}
	for y := 0; y < l.H; y++ {
		for x := 0; x < l.W; x++ {
			def := l.At(game.Pos{X: x, Y: y}).Def
			*v.At(x, y) = Cell{Glyph: def.Rune(), Color: def.Color}
		}
	}
	*v.At(g.Player.X, g.Player.Y) = Cell{Glyph: PlayerGlyph, Color: PlayerColor}
	return v
}

// Run is the core game loop: render, get an action, apply it, until quit.
func Run(g *game.Game, p Prompter, r Renderer) error {
	for {
		r.Render(BuildView(g))
		a, err := p.NextAction()
		if err != nil {
			return err
		}
		switch a.Kind {
		case ActQuit:
			return nil
		case ActMove:
			g.Move(a.Dir)
		}
	}
}
