// Package fov computes field of view via recursive shadowcasting. It depends on
// nothing but the caller-supplied Grid, so it is a pure leaf package.
package fov

// Grid is the read-only map view FOV needs.
type Grid interface {
	InBounds(x, y int) bool
	Opaque(x, y int) bool // does (x,y) block sight?
}

// multipliers for transforming coordinates into the 8 octants.
var mult = [4][8]int{
	{1, 0, 0, -1, -1, 0, 0, 1},
	{0, 1, -1, 0, 0, -1, 1, 0},
	{0, 1, 1, 0, 0, -1, -1, 0},
	{1, 0, 0, 1, -1, 0, 0, -1},
}

// Compute calls visit(x,y) once for every tile visible from (ox,oy) within
// radius (Euclidean), including the origin and any opaque tiles that are seen.
func Compute(g Grid, ox, oy, radius int, visit func(x, y int)) {
	if visit == nil || radius < 0 {
		return
	}
	visit(ox, oy)
	for oct := 0; oct < 8; oct++ {
		castLight(g, ox, oy, radius, 1, 1.0, 0.0,
			mult[0][oct], mult[1][oct], mult[2][oct], mult[3][oct], visit)
	}
}

func castLight(g Grid, cx, cy, radius, row int, start, end float64, xx, xy, yx, yy int, visit func(x, y int)) {
	if start < end {
		return
	}
	radiusSq := radius * radius
	newStart := 0.0
	blocked := false
	for j := row; j <= radius && !blocked; j++ {
		dy := -j
		for dx := -j; dx <= 0; dx++ {
			lSlope := (float64(dx) - 0.5) / (float64(dy) + 0.5)
			rSlope := (float64(dx) + 0.5) / (float64(dy) - 0.5)
			if start < rSlope {
				continue
			}
			if end > lSlope {
				break
			}
			mx := cx + dx*xx + dy*xy
			my := cy + dx*yx + dy*yy
			inBounds := g.InBounds(mx, my)
			if dx*dx+dy*dy <= radiusSq && inBounds {
				visit(mx, my)
			}
			// Out-of-bounds counts as opaque so light can't leak past map edges.
			opaque := !inBounds || g.Opaque(mx, my)
			if blocked {
				if opaque {
					newStart = rSlope
					continue
				}
				blocked = false
				start = newStart
			} else if opaque && j < radius {
				blocked = true
				castLight(g, cx, cy, radius, j+1, start, lSlope, xx, xy, yx, yy, visit)
				newStart = rSlope
			}
		}
	}
}
