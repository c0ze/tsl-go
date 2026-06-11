package game

import (
	"testing"

	"github.com/c0ze/tsl-go/internal/content"
)

func wear(g *Game, slot **Item, def *content.ItemDef) *Item {
	it := &Item{Def: def}
	g.Inventory = append(g.Inventory, it)
	*slot = it
	return it
}

func TestBootsOfSpeedQuickenTheScheduler(t *testing.T) {
	g, rat := schedulerGame()
	wear(g, &g.Boots, &content.ItemDef{ID: "boots_of_speed", Name: "boots of speed", Kind: "boots", SpeedMod: 20})
	// +20 speed banks a free action every sixth turn (haste at +30 banks
	// every fifth): six actions cost five ticks.
	for i := 0; i < 6; i++ {
		g.advanceWorld()
	}
	if rat.Pos.X != 3 {
		t.Errorf("at 120 speed six actions cost five ticks: rat at x=%d, want 3", rat.Pos.X)
	}
}

func TestLeadBootsCrawl(t *testing.T) {
	g, rat := schedulerGame()
	wear(g, &g.Boots, &content.ItemDef{ID: "lead_boots", Name: "lead boots", Kind: "boots", SpeedMod: -50})
	g.advanceWorld() // speed 50: the world ticks twice per action
	if rat.Pos.X != 6 {
		t.Errorf("lead boots halve the pace: rat at x=%d, want 6", rat.Pos.X)
	}
}

func TestFlippersBuySwimTurns(t *testing.T) {
	g := waterGame()
	wear(g, &g.Boots, &content.ItemDef{ID: "flippers", Name: "lizardskin flippers", Kind: "boots", SwimSkill: 3})
	g.AddEffect("blind", 2)
	g.PlayerStep(DirE) // into the pool: fatigue 1 <= skill 3, no damage
	g.advanceWorld()   // fatigue 2
	g.advanceWorld()   // fatigue 3 — still free
	if g.PlayerHP != 20 {
		t.Fatalf("three turns of grace with +3 swimming (C attr_swimming), HP %d", g.PlayerHP)
	}
	g.advanceWorld() // fatigue 4 > 3: the clock bites
	if g.PlayerHP != 19 {
		t.Errorf("the fourth turn drowns as usual, HP %d", g.PlayerHP)
	}
}

func TestGasMaskBlocksFumesAndFood(t *testing.T) {
	g := combatGame()
	wear(g, &g.Head, &content.ItemDef{ID: "gas_mask", Name: "gas mask", Kind: "head", GasImmune: true})
	toad := &Creature{Def: &content.MonsterDef{ID: "toad", Name: "giant slimy toad", Glyph: "t", HP: 14, Attack: 5, Dodge: 1, Damage: "1d4", Breath: "poison"}, Pos: Pos{1, 2}, HP: 14}
	g.Level.Creatures = append(g.Level.Creatures, toad)
	g.worldTick()
	if g.HasEffect("poison") {
		t.Error("a gas mask blocks noxious breath (C attr_gas_immunity)")
	}
	ration := &Item{Def: &content.ItemDef{ID: "ration", Name: "ration", Kind: "food", Use: "eat", Power: 5}}
	g.Inventory = append(g.Inventory, ration)
	g.PlayerHP = 10
	g.PlayerUse(ration)
	if g.PlayerHP != 10 || !g.hasInventoryItem(ration) {
		t.Error("you cannot eat through a gas mask (C attr_p_eat)")
	}
}

func TestBlindfoldStumblesLikeBlindness(t *testing.T) {
	g := waterGame()
	wear(g, &g.Head, &content.ItemDef{ID: "blindfold", Name: "blindfold", Kind: "head", Blindfold: true})
	g.PlayerStep(DirE)
	if g.Player != (Pos{2, 1}) {
		t.Errorf("a blindfolded walker blunders into water like the blinded, at %v", g.Player)
	}
}

func TestStealthShrinksNoticeRange(t *testing.T) {
	g := combatGame()
	wear(g, &g.Cloak, &content.ItemDef{ID: "dark_cloak", Name: "dark cloak", Kind: "cloak", Stealth: 2})
	rat := &Creature{Def: &content.MonsterDef{ID: "rat", Name: "rat", Glyph: "r", HP: 3, Attack: 0, Dodge: 1, Damage: "1d1"}, Pos: Pos{8, 1}, HP: 3}
	g.Level.Creatures = append(g.Level.Creatures, rat)
	g.worldTick()
	if rat.Pos.X != 8 {
		t.Errorf("at distance 7 with stealth 2 (range 6) the rat hasn't noticed, at x=%d", rat.Pos.X)
	}
	g.Player = Pos{2, 1} // distance 6: inside the shrunken range
	g.worldTick()
	if rat.Pos.X != 7 {
		t.Errorf("inside the shrunken range pursuit resumes, at x=%d", rat.Pos.X)
	}
}

func TestNewSlotsAutoEquipAndSum(t *testing.T) {
	g := combatGame()
	boots := &Item{Def: &content.ItemDef{ID: "padded_boots", Name: "padded boots", Kind: "boots", Stealth: 2}, Pos: g.Player}
	g.Level.Items = append(g.Level.Items, boots)
	g.PlayerPickup()
	if g.Boots != boots {
		t.Error("empty slots autoequip on pickup, like weapon and armor")
	}
	wear(g, &g.Cloak, &content.ItemDef{ID: "seaweed_cloak", Name: "seaweed cloak", Kind: "cloak", Dodge: 1})
	if g.playerDodgeStat() != playerDodge+1 {
		t.Errorf("worn gear joins the stat sums, dodge %d", g.playerDodgeStat())
	}
}
