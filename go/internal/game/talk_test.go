package game

import (
	"testing"

	"github.com/c0ze/tsl/internal/content"
)

func TestTalkGetsTheChatLine(t *testing.T) {
	g := combatGame()
	gnoblin := &Creature{Def: &content.MonsterDef{ID: "gnoblin", Name: "gnoblin", Glyph: "g", HP: 4, Attack: 0, Dodge: 1, Damage: "1d2", Chat: "The gnoblin snarls at you!"}, Pos: Pos{2, 1}, HP: 4}
	rat := &Creature{Def: &content.MonsterDef{ID: "rat", Name: "rat", Glyph: "r", HP: 3, Attack: 0, Dodge: 1, Damage: "1d1"}, Pos: Pos{8, 1}, HP: 3}
	g.Level.Creatures = append(g.Level.Creatures, gnoblin, rat)
	g.Talk()
	if !hasMessage(g, "The gnoblin snarls at you!") {
		t.Errorf("expected the C interact line, got %v", g.Messages)
	}
	if rat.Pos.X != 7 {
		t.Error("a chat passes the turn (C interact returns true)")
	}
}

func TestTalkToNobodyIsFree(t *testing.T) {
	g := combatGame()
	rat := &Creature{Def: &content.MonsterDef{ID: "rat", Name: "rat", Glyph: "r", HP: 3, Attack: 0, Dodge: 1, Damage: "1d1"}, Pos: Pos{8, 1}, HP: 3}
	g.Level.Creatures = append(g.Level.Creatures, rat)
	g.Talk()
	if !hasMessage(g, "There is no one to talk to.") {
		t.Errorf("expected a refusal, got %v", g.Messages)
	}
	if rat.Pos.X != 8 {
		t.Error("talking to the air costs nothing")
	}
}

func TestTalkToSleeperIsFree(t *testing.T) {
	g := combatGame()
	gnoblin := &Creature{Def: &content.MonsterDef{ID: "gnoblin", Name: "gnoblin", Glyph: "g", HP: 4, Attack: 0, Dodge: 1, Damage: "1d2", Chat: "The gnoblin snarls at you!"}, Pos: Pos{2, 1}, HP: 4}
	gnoblin.AddEffect("sleep", 10)
	rat := &Creature{Def: &content.MonsterDef{ID: "rat", Name: "rat", Glyph: "r", HP: 3, Attack: 0, Dodge: 1, Damage: "1d1"}, Pos: Pos{8, 1}, HP: 3}
	g.Level.Creatures = append(g.Level.Creatures, gnoblin, rat)
	g.Talk()
	if !hasMessage(g, "The gnoblin appears to be asleep.") {
		t.Errorf("expected the C sleep line, got %v", g.Messages)
	}
	if hasMessage(g, "snarls") || rat.Pos.X != 8 {
		t.Error("a sleeping neighbor neither chats nor costs a turn (C interact returns false)")
	}
}

func TestTalkToSilentMonster(t *testing.T) {
	g := combatGame()
	rat := &Creature{Def: &content.MonsterDef{ID: "rat", Name: "rat", Glyph: "r", HP: 3, Attack: 0, Dodge: 1, Damage: "1d1"}, Pos: Pos{2, 1}, HP: 3}
	observer := &Creature{Def: &content.MonsterDef{ID: "rat", Name: "rat", Glyph: "r", HP: 3, Attack: 0, Dodge: 1, Damage: "1d1"}, Pos: Pos{8, 1}, HP: 3}
	g.Level.Creatures = append(g.Level.Creatures, rat, observer)
	g.Talk()
	if !hasMessage(g, "You get no reply.") {
		t.Errorf("a silent monster gets the gentle default (the C's line is a BUG marker), got %v", g.Messages)
	}
	if observer.Pos.X != 7 {
		t.Error("stopping to ask still costs the turn")
	}
}
