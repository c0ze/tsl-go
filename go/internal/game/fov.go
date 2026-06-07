package game

import "github.com/c0ze/tsl/internal/fov"

// VisionRadius is how far the player can see, in tiles. It becomes
// attribute-driven once creatures land in a later plan.
const VisionRadius = 8

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
	fov.Compute(fovGrid{g.Level}, g.Player.X, g.Player.Y, VisionRadius, func(x, y int) {
		t := g.Level.At(Pos{X: x, Y: y})
		t.Visible = true
		t.Seen = true
	})
}
