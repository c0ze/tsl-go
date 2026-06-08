package behaviors

import (
	"testing"

	"github.com/c0ze/tsl/internal/content"
	"github.com/c0ze/tsl/internal/game"
)

func TestHealCapsAtMax(t *testing.T) {
	reg := Registry()
	heal, ok := reg["heal"]
	if !ok {
		t.Fatal("registry missing heal")
	}
	g := &game.Game{PlayerHP: 18, PlayerMax: 20}
	it := &game.Item{Def: &content.ItemDef{Name: "healing potion", Power: 8}}
	msgs := heal(g, it)
	if g.PlayerHP != 20 {
		t.Errorf("HP = %d, want capped at 20", g.PlayerHP)
	}
	if len(msgs) == 0 {
		t.Error("heal should return a message")
	}
}

func TestEatRestoresHPClamped(t *testing.T) {
	reg := Registry()
	eat, ok := reg["eat"]
	if !ok {
		t.Fatal("eat behavior not registered")
	}
	g := &game.Game{PlayerHP: 18, PlayerMax: 20}
	it := &game.Item{Def: &content.ItemDef{Name: "rat corpse", Power: 5}}
	msgs := eat(g, it)
	if g.PlayerHP != 20 {
		t.Errorf("HP = %d, want 20 (clamped)", g.PlayerHP)
	}
	if len(msgs) == 0 {
		t.Error("expected an eat message")
	}
}

func TestRegenerateAddsEffect(t *testing.T) {
	regen, ok := Registry()["regenerate"]
	if !ok {
		t.Fatal("regenerate behavior not registered")
	}
	g := &game.Game{PlayerHP: 10, PlayerMax: 20}
	regen(g, &game.Item{Def: &content.ItemDef{Name: "potion of regeneration", Power: 8}})
	if len(g.Effects) != 1 || g.Effects[0].Kind != "regen" || g.Effects[0].Turns != 8 {
		t.Errorf("expected a regen effect for 8 turns, got %v", g.Effects)
	}
}

func TestEatMushroomHealsAndPoisons(t *testing.T) {
	eat, ok := Registry()["eat_mushroom"]
	if !ok {
		t.Fatal("eat_mushroom behavior not registered")
	}
	g := &game.Game{PlayerHP: 10, PlayerMax: 20}
	eat(g, &game.Item{Def: &content.ItemDef{Name: "red mushroom", Power: 4}})
	if g.PlayerHP != 14 {
		t.Errorf("HP = %d, want 14 (healed 4)", g.PlayerHP)
	}
	poisoned := false
	for _, e := range g.Effects {
		if e.Kind == "poison" {
			poisoned = true
		}
	}
	if !poisoned {
		t.Error("eating the red mushroom should poison the player")
	}
}
