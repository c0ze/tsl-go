package game

import (
	"strings"
	"testing"

	"github.com/c0ze/tsl/internal/content"
	"github.com/c0ze/tsl/internal/rng"
)

func identifyGame() *Game {
	c := &content.Content{
		Tiles: map[string]*content.TileDef{
			"floor": {ID: "floor", Glyph: ".", Passable: true, Transparent: true},
		},
		Items: map[string]*content.ItemDef{
			"healing":  {ID: "healing", Name: "potion of healing", Kind: "potion", Use: "heal", Power: 8},
			"poison_p": {ID: "poison_p", Name: "potion of poison", Kind: "potion", Use: "venom"},
			"tele":     {ID: "tele", Name: "scroll of teleport", Kind: "scroll", Use: "teleport"},
			"force":    {ID: "force", Name: "wand of force", Kind: "wand", Damage: "1d4"},
			"dagger":   {ID: "dagger", Name: "dagger", Kind: "weapon", Damage: "1d4"},
		},
	}
	g := &Game{
		Content: c, Level: NewLevel(5, 3, c.Tiles["floor"]), Player: Pos{1, 1},
		RNG: rng.NewWithSeed(1), PlayerHP: 20, PlayerMax: 20,
		Behaviors: map[string]Behavior{
			"heal":  func(g *Game, it *Item) []string { return []string{"glug"} },
			"venom": func(g *Game, it *Item) []string { return []string{"ugh"} },
		},
	}
	g.AssignAppearances()
	return g
}

func TestAssignAppearancesFromPools(t *testing.T) {
	g := identifyGame()
	for _, id := range []string{"healing", "poison_p", "tele", "force"} {
		app := g.appearances[id]
		if app == "" {
			t.Errorf("%q got no appearance", id)
		}
		if app == g.Content.Items[id].Name {
			t.Errorf("appearance for %q should differ from its real name", id)
		}
	}
	if _, ok := g.appearances["dagger"]; ok {
		t.Error("a weapon should not get a hidden appearance")
	}
	// Deterministic per seed.
	if identifyGame().appearances["healing"] != g.appearances["healing"] {
		t.Error("appearance assignment should be deterministic for a given seed")
	}
}

func TestDisplayNameUnidentifiedThenReal(t *testing.T) {
	g := identifyGame()
	it := &Item{Def: g.Content.Items["healing"]}
	if got := g.DisplayName(it); got != g.appearances["healing"] {
		t.Errorf("unidentified DisplayName = %q, want appearance %q", got, g.appearances["healing"])
	}
	g.Identified["healing"] = true
	if got := g.DisplayName(it); got != "potion of healing" {
		t.Errorf("identified DisplayName = %q, want the real name", got)
	}
}

func TestDisplayNameWeaponAlwaysReal(t *testing.T) {
	g := identifyGame()
	if got := g.DisplayName(&Item{Def: g.Content.Items["dagger"]}); got != "dagger" {
		t.Errorf("a weapon should always show its real name, got %q", got)
	}
}

func TestUsePotionIdentifiesType(t *testing.T) {
	g := identifyGame()
	p1 := &Item{Def: g.Content.Items["healing"]}
	p2 := &Item{Def: g.Content.Items["healing"]}
	g.Inventory = append(g.Inventory, p1, p2)

	g.PlayerUse(p1)

	if !g.Identified["healing"] {
		t.Error("quaffing should identify the potion type")
	}
	if g.DisplayName(p2) != "potion of healing" {
		t.Error("identifying a type should reveal every item of that type")
	}
	found := false
	for _, m := range g.Messages {
		if strings.Contains(m, "It was") {
			found = true
		}
	}
	if !found {
		t.Errorf("expected an 'It was ...' identify message, got %v", g.Messages)
	}
}

func TestMorgueShowsUnidentifiedByAppearance(t *testing.T) {
	g := identifyGame()
	g.Inventory = append(g.Inventory, &Item{Def: g.Content.Items["healing"]})
	txt := g.MorgueText()
	if !strings.Contains(txt, g.appearances["healing"]) {
		t.Errorf("morgue should list the unidentified potion by appearance, got:\n%s", txt)
	}
	if strings.Contains(txt, "potion of healing") {
		t.Error("morgue should not reveal an unidentified potion's real name")
	}
}

func TestZapWandIdentifies(t *testing.T) {
	g := identifyGame()
	wand := &Item{Def: g.Content.Items["force"], Charges: 2}
	g.ZapWand(wand, Pos{2, 1}) // empty tile: fizzles, but still reveals the wand
	if !g.Identified["force"] {
		t.Error("zapping should identify the wand type")
	}
}

func TestHurtPlayerResolvesDeath(t *testing.T) {
	g := combatGame()
	g.PlayerHP = 3
	g.HurtPlayer(10, "a potion of pain")
	if !g.Dead || g.PlayerHP != 0 || g.DeathCause != "a potion of pain" {
		t.Errorf("HurtPlayer should kill at 0 HP with the cause, got dead=%v hp=%d cause=%q", g.Dead, g.PlayerHP, g.DeathCause)
	}
}

func TestUnidentifiedInventoryFiltersAndDrops(t *testing.T) {
	g := identifyGame()
	p := &Item{Def: g.Content.Items["healing"]}
	weapon := &Item{Def: g.Content.Items["dagger"]} // weapons are always identified
	g.Inventory = append(g.Inventory, p, weapon)

	un := g.UnidentifiedInventory()
	if len(un) != 1 || un[0] != p {
		t.Errorf("UnidentifiedInventory should list only the hidden potion, got %v", un)
	}
	g.IdentifyItem(p)
	if len(g.UnidentifiedInventory()) != 0 {
		t.Error("identifying the potion should drop it from the unidentified list")
	}
	if !g.IsIdentified(p) {
		t.Error("IsIdentified should report the potion identified")
	}
}
