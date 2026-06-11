package game

import (
	"bytes"
	"strings"
	"testing"

	"github.com/c0ze/tsl-go/internal/content"
	"github.com/c0ze/tsl-go/internal/rng"
)

// savedWorld builds a two-level dungeon over registered content and salts it
// with every kind of state the save format must carry.
func savedWorld(t *testing.T) *Game {
	t.Helper()
	floor := &content.TileDef{ID: "floor", Glyph: ".", Passable: true, Transparent: true}
	c := &content.Content{
		Tiles: map[string]*content.TileDef{
			"floor": floor,
			"water": {ID: "water", Glyph: "_", Transparent: true, Water: true},
			"trap":  {ID: "trap", Glyph: "^", Passable: true, Transparent: true, Effect: "poison", EffectTurns: 5},
		},
		Monsters: map[string]*content.MonsterDef{
			"rat":   {ID: "rat", Name: "rat", Glyph: "r", HP: 3, Attack: 2, Dodge: 1, Damage: "1d2"},
			"imp":   {ID: "imp", Name: "imp", Glyph: "i", HP: 6, Attack: 8, Dodge: 2, Damage: "1d4"},
			"mimic": {ID: "mimic", Name: "mimic", Glyph: "m", HP: 5, Attack: 6, Damage: "1d4", Mimic: true},
		},
		Items: map[string]*content.ItemDef{
			"dagger": {ID: "dagger", Name: "dagger", Glyph: ")", Kind: "weapon", Attack: 2, Damage: "1d4", Weight: 18},
			"potion": {ID: "potion", Name: "odd potion", Glyph: "!", Kind: "potion", Weight: 7},
			"arrow":  {ID: "arrow", Name: "crude arrow", Glyph: ":", Kind: "ammo", Weight: 1},
		},
		Levels: map[string]*content.LevelDef{
			"a": {ID: "a", Name: "Level A", W: 8, H: 4, Start: true, Links: []string{"b"}},
			"b": {ID: "b", Name: "Level B", W: 8, H: 4, Links: []string{"a"}},
		},
	}
	build := func(def *content.LevelDef) (*Level, error) {
		l := NewLevel(def.W, def.H, floor)
		l.Start = Pos{1, 1}
		l.Portals = []Portal{{Pos: Pos{6, 1}, Target: def.Links[0]}}
		return l, nil
	}
	d, err := NewDungeon(c.Levels, "a", build)
	if err != nil {
		t.Fatal(err)
	}
	g := &Game{
		Content: c, Dungeon: d, Level: d.Current(),
		RNG: rng.NewWithSeed(99), PlayerHP: 17, PlayerMax: 20, EP: 3, EPMax: 9,
		Identified: map[string]bool{"potion": true},
	}
	g.EnterStart()

	// Salt the world: clocks, effects, gear, allies, glamours, hidden traps.
	g.AddEffect("poison", 4)
	g.Known = map[string]bool{"potion": true} // a learned "book" (any item id works for the round-trip)
	g.swimFatigue = 2
	g.epTurn = 1
	dagger := &Item{Def: c.Items["dagger"]}
	arrows := &Item{Def: c.Items["arrow"], Charges: 13}
	g.Inventory = append(g.Inventory, dagger, arrows)
	g.Weapon = dagger
	g.MarkRecall() // pinned on level a at the start tile
	lvl := d.Current()
	lvl.Set(Pos{4, 2}, c.Tiles["trap"])
	tr := lvl.At(Pos{4, 2})
	tr.Disguise = floor
	tr.TrapDifficulty = 9
	lvl.Set(Pos{6, 2}, c.Tiles["water"])
	rat := &Creature{Def: c.Monsters["rat"], Pos: Pos{5, 1}, HP: 3}
	rat.AddEffect("slow", 6)
	imp := &Creature{Def: c.Monsters["imp"], Pos: Pos{2, 2}, HP: 6, Ally: true, Lifetime: 321, Energy: 40}
	mim := &Creature{Def: c.Monsters["mimic"], Pos: Pos{3, 3}, HP: 5, Disguised: true, DisguiseAs: c.Items["potion"]}
	lvl.Creatures = append(lvl.Creatures, rat, imp, mim)
	lvl.Items = append(lvl.Items, &Item{Def: c.Items["potion"], Pos: Pos{5, 2}})

	// Visit level b so the cache holds two levels with state.
	g.Player = Pos{6, 1}
	g.Travel()
	return g
}

func TestSaveLoadFixedPoint(t *testing.T) {
	g := savedWorld(t)
	var first bytes.Buffer
	if err := g.Save(&first); err != nil {
		t.Fatal(err)
	}
	g2, err := LoadGame(bytes.NewReader(first.Bytes()), g.Content, g.Behaviors, nil)
	if err != nil {
		t.Fatal(err)
	}
	var second bytes.Buffer
	if err := g2.Save(&second); err != nil {
		t.Fatal(err)
	}
	if first.String() != second.String() {
		t.Error("save(load(save(g))) must equal save(g) — some state fell out of the round-trip")
	}
}

func TestLoadContinuesDiceExactly(t *testing.T) {
	g := savedWorld(t)
	var buf bytes.Buffer
	if err := g.Save(&buf); err != nil {
		t.Fatal(err)
	}
	g2, err := LoadGame(&buf, g.Content, g.Behaviors, nil)
	if err != nil {
		t.Fatal(err)
	}
	for i := 0; i < 25; i++ {
		if a, b := g.RNG.Uint32(), g2.RNG.Uint32(); a != b {
			t.Fatalf("dice diverged at draw %d: %d vs %d", i, a, b)
		}
	}
}

func TestLoadRestoresEquippedIdentity(t *testing.T) {
	g := savedWorld(t)
	var buf bytes.Buffer
	if err := g.Save(&buf); err != nil {
		t.Fatal(err)
	}
	g2, err := LoadGame(&buf, g.Content, g.Behaviors, nil)
	if err != nil {
		t.Fatal(err)
	}
	if g2.Weapon == nil || g2.Weapon != g2.Inventory[0] {
		t.Error("the loaded weapon must BE the inventory entry, not a copy")
	}
	if g2.Inventory[1].Charges != 13 {
		t.Errorf("the quiver should still hold 13, got %d", g2.Inventory[1].Charges)
	}
}

func TestLoadKeepsRecallAcrossLevels(t *testing.T) {
	g := savedWorld(t) // saved while standing on level b, pin on a
	var buf bytes.Buffer
	if err := g.Save(&buf); err != nil {
		t.Fatal(err)
	}
	g2, err := LoadGame(&buf, g.Content, g.Behaviors, nil)
	if err != nil {
		t.Fatal(err)
	}
	if g2.Dungeon.current != "b" {
		t.Fatalf("should resume on level b, got %q", g2.Dungeon.current)
	}
	if !g2.Recall() {
		t.Fatal("the pinned recall should survive the save")
	}
	if g2.Dungeon.current != "a" || g2.Player != (Pos{1, 1}) {
		t.Errorf("recall should land on a at the pin, got %q %v", g2.Dungeon.current, g2.Player)
	}
}

func TestLoadFailsOnUnknownContent(t *testing.T) {
	g := savedWorld(t)
	var buf bytes.Buffer
	if err := g.Save(&buf); err != nil {
		t.Fatal(err)
	}
	doctored := strings.ReplaceAll(buf.String(), `"id":"rat"`, `"id":"ghost"`)
	if _, err := LoadGame(strings.NewReader(doctored), g.Content, g.Behaviors, nil); err == nil {
		t.Error("a save referencing unknown content must fail the load (fail-fast)")
	}
}
