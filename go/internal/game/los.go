package game

// lineOfSight reports whether b is visible from a along a straight line: it
// returns false if any tile strictly between a and b is opaque (a wall or a
// closed door). The endpoints themselves are never treated as blockers. Uses a
// Bresenham walk, matching the FOV's transparency model.
func (g *Game) lineOfSight(a, b Pos) bool {
	dx := b.X - a.X
	if dx < 0 {
		dx = -dx
	}
	dy := b.Y - a.Y
	if dy < 0 {
		dy = -dy
	}
	sx, sy := 1, 1
	if a.X > b.X {
		sx = -1
	}
	if a.Y > b.Y {
		sy = -1
	}
	err := dx - dy
	x, y := a.X, a.Y
	for {
		if x == b.X && y == b.Y {
			return true
		}
		e2 := 2 * err
		if e2 > -dy {
			err -= dy
			x += sx
		}
		if e2 < dx {
			err += dx
			y += sy
		}
		if x == b.X && y == b.Y {
			return true // reached the target; its own tile isn't a blocker
		}
		if !g.Level.InBounds(Pos{X: x, Y: y}) || !g.Level.At(Pos{X: x, Y: y}).Def.Transparent {
			return false
		}
	}
}
