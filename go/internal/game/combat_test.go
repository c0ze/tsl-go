package game

import (
	"testing"

	"github.com/c0ze/tsl/internal/content"
	"github.com/c0ze/tsl/internal/rng"
)

func combatGame() *Game {
	c := testContent()
	l := NewLevel(10, 3, c.Tiles["floor"])
	return &Game{
		Content: c, Level: l, Player: Pos{1, 1},
		RNG: rng.NewWithSeed(1), PlayerHP: 20, PlayerMax: 20,
	}
}

func TestPlayerBumpKillsMonster(t *testing.T) {
	g := combatGame()
	rat := &Creature{Def: &content.MonsterDef{ID: "rat", Name: "rat", Glyph: "r", HP: 1, Attack: 1, Dodge: 0, Damage: "1d1"}, Pos: Pos{2, 1}, HP: 1}
	g.Level.Creatures = append(g.Level.Creatures, rat)
	// Bump east into the rat repeatedly until it dies (HP=1, so first solid hit).
	for i := 0; i < 50 && g.Level.CreatureAt(Pos{2, 1}) != nil; i++ {
		g.PlayerStep(DirE)
	}
	if g.Level.CreatureAt(Pos{2, 1}) != nil {
		t.Fatal("rat should be dead after repeated bumps")
	}
	if g.Player != (Pos{1, 1}) {
		t.Errorf("player should not have moved onto the rat's tile while attacking; at %v", g.Player)
	}
	if len(g.Messages) == 0 {
		t.Error("expected combat messages")
	}
}

func TestMonsterAttacksAndCanKillPlayer(t *testing.T) {
	g := combatGame()
	g.PlayerHP = 1
	ogre := &Creature{Def: &content.MonsterDef{ID: "ogre", Name: "ogre", Glyph: "O", HP: 99, Attack: 100, Dodge: 0, Damage: "5d6"}, Pos: Pos{2, 1}, HP: 99}
	g.Level.Creatures = append(g.Level.Creatures, ogre)
	// Player waits in place (steps into a wall-less open tile west and back is
	// unnecessary); just run monster turns directly.
	for i := 0; i < 50 && !g.Dead; i++ {
		g.monstersAct()
	}
	if !g.Dead {
		t.Fatal("player should be dead after the ogre attacks")
	}
}

func TestMonsterMovesTowardPlayer(t *testing.T) {
	g := combatGame()
	rat := &Creature{Def: &content.MonsterDef{ID: "rat", Glyph: "r", HP: 3, Attack: 1, Dodge: 1, Damage: "1d1"}, Pos: Pos{8, 1}, HP: 3}
	g.Level.Creatures = append(g.Level.Creatures, rat)
	before := rat.Pos.X
	g.monstersAct()
	if rat.Pos.X >= before {
		t.Errorf("rat at x=%d should have stepped toward the player (x decreasing), was %d", rat.Pos.X, before)
	}
}

func TestFasterMonsterActsMultipleTimes(t *testing.T) {
	g := combatGame() // 10x3 floor, player at (1,1)
	rat := &Creature{Def: &content.MonsterDef{ID: "rat", Name: "rat", Glyph: "r", HP: 9, Attack: 1, Dodge: 1, Damage: "1d1", Speed: 250}, Pos: Pos{8, 1}, HP: 9}
	g.Level.Creatures = append(g.Level.Creatures, rat)
	before := rat.Pos.X
	g.monstersAct() // speed 250 → 250 energy → acts twice (250-100-100=50)
	if before-rat.Pos.X != 2 {
		t.Errorf("speed-250 monster stepped %d tiles in one turn, want 2", before-rat.Pos.X)
	}
}

func TestDefaultSpeedActsOnce(t *testing.T) {
	g := combatGame()
	rat := &Creature{Def: &content.MonsterDef{ID: "rat", Glyph: "r", HP: 9, Attack: 1, Dodge: 1, Damage: "1d1"}, Pos: Pos{8, 1}, HP: 9} // Speed 0 → default 100
	g.Level.Creatures = append(g.Level.Creatures, rat)
	before := rat.Pos.X
	g.monstersAct()
	if before-rat.Pos.X != 1 {
		t.Errorf("default-speed monster stepped %d tiles, want 1", before-rat.Pos.X)
	}
}

func TestKillCreatureDropsCorpse(t *testing.T) {
	g := combatGame()
	food := &content.ItemDef{ID: "rat_corpse", Name: "rat corpse", Glyph: "%", Color: content.ColorBrown, Kind: "food", Use: "eat", Power: 3}
	g.Content.Items = map[string]*content.ItemDef{"rat_corpse": food}
	rat := &Creature{Def: &content.MonsterDef{ID: "rat", Name: "rat", Corpse: "rat_corpse"}, Pos: Pos{2, 1}, HP: 1}
	g.Level.Creatures = append(g.Level.Creatures, rat)

	g.killCreature(rat)

	if g.Level.CreatureAt(Pos{2, 1}) != nil {
		t.Error("creature should be removed")
	}
	it := g.Level.ItemAt(Pos{2, 1})
	if it == nil || it.Def.ID != "rat_corpse" {
		t.Errorf("expected rat_corpse dropped at kill site, got %v", it)
	}
}

func TestPlayerStepOntoAltarWins(t *testing.T) {
	g := combatGame() // 10x3 floor, player at (1,1)
	g.Level.Set(Pos{2, 1}, &content.TileDef{ID: "altar", Glyph: "_", Passable: true, Transparent: true, Win: true})
	g.PlayerStep(DirE)
	if !g.Won {
		t.Error("stepping onto a win tile should win")
	}
	if g.Player != (Pos{2, 1}) {
		t.Errorf("player should be on the altar at {2 1}, got %v", g.Player)
	}
}

func TestPlayerStepTriggersTrap(t *testing.T) {
	g := combatGame() // 10x3 floor, player at (1,1)
	g.Level.Set(Pos{2, 1}, &content.TileDef{ID: "dart_trap", Glyph: "^", Passable: true, Transparent: true, Effect: "poison", EffectTurns: 5})
	g.PlayerStep(DirE)
	if g.Player != (Pos{2, 1}) {
		t.Fatalf("player should have stepped onto the trap, at %v", g.Player)
	}
	poisoned := false
	for _, e := range g.Effects {
		if e.Kind == "poison" {
			poisoned = true
		}
	}
	if !poisoned {
		t.Error("stepping on a dart trap should poison the player")
	}
}

func TestZapWandDamagesAndKills(t *testing.T) {
	g := combatGame()
	rat := &Creature{Def: &content.MonsterDef{ID: "rat", Name: "rat", HP: 3}, Pos: Pos{3, 1}, HP: 3}
	g.Level.Creatures = append(g.Level.Creatures, rat)
	wand := &Item{Def: &content.ItemDef{Name: "wand", Kind: "wand", Damage: "10d1"}, Charges: 2}
	g.ZapWand(wand, Pos{3, 1})
	if g.Level.CreatureAt(Pos{3, 1}) != nil {
		t.Error("wand should have killed the rat (10 dmg vs 3 HP)")
	}
	if wand.Charges != 1 {
		t.Errorf("charges = %d, want 1 (spent one)", wand.Charges)
	}
}

func TestZapEmptyWandDoesNothing(t *testing.T) {
	g := combatGame()
	rat := &Creature{Def: &content.MonsterDef{ID: "rat", Name: "rat", HP: 3}, Pos: Pos{3, 1}, HP: 3}
	g.Level.Creatures = append(g.Level.Creatures, rat)
	wand := &Item{Def: &content.ItemDef{Name: "wand", Kind: "wand", Damage: "10d1"}, Charges: 0}
	g.ZapWand(wand, Pos{3, 1})
	if g.Level.CreatureAt(Pos{3, 1}) == nil {
		t.Error("an empty wand should not damage anything")
	}
}

func TestKillCreatureNoCorpse(t *testing.T) {
	g := combatGame()
	ghost := &Creature{Def: &content.MonsterDef{ID: "ghost", Name: "ghost"}, Pos: Pos{2, 1}, HP: 1}
	g.Level.Creatures = append(g.Level.Creatures, ghost)

	g.killCreature(ghost)

	if g.Level.ItemAt(Pos{2, 1}) != nil {
		t.Error("monster with no corpse should drop nothing")
	}
}
