package behaviors

import (
	"testing"

	"github.com/c0ze/tsl/internal/content"
	"github.com/c0ze/tsl/internal/game"
	"github.com/c0ze/tsl/internal/rng"
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

func TestTeleportBehaviorMovesPlayer(t *testing.T) {
	tele, ok := Registry()["teleport"]
	if !ok {
		t.Fatal("teleport behavior not registered")
	}
	floor := &content.TileDef{ID: "floor", Glyph: ".", Color: content.ColorNormal, Passable: true, Transparent: true}
	g := &game.Game{Level: game.NewLevel(8, 3, floor), Player: game.Pos{X: 1, Y: 1}, RNG: rng.NewWithSeed(1)}
	start := g.Player
	msgs := tele(g, &game.Item{Def: &content.ItemDef{Name: "scroll of teleportation"}})
	if g.Player == start {
		t.Error("teleport scroll should move the player")
	}
	if len(msgs) == 0 {
		t.Error("teleport should return a message")
	}
}

func TestRevealBehaviorMarksSeen(t *testing.T) {
	reveal, ok := Registry()["reveal"]
	if !ok {
		t.Fatal("reveal behavior not registered")
	}
	floor := &content.TileDef{ID: "floor", Glyph: ".", Color: content.ColorNormal, Passable: true, Transparent: true}
	lvl := game.NewLevel(6, 3, floor)
	g := &game.Game{Level: lvl, Player: game.Pos{X: 1, Y: 1}}
	reveal(g, &game.Item{Def: &content.ItemDef{Name: "scroll of magic mapping"}})
	if !lvl.At(game.Pos{X: 4, Y: 2}).Seen {
		t.Error("magic mapping should mark distant tiles seen")
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
