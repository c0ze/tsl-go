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
