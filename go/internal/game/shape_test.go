package game

import (
	"bytes"
	"testing"

	"github.com/c0ze/tsl/internal/content"
)

func wolfForm() *content.MonsterDef {
	return &content.MonsterDef{ID: "dire_wolf", Name: "dire wolf", Glyph: "d", HP: 10, Attack: 9, Dodge: 4, Damage: "2d3", Speed: 50}
}

func TestShapedStatsGovern(t *testing.T) {
	g := combatGame()
	g.Weapon = &Item{Def: &content.ItemDef{Name: "doomblade", Kind: "weapon", Attack: 5, Damage: "2d6"}}
	g.Shape = wolfForm()
	if got := g.playerDamageSpec(); got != "2d3" {
		t.Errorf("claws, not the blade: damage spec %q", got)
	}
	if g.playerAttackStat() != 9 || g.playerDodgeStat() != 4 {
		t.Errorf("the form's combat stats govern, got %d/%d", g.playerAttackStat(), g.playerDodgeStat())
	}
}

func TestShapedSpeedGoverns(t *testing.T) {
	g, rat := schedulerGame()
	g.Shape = wolfForm() // speed 50: the world ticks twice per action
	g.advanceWorld()
	if rat.Pos.X != 6 {
		t.Errorf("a slow form drags like a slow effect: rat at x=%d, want 6", rat.Pos.X)
	}
}

func TestSwimmingFormCrossesWater(t *testing.T) {
	g := waterGame()
	g.Shape = &content.MonsterDef{ID: "merman", Name: "merman", Glyph: "M", HP: 8, Attack: 4, Dodge: 2, Damage: "1d4", Speed: 100, Swim: true}
	g.PlayerStep(DirE) // into the pool, fins first
	if g.Player != (Pos{2, 1}) {
		t.Fatalf("a swimming form enters water (C move_creature), at %v", g.Player)
	}
	g.advanceWorld()
	if g.PlayerHP != 20 {
		t.Errorf("a swimming form never drowns (C swim() free_swim), HP %d", g.PlayerHP)
	}
}

func TestPolymorphExpiryReverts(t *testing.T) {
	g := combatGame()
	g.Shape = wolfForm()
	g.AddEffect("polymorph", 2)
	g.advanceWorld()
	g.advanceWorld()
	if g.Shape != nil {
		t.Fatal("the form should expire (C SHAPESHIFT_DURATION)")
	}
	if !hasMessage(g, "You return to your native shape.") {
		t.Errorf("expected the C revert line, got %v", g.Messages)
	}
}

func TestShapeSurvivesTheSave(t *testing.T) {
	g := savedWorld(t)
	g.Content.Monsters["wolf"] = wolfForm()
	g.Content.Monsters["wolf"].ID = "wolf"
	g.Shape = g.Content.Monsters["wolf"]
	g.AddEffect("polymorph", 40)
	var buf bytes.Buffer
	if err := g.Save(&buf); err != nil {
		t.Fatal(err)
	}
	g2, err := LoadGame(&buf, g.Content, g.Behaviors, nil)
	if err != nil {
		t.Fatal(err)
	}
	if g2.Shape == nil || g2.Shape.ID != "wolf" {
		t.Errorf("the form should survive the save, got %+v", g2.Shape)
	}
}
