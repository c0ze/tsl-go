package game

import (
	"testing"

	"github.com/c0ze/tsl-go/internal/content"
)

// A wounding hit makes the player bleed 1 HP per step until it closes.
func TestWoundedPlayerBleedsWhenMoving(t *testing.T) {
	g := combatGame()
	g.woundPlayer()
	if !hasMessage(g, "You have been wounded!") || !g.HasEffect("wound") {
		t.Fatalf("expected an open wound, got %v %v", g.Messages, g.Effects)
	}
	hp := g.PlayerHP
	g.PlayerStep(DirE)
	if g.PlayerHP != hp-1 || !hasMessage(g, "You are bleeding!") {
		t.Errorf("one step should cost 1 HP of blood: hp %d -> %d", hp, g.PlayerHP)
	}
}

// Standing still doesn't bleed (C wound_damage runs after a move).
func TestWoundDoesNotBleedWithoutMoving(t *testing.T) {
	g := combatGame()
	g.woundPlayer()
	g.Player = Pos{1, 0}
	hp := g.PlayerHP
	g.PlayerStep(DirN) // off the map's top edge: no move
	if g.PlayerHP != hp {
		t.Errorf("a blocked step shouldn't bleed: hp %d -> %d", hp, g.PlayerHP)
	}
}

// Wounds stack their time (C prolong_effect) and bleeding can kill.
func TestWoundsStackAndBleedOut(t *testing.T) {
	g := combatGame()
	g.woundPlayer()
	g.woundPlayer()
	if g.Effects[0].Turns != 2*meleeWoundTime {
		t.Errorf("wound time should add up, got %d", g.Effects[0].Turns)
	}
	g.PlayerHP = 1
	g.PlayerStep(DirE)
	if !g.Dead || g.DeathCause != "bled to death" {
		t.Errorf("the last drop should kill: dead=%v cause=%q", g.Dead, g.DeathCause)
	}
}

// A clawed monster wounds on a landed hit at its chance.
func TestClawsWoundThePlayer(t *testing.T) {
	g := combatGame()
	cat := &Creature{Def: &content.MonsterDef{ID: "cat", Name: "cat", HP: 5, Attack: 1000, Damage: "1d1", Wound: 100}, Pos: Pos{2, 1}, HP: 5}
	g.monsterAttacks(cat)
	if !g.HasEffect("wound") {
		t.Error("a certain-wound claw should wound")
	}
}

// Monsters bleed as they move too; the immune never do.
func TestCreaturesBleedUnlessImmune(t *testing.T) {
	g := combatGame()
	rat := &Creature{Def: &content.MonsterDef{ID: "rat", Name: "rat", HP: 1}, Pos: Pos{8, 1}, HP: 1}
	slime := &Creature{Def: &content.MonsterDef{ID: "slime", Name: "slime", HP: 3, WoundImmune: true}, Pos: Pos{5, 0}, HP: 3}
	g.Level.Creatures = append(g.Level.Creatures, rat, slime)
	g.UpdateFOV() // collapses are only narrated in view
	g.woundCreature(rat)
	g.woundCreature(slime)
	if slime.HasEffect("wound") {
		t.Error("a wound-immune slime was wounded")
	}
	g.stepToward(rat, g.Player)
	if g.Level.CreatureAt(rat.Pos) == rat || !hasMessage(g, "The rat collapses!") {
		t.Errorf("a 1-HP bleeding rat should collapse after moving, messages %v", g.Messages)
	}
}

// First aid on an unwounded caster is refused free (C: invoke returns false).
func TestFirstAidRefusedWhenUnwounded(t *testing.T) {
	g := combatGame()
	g.EP, g.EPMax = 5, 5
	book := &Item{Def: &content.ItemDef{ID: "spellbook_first_aid", Name: "manual of first aid", Kind: "spellbook", Use: "first_aid", Cost: 1}}
	g.CastSpell(book)
	if g.EP != 5 || !hasMessage(g, "You are not wounded.") {
		t.Errorf("unwounded first aid should cost nothing: EP %d, messages %v", g.EP, g.Messages)
	}
}

// Off-screen wounds and collapses stay unnarrated (C can_see / can_see_creature).
func TestUnseenWoundsAreSilent(t *testing.T) {
	g := combatGame()
	rat := &Creature{Def: &content.MonsterDef{ID: "rat", Name: "rat", HP: 1}, Pos: Pos{8, 1}, HP: 1}
	g.Level.Creatures = append(g.Level.Creatures, rat)
	g.woundCreature(rat) // no FOV computed: nothing is visible
	g.stepToward(rat, g.Player)
	if len(g.Messages) != 0 {
		t.Errorf("unseen events were narrated: %v", g.Messages)
	}
}

// Temporary weapons replace the wielded one and never wound (C vweapon.c).
func TestTempWeaponIgnoresWieldedWound(t *testing.T) {
	g := combatGame()
	g.Weapon = &Item{Def: &content.ItemDef{ID: "machete", Name: "machete", Kind: "weapon", Wound: 50}}
	g.AddEffect("flame_hands", 5)
	if g.playerWoundChance() != 0 {
		t.Error("flaming hands shouldn't carry the machete's wounding")
	}
}

// Bleeding out on a step into water ends there: the cause stays "bled to
// death" rather than being rewritten by the drowning check.
func TestBleedDeathIsFinal(t *testing.T) {
	g := waterGame()
	g.woundPlayer()
	g.PlayerHP = 1
	g.PlayerStep(DirE)
	if !g.Dead || g.DeathCause != "bled to death" || hasMessage(g, "You drown...") {
		t.Errorf("dead=%v cause=%q messages %v", g.Dead, g.DeathCause, g.Messages)
	}
}

// A collapse right beside the player is heard even out of view — unless the
// player is blind (C can_see_creature, fov.c:374-418).
func TestAdjacentCollapseNarratedUnlessBlind(t *testing.T) {
	for _, blind := range []bool{false, true} {
		g := combatGame()
		if blind {
			g.AddEffect("blind", 10)
		}
		rat := &Creature{Def: &content.MonsterDef{ID: "rat", Name: "rat", HP: 1}, Pos: Pos{3, 1}, HP: 1}
		g.Level.Creatures = append(g.Level.Creatures, rat)
		rat.Effects = prolongEffect(rat.Effects, "wound", meleeWoundTime) // no FOV: the tile isn't Visible
		g.stepToward(rat, g.Player)                                       // bleeds out beside the player at (2,1)
		if heard := hasMessage(g, "The rat collapses!"); heard == blind {
			t.Errorf("blind=%v: heard=%v, messages %v", blind, heard, g.Messages)
		}
	}
}
