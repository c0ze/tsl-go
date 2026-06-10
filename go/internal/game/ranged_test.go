package game

import (
	"testing"

	"github.com/c0ze/tsl/internal/content"
)

func TestLineOfSightClear(t *testing.T) {
	g := combatGame() // 10x3, all floor
	if !g.lineOfSight(Pos{1, 1}, Pos{8, 1}) {
		t.Error("clear floor should have line of sight")
	}
}

func TestLineOfSightBlockedByWall(t *testing.T) {
	g := combatGame()
	g.Level.Set(Pos{5, 1}, g.Content.Tiles["wall"])
	if g.lineOfSight(Pos{1, 1}, Pos{8, 1}) {
		t.Error("a wall between the endpoints should block line of sight")
	}
}

func TestRangedMonsterShootsAcrossRoom(t *testing.T) {
	g := combatGame() // player at (1,1)
	g.PlayerHP = 20
	imp := &Creature{Def: &content.MonsterDef{ID: "imp", Name: "imp", HP: 6, Attack: 100, Dodge: 3, Damage: "1d4", Ranged: 6}, Pos: Pos{5, 1}, HP: 6}
	g.Level.Creatures = append(g.Level.Creatures, imp)
	before := imp.Pos

	g.worldTick()

	if g.PlayerHP >= 20 {
		t.Errorf("a ranged monster with line of sight should hit the player, HP=%d", g.PlayerHP)
	}
	if imp.Pos != before {
		t.Errorf("a shooting monster should hold position, moved to %v", imp.Pos)
	}
}

func TestRangedMonsterBlockedApproaches(t *testing.T) {
	g := combatGame()
	g.PlayerHP = 20
	g.Level.Set(Pos{3, 1}, g.Content.Tiles["wall"]) // wall on the sightline
	imp := &Creature{Def: &content.MonsterDef{ID: "imp", Name: "imp", HP: 6, Attack: 100, Dodge: 3, Damage: "1d4", Ranged: 6}, Pos: Pos{5, 1}, HP: 6}
	g.Level.Creatures = append(g.Level.Creatures, imp)
	beforeX := imp.Pos.X

	g.worldTick()

	if g.PlayerHP != 20 {
		t.Errorf("a monster with no line of sight should not shoot, HP=%d", g.PlayerHP)
	}
	if imp.Pos.X >= beforeX {
		t.Error("a monster with no line of sight should approach instead of idling")
	}
}

func TestRangedMonsterOutOfRangeApproaches(t *testing.T) {
	g := combatGame() // 10 wide; player at (1,1)
	g.PlayerHP = 20
	imp := &Creature{Def: &content.MonsterDef{ID: "imp", Name: "imp", HP: 6, Attack: 100, Dodge: 3, Damage: "1d4", Ranged: 3}, Pos: Pos{8, 1}, HP: 6}
	g.Level.Creatures = append(g.Level.Creatures, imp)
	beforeX := imp.Pos.X

	g.worldTick()

	if g.PlayerHP != 20 {
		t.Errorf("an out-of-range ranged monster should not shoot, HP=%d", g.PlayerHP)
	}
	if imp.Pos.X >= beforeX {
		t.Error("an out-of-range ranged monster should approach the player")
	}
}
