package game

import "github.com/c0ze/tsl/internal/fov"

// VisionRadius is how far the player can see on a normally-lit level, in tiles.
// DarkVisionRadius is the reduced reach on an unlit (dark) level.
const (
	VisionRadius      = 8
	DarkVisionRadius  = 3
	BlindVisionRadius = 1
)

// visionRadius is the player's current sight radius. Blindness collapses it to a
// single tile (overriding any light); otherwise a dark level shrinks it to
// DarkVisionRadius, which a carried light source (a torch) pushes back out.
func (g *Game) visionRadius() int {
	if g.HasEffect("blind") {
		return BlindVisionRadius
	}
	if g.Level == nil || !g.Level.Dark {
		return VisionRadius
	}
	r := DarkVisionRadius
	if light := g.carriedLight(); light > r {
		r = light
	}
	return r
}

// carriedLight returns the largest light radius among the items the player is
// carrying — a torch glows passively while held.
func (g *Game) carriedLight() int {
	best := 0
	for _, it := range g.Inventory {
		if it.Def != nil && it.Def.Light > best {
			best = it.Def.Light
		}
	}
	return best
}

// fovGrid adapts a Level to fov.Grid.
type fovGrid struct{ l *Level }

func (f fovGrid) InBounds(x, y int) bool { return f.l.InBounds(Pos{X: x, Y: y}) }

func (f fovGrid) Opaque(x, y int) bool {
	p := Pos{X: x, Y: y}
	return !f.l.InBounds(p) || !f.l.At(p).Def.Transparent
}

// UpdateFOV recomputes which tiles are currently visible from the player and
// marks them Seen. Tiles outside the new FOV are cleared to not-Visible but
// keep their Seen state (so they render dimmed from memory).
func (g *Game) UpdateFOV() {
	for i := range g.Level.tiles {
		g.Level.tiles[i].Visible = false
	}
	fov.Compute(fovGrid{g.Level}, g.Player.X, g.Player.Y, g.visionRadius(), func(x, y int) {
		t := g.Level.At(Pos{X: x, Y: y})
		t.Visible = true
		t.Seen = true
	})
}
