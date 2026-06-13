package ui

import (
	"testing"

	"github.com/c0ze/tsl-go/internal/content"
	"github.com/c0ze/tsl-go/internal/game"
)

// decorLevel parses an ASCII map (legend: '.' floor, '#' wall, '~' water, '@'
// start over floor) and marks every tile visible, so displayGlyph autotiles
// against the whole grid.
func decorLevel(t *testing.T, rows []string) *game.Level {
	t.Helper()
	c := &content.Content{Tiles: map[string]*content.TileDef{
		"floor": {ID: "floor", Glyph: ".", Color: content.ColorNormal, Passable: true, Transparent: true},
		"wall":  {ID: "wall", Glyph: "#", Color: content.ColorNormal},
		"water": {ID: "water", Glyph: "_", Color: content.ColorBlue, Transparent: true, Water: true},
	}}
	lvl, _, err := game.ParseLevel(c, rows, map[rune]string{'.': "floor", '#': "wall", '~': "water", '@': "floor"})
	if err != nil {
		t.Fatal(err)
	}
	for y := 0; y < lvl.H; y++ {
		for x := 0; x < lvl.W; x++ {
			lvl.At(game.Pos{X: x, Y: y}).Visible = true
		}
	}
	return lvl
}

func glyphAt(l *game.Level, x, y int) rune {
	return displayGlyph(l, x, y, l.At(game.Pos{X: x, Y: y}).Appears())
}

func TestWallAutotilingJunction(t *testing.T) {
	l := decorLevel(t, []string{
		"@#.",
		"###",
		".#.",
	})
	if g := glyphAt(l, 1, 1); g != '┼' {
		t.Errorf("centre of a + of walls = %q, want ┼", g)
	}
	if g := glyphAt(l, 1, 0); g != '│' {
		t.Errorf("wall with only a southern neighbour = %q, want │", g)
	}
	if g := glyphAt(l, 0, 1); g != '─' {
		t.Errorf("wall with only an eastern neighbour = %q, want ─", g)
	}
}

func TestWallAutotilingCornersAndFloor(t *testing.T) {
	l := decorLevel(t, []string{
		"###",
		"#@#",
		"###",
	})
	for _, tc := range []struct {
		x, y int
		want rune
		name string
	}{
		{0, 0, '┌', "NW corner"},
		{2, 0, '┐', "NE corner"},
		{0, 2, '└', "SW corner"},
		{2, 2, '┘', "SE corner"},
		{1, 0, '─', "top edge"},
		{0, 1, '│', "left edge"},
		{1, 1, '·', "interior floor"},
	} {
		if g := glyphAt(l, tc.x, tc.y); g != tc.want {
			t.Errorf("%s = %q, want %q", tc.name, g, tc.want)
		}
	}
}

func TestWaterGlyph(t *testing.T) {
	l := decorLevel(t, []string{"@~."})
	if g := glyphAt(l, 1, 0); g != '≈' {
		t.Errorf("deep water = %q, want ≈", g)
	}
}

// Autotiling must not connect to a wall the player has not seen, or the
// decoration would leak the shape of unexplored rock.
func TestWallAutotilingIgnoresUnseenNeighbours(t *testing.T) {
	l := decorLevel(t, []string{
		"@#.",
		"###",
		".#.",
	})
	l.At(game.Pos{X: 1, Y: 0}).Visible = false
	l.At(game.Pos{X: 1, Y: 0}).Seen = false // hide the north arm
	if g := glyphAt(l, 1, 1); g != '┬' {
		t.Errorf("centre wall with an unseen north arm = %q, want ┬", g)
	}
}

func TestTileLightFallsOff(t *testing.T) {
	center := tileLight(5, 5, 5, 5, 8)
	near := tileLight(5, 5, 6, 5, 8)
	edge := tileLight(5, 5, 13, 5, 8) // distance 8 == radius
	if center != 1 {
		t.Errorf("light at the player's feet = %v, want 1", center)
	}
	if !(center > near && near > edge) {
		t.Errorf("light should fall off with distance: centre %v, near %v, edge %v", center, near, edge)
	}
	if edge < 0.15 || edge > 0.25 {
		t.Errorf("edge light = %v, want the ~0.2 falloff floor", edge)
	}
}

func TestLightingDarkensWithDistanceAndMemory(t *testing.T) {
	base := ColorRGB(content.ColorNormal)
	bright := Lit(base, 1)
	dim := Lit(base, 0.2)
	if bright.R <= dim.R || bright.G <= dim.G {
		t.Errorf("a fully lit tile should outshine a dim one: %+v vs %+v", bright, dim)
	}
	if r := Remembered(base); r.R >= base.R || r.G >= base.G {
		t.Errorf("a remembered tile should be darker than its base hue: %+v vs %+v", r, base)
	}
}

func TestBuildViewLightsVisibleTiles(t *testing.T) {
	g := testGame(t, []string{".....", ".@...", "....."})
	g.UpdateFOV()
	v := BuildView(g)
	if c := v.At(1, 1); c.Light <= 0 {
		t.Errorf("player tile light = %v, want > 0", c.Light)
	}
	if near, far := v.At(2, 1).Light, v.At(4, 1).Light; near <= far {
		t.Errorf("light should fall off across the room: near %v, far %v", near, far)
	}
}
