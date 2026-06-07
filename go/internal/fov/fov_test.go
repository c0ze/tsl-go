package fov

import "testing"

type testGrid struct {
	w, h   int
	opaque map[[2]int]bool
}

func (g testGrid) InBounds(x, y int) bool { return x >= 0 && y >= 0 && x < g.w && y < g.h }
func (g testGrid) Opaque(x, y int) bool   { return g.opaque[[2]int{x, y}] }

func collect(g Grid, ox, oy, r int) map[[2]int]bool {
	seen := map[[2]int]bool{}
	Compute(g, ox, oy, r, func(x, y int) { seen[[2]int{x, y}] = true })
	return seen
}

func TestComputeOriginAlwaysVisible(t *testing.T) {
	seen := collect(testGrid{w: 5, h: 5}, 2, 2, 3)
	if !seen[[2]int{2, 2}] {
		t.Error("origin must be visible")
	}
}

func TestComputeOpenFieldDisk(t *testing.T) {
	seen := collect(testGrid{w: 21, h: 21}, 10, 10, 4)
	if !seen[[2]int{14, 10}] {
		t.Error("tile at radius 4 should be visible")
	}
	if seen[[2]int{16, 10}] {
		t.Error("tile beyond radius should not be visible")
	}
}

func TestComputeWallColumnBlocks(t *testing.T) {
	g := testGrid{w: 11, h: 3, opaque: map[[2]int]bool{
		{5, 0}: true, {5, 1}: true, {5, 2}: true,
	}}
	seen := collect(g, 1, 1, 8)
	if !seen[[2]int{5, 1}] {
		t.Error("the wall itself should be visible")
	}
	if seen[[2]int{9, 1}] {
		t.Error("tile behind the wall column should be blocked")
	}
}
