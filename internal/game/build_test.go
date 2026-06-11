package game

import (
	"testing"

	"github.com/c0ze/tsl-go/internal/content"
)

func TestNewLevelFills(t *testing.T) {
	wall := &content.TileDef{ID: "wall", Glyph: "#", Color: content.ColorNormal}
	l := NewLevel(4, 3, wall)
	if l.W != 4 || l.H != 3 {
		t.Fatalf("size = %dx%d, want 4x3", l.W, l.H)
	}
	for y := 0; y < l.H; y++ {
		for x := 0; x < l.W; x++ {
			if l.At(Pos{x, y}).Def != wall {
				t.Fatalf("tile (%d,%d) not filled with wall", x, y)
			}
		}
	}
}

func TestSetReplacesTile(t *testing.T) {
	wall := &content.TileDef{ID: "wall", Glyph: "#", Passable: false}
	floor := &content.TileDef{ID: "floor", Glyph: ".", Passable: true}
	l := NewLevel(3, 3, wall)
	l.Set(Pos{1, 1}, floor)
	if l.At(Pos{1, 1}).Def != floor {
		t.Error("Set did not replace the tile def")
	}
	if !l.Passable(Pos{1, 1}) {
		t.Error("center should be passable after Set(floor)")
	}
	if l.Passable(Pos{0, 0}) {
		t.Error("corner should still be wall")
	}
}
