package game

import (
	"testing"

	"github.com/c0ze/tsl-go/internal/content"
	"github.com/c0ze/tsl-go/internal/rng"
)

func sfxGame(t *testing.T, rows []string) *Game {
	t.Helper()
	c := &content.Content{Tiles: map[string]*content.TileDef{
		"floor":       {ID: "floor", Glyph: ".", Color: content.ColorNormal, Passable: true, Transparent: true},
		"wall":        {ID: "wall", Glyph: "#", Passable: false},
		"door_closed": {ID: "door_closed", Glyph: "+", Color: content.ColorBrown, OpensTo: "door_open"},
		"door_open":   {ID: "door_open", Glyph: "'", Color: content.ColorBrown, Passable: true, Transparent: true, ClosesTo: "door_closed"},
	}}
	lvl, start, err := ParseLevel(c, rows, map[rune]string{'.': "floor", '#': "wall", '@': "floor", '+': "door_closed"})
	if err != nil {
		t.Fatal(err)
	}
	return &Game{Content: c, Level: lvl, Player: start, RNG: rng.NewWithSeed(1), PlayerHP: 20, PlayerMax: 20}
}

func hasSound(g *Game, id string) bool {
	for _, s := range g.Sounds {
		if s == id {
			return true
		}
	}
	return false
}

func TestStepCue(t *testing.T) {
	g := sfxGame(t, []string{"@.."})
	g.PlayerStep(DirE)
	if !hasSound(g, "step") {
		t.Errorf("a move should cue a footstep, got %v", g.Sounds)
	}
}

func TestSwooshCue(t *testing.T) {
	g := sfxGame(t, []string{"@.."})
	m := &Creature{Def: &content.MonsterDef{ID: "rat", Name: "rat", HP: 5, Damage: "1d1"}, Pos: Pos{X: 1, Y: 0}, HP: 5}
	g.playerAttacks(m) // a swing cues a swoosh whether or not it lands
	if !hasSound(g, "swoosh") {
		t.Errorf("an attack should cue a swoosh, got %v", g.Sounds)
	}
}

func TestDeathCue(t *testing.T) {
	g := sfxGame(t, []string{"@.."})
	m := &Creature{Def: &content.MonsterDef{ID: "rat", Name: "rat", HP: 1, Damage: "1d1"}, Pos: Pos{X: 1, Y: 0}, HP: 1}
	g.Level.Creatures = append(g.Level.Creatures, m)
	g.killCreature(m)
	if !hasSound(g, "death") {
		t.Errorf("a kill should cue a death sound, got %v", g.Sounds)
	}
}

func TestHurtCue(t *testing.T) {
	g := sfxGame(t, []string{"@.."})
	g.HurtPlayer(1, "rat")
	if !hasSound(g, "hurt") {
		t.Errorf("taking damage should cue a hurt sound, got %v", g.Sounds)
	}
}

func TestPickupWeaponCue(t *testing.T) {
	g := sfxGame(t, []string{"@.."})
	g.Level.Items = append(g.Level.Items, &Item{Def: &content.ItemDef{ID: "sword", Name: "sword", Kind: "weapon", Damage: "1d6"}, Pos: Pos{X: 0, Y: 0}})
	g.PlayerPickup()
	if !hasSound(g, "pickup_weapon") {
		t.Errorf("picking up a weapon should cue pickup_weapon, got %v", g.Sounds)
	}
}

func TestEatCue(t *testing.T) {
	g := sfxGame(t, []string{"@.."})
	food := &Item{Def: &content.ItemDef{ID: "ration", Name: "ration", Kind: "food", Use: "noop"}}
	g.Inventory = append(g.Inventory, food)
	g.PlayerUse(food)
	if !hasSound(g, "eat") {
		t.Errorf("eating should cue an eat sound, got %v", g.Sounds)
	}
}

func TestZapCue(t *testing.T) {
	g := sfxGame(t, []string{"@.."})
	wand := &Item{Def: &content.ItemDef{Name: "wand", Kind: "wand", Damage: "1d4"}, Charges: 2}
	g.ZapWand(wand, Pos{X: 2, Y: 0})
	if !hasSound(g, "zap") {
		t.Errorf("zapping a wand should cue a zap, got %v", g.Sounds)
	}
}

func TestDoorOpenCue(t *testing.T) {
	g := sfxGame(t, []string{"@+."})
	g.PlayerStep(DirE) // bump the closed door open
	if !hasSound(g, "door") {
		t.Errorf("opening a door should cue a door sound, got %v", g.Sounds)
	}
}

func TestCastCue(t *testing.T) {
	g := sfxGame(t, []string{"@.."})
	g.EP, g.EPMax = 10, 10
	g.Behaviors = map[string]Behavior{"first_aid": func(gg *Game, it *Item) []string { return []string{"mend"} }}
	book := &Item{Def: &content.ItemDef{ID: "book_aid", Name: "first aid", Kind: "spellbook", Use: "first_aid", Cost: 4}}
	g.CastSpell(book)
	if !hasSound(g, "cast") {
		t.Errorf("casting a spell should cue a cast, got %v", g.Sounds)
	}
}
