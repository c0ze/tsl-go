package ui

import (
	"math"

	"github.com/c0ze/tsl-go/internal/content"
	"github.com/c0ze/tsl-go/internal/game"
)

// wallRune holds the box-drawing glyph for each 4-bit mask of orthogonally
// adjacent walls (bit 1=N, 2=E, 4=S, 8=W). It turns the flat '#' grid into
// connected walls the way Brogue and DCSS draw them; a wall with no known
// neighbours (e.g. a lone cell at the edge of sight) shows as a solid block
// rather than a glyph that could be mistaken for a creature or item.
var wallRune = [16]rune{
	'■', '│', '─', '└', '│', '│', '┌', '├',
	'─', '┘', '─', '┴', '┐', '┤', '┬', '┼',
}

// displayGlyph maps a tile's raw glyph to its decorated form: deep water
// becomes a ripple, '#' walls become connected box-drawing, '.' floor becomes a
// centred dot, and everything else is drawn as authored.
func displayGlyph(l *game.Level, x, y int, def *content.TileDef) rune {
	switch {
	case def.Water:
		return '≈'
	case def.Rune() == '#':
		return wallRune[wallMask(l, x, y)]
	case def.Rune() == '.':
		return '·'
	default:
		return def.Rune()
	}
}

// wallMask builds the 4-bit neighbour mask for wall autotiling, connecting only
// to walls the player already knows (visible or remembered) so the decoration
// never leaks the shape of unseen rock.
func wallMask(l *game.Level, x, y int) int {
	m := 0
	if knownWall(l, x, y-1) {
		m |= 1
	}
	if knownWall(l, x+1, y) {
		m |= 2
	}
	if knownWall(l, x, y+1) {
		m |= 4
	}
	if knownWall(l, x-1, y) {
		m |= 8
	}
	return m
}

// knownWall reports whether (x,y) is a wall the player has seen — the join
// condition for autotiling.
func knownWall(l *game.Level, x, y int) bool {
	p := game.Pos{X: x, Y: y}
	if !l.InBounds(p) {
		return false
	}
	t := l.At(p)
	return (t.Visible || t.Seen) && t.Appears().Rune() == '#'
}

// tileLight returns a 0..1 brightness for a visible tile: full (1) at the
// player's feet, falling off with the square of distance to a floor of 0.2 at
// the edge of sight — the pool-of-torchlight look.
func tileLight(px, py, x, y, radius int) float64 {
	if radius < 1 {
		radius = 1
	}
	t := math.Hypot(float64(x-px), float64(y-py)) / float64(radius)
	if t > 1 {
		t = 1
	}
	return 1 - 0.8*t*t
}
