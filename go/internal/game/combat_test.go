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
