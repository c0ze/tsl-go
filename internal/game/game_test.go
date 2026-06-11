package game

import (
	"testing"

	"github.com/c0ze/tsl-go/internal/content"
)

func testContent() *content.Content {
	return &content.Content{Tiles: map[string]*content.TileDef{
		"floor": {ID: "floor", Glyph: ".", Color: content.ColorNormal, Passable: true, Transparent: true},
		"wall":  {ID: "wall", Glyph: "#", Color: content.ColorNormal, Passable: false, Transparent: false},
	}}
}

func TestParseLevel(t *testing.T) {
	c := testContent()
	rows := []string{
		"###",
		"#@#",
		"###",
	}
	legend := map[rune]string{'#': "wall", '@': "floor"}
	lvl, start, err := ParseLevel(c, rows, legend)
	if err != nil {
		t.Fatalf("ParseLevel: %v", err)
	}
	if lvl.W != 3 || lvl.H != 3 {
		t.Fatalf("size = %dx%d, want 3x3", lvl.W, lvl.H)
	}
	if start != (Pos{1, 1}) {
		t.Errorf("start = %v, want {1 1}", start)
	}
	if lvl.Passable(Pos{0, 0}) {
		t.Error("corner wall should be impassable")
	}
	if !lvl.Passable(Pos{1, 1}) {
		t.Error("center floor should be passable")
	}
}

func TestMoveIntoFloorSucceeds(t *testing.T) {
	c := testContent()
	lvl, start, _ := ParseLevel(c, []string{"...", ".@.", "..."}, map[rune]string{'.': "floor", '@': "floor"})
	g := &Game{Content: c, Level: lvl, Player: start}
	if !g.Move(DirE) {
		t.Fatal("move east into floor should succeed")
	}
	if g.Player != (Pos{2, 1}) {
		t.Errorf("player = %v, want {2 1}", g.Player)
	}
}

func TestMoveIntoWallBlocked(t *testing.T) {
	c := testContent()
	lvl, start, _ := ParseLevel(c, []string{"###", "#@#", "###"}, map[rune]string{'#': "wall", '@': "floor"})
	g := &Game{Content: c, Level: lvl, Player: start}
	if g.Move(DirN) {
		t.Fatal("move into wall should be blocked")
	}
	if g.Player != (Pos{1, 1}) {
		t.Errorf("player moved to %v, want {1 1}", g.Player)
	}
}

func TestMoveOutOfBoundsBlocked(t *testing.T) {
	c := testContent()
	lvl, start, _ := ParseLevel(c, []string{"@."}, map[rune]string{'.': "floor", '@': "floor"})
	g := &Game{Content: c, Level: lvl, Player: start}
	if g.Move(DirW) {
		t.Fatal("move off the west edge should be blocked")
	}
}

func TestDirectionDelta(t *testing.T) {
	dx, dy := DirNE.Delta()
	if dx != 1 || dy != -1 {
		t.Errorf("DirNE delta = (%d,%d), want (1,-1)", dx, dy)
	}
}

func TestParseLevelRequiresStart(t *testing.T) {
	c := testContent()
	_, _, err := ParseLevel(c, []string{"..", ".."}, map[rune]string{'.': "floor"})
	if err == nil {
		t.Fatal("expected error when no '@' start marker is present, got nil")
	}
}
