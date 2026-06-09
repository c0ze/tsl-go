// Package game holds the core world model and turn logic. It performs no I/O
// and never imports a UI.
package game

import (
	"github.com/c0ze/tsl/internal/content"
	"github.com/c0ze/tsl/internal/rng"
)

// Pos is a grid coordinate.
type Pos struct{ X, Y int }

// Direction is one of the eight compass directions (or none).
type Direction int

const (
	DirNone Direction = iota
	DirN
	DirNE
	DirE
	DirSE
	DirS
	DirSW
	DirW
	DirNW
)

// Delta returns the (dx, dy) step for a direction.
func (d Direction) Delta() (int, int) {
	switch d {
	case DirN:
		return 0, -1
	case DirNE:
		return 1, -1
	case DirE:
		return 1, 0
	case DirSE:
		return 1, 1
	case DirS:
		return 0, 1
	case DirSW:
		return -1, 1
	case DirW:
		return -1, 0
	case DirNW:
		return -1, -1
	default:
		return 0, 0
	}
}

// Tile is a single map cell. Visible is true when currently in the player's
// FOV; Seen is true once it has ever been visible (remembered/dimmed).
type Tile struct {
	Def     *content.TileDef
	Visible bool
	Seen    bool
}

// Level is a rectangular grid of tiles stored row-major.
type Level struct {
	W, H      int
	tiles     []Tile
	Creatures []*Creature
	Items     []*Item

	ID      string   // level id (set by the Dungeon on generation)
	Start   Pos      // first-arrival spawn position
	Return  Pos      // saved player position for re-entry (set on leaving)
	entered bool     // whether the player has arrived here at least once
	Portals []Portal // stairs leading to other levels
	Dark    bool     // unlit: the player sees only a small radius
}

// InBounds reports whether p lies inside the level.
func (l *Level) InBounds(p Pos) bool {
	return p.X >= 0 && p.Y >= 0 && p.X < l.W && p.Y < l.H
}

// At returns the tile at p, which must be in bounds.
func (l *Level) At(p Pos) *Tile {
	return &l.tiles[p.Y*l.W+p.X]
}

// Passable reports whether p is in bounds and its tile can be walked into.
func (l *Level) Passable(p Pos) bool {
	return l.InBounds(p) && l.At(p).Def.Passable
}

// Game is the whole game state.
type Game struct {
	Content     *content.Content
	Level       *Level
	Dungeon     *Dungeon
	Player      Pos
	RNG         *rng.MT
	PlayerHP    int
	PlayerMax   int
	EP          int // energy points: the spellcasting resource
	EPMax       int
	epTurn      int // counter for slow EP regeneration
	Messages    []string
	Dead        bool
	Inventory   []*Item
	Weapon      *Item
	Armor       *Item
	Ring        *Item // worn accessory: passive attack/dodge bonus
	Amulet      *Item // worn accessory: passive attack/dodge bonus
	Behaviors   map[string]Behavior
	Won         bool
	DeathCause  string
	Effects     []Effect
	Identified  map[string]bool   // item ids whose type is globally known this game
	appearances map[string]string // item id -> shuffled cosmetic name while unidentified
}

// Move attempts to move the player one step in d. It returns true if the player
// moved, false if the target was blocked or out of bounds.
func (g *Game) Move(d Direction) bool {
	dx, dy := d.Delta()
	dst := Pos{g.Player.X + dx, g.Player.Y + dy}
	if !g.Level.Passable(dst) {
		return false
	}
	g.Player = dst
	return true
}
