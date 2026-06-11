package game

import (
	"testing"

	"github.com/c0ze/tsl-go/internal/content"
)

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

func TestCarriedTorchLightsTheDark(t *testing.T) {
	g := combatGame() // 10x3 all floor, player at (1,1)
	g.Level.Dark = true
	g.UpdateFOV()
	if g.Level.At(Pos{6, 1}).Visible {
		t.Fatal("on a dark level a tile 5 away should be out of sight without a torch")
	}

	g.Inventory = append(g.Inventory, &Item{Def: &content.ItemDef{Name: "torch", Kind: "light", Light: 6}})
	g.UpdateFOV()

	if !g.Level.At(Pos{6, 1}).Visible {
		t.Error("carrying a torch (light 6) should reveal a tile 5 away in the dark")
	}
}

func TestBlindnessOverridesVision(t *testing.T) {
	g := combatGame()
	g.Level.Dark = true
	g.Inventory = append(g.Inventory, &Item{Def: &content.ItemDef{Name: "torch", Kind: "light", Light: 8}})
	if r := g.visionRadius(); r != 8 { // sanity: a bright torch lights the dark
		t.Fatalf("torch should give radius 8, got %d", r)
	}
	g.AddEffect("blind", 5)
	if r := g.visionRadius(); r != BlindVisionRadius {
		t.Errorf("a blind player should see only %d tiles, got %d", BlindVisionRadius, r)
	}
}

func TestPlayerHasEffect(t *testing.T) {
	g := &Game{}
	if g.HasEffect("blind") {
		t.Error("no effect should be present initially")
	}
	g.AddEffect("blind", 3)
	if !g.HasEffect("blind") {
		t.Error("HasEffect should report the active blind effect")
	}
}

func TestTorchNoEffectWhenLit(t *testing.T) {
	g := combatGame() // lit
	g.Inventory = append(g.Inventory, &Item{Def: &content.ItemDef{Name: "torch", Kind: "light", Light: 99}})
	if r := g.visionRadius(); r != VisionRadius {
		t.Errorf("a torch should not extend sight on a lit level: got %d, want %d", r, VisionRadius)
	}
}
