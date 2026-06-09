package game

import "testing"

func TestDarkLevelShrinksVision(t *testing.T) {
	g := combatGame() // 10x3 all floor, player at (1,1)
	g.UpdateFOV()
	if !g.Level.At(Pos{7, 1}).Visible {
		t.Fatal("on a lit level a tile 6 away should be visible")
	}

	g.Level.Dark = true
	g.UpdateFOV()

	if g.Level.At(Pos{7, 1}).Visible {
		t.Error("on a dark level a tile 6 away should be out of sight")
	}
	if !g.Level.At(Pos{3, 1}).Visible {
		t.Error("on a dark level a near tile (2 away) should still be visible")
	}
}

func TestVisionRadiusDarkVsLit(t *testing.T) {
	g := combatGame()
	if r := g.visionRadius(); r != VisionRadius {
		t.Errorf("lit visionRadius = %d, want %d", r, VisionRadius)
	}
	g.Level.Dark = true
	if r := g.visionRadius(); r != DarkVisionRadius {
		t.Errorf("dark visionRadius = %d, want %d", r, DarkVisionRadius)
	}
}
