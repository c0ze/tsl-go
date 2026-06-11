package game

import (
	"testing"

	"github.com/c0ze/tsl-go/internal/content"
)

func TestCreatureAtAndRemove(t *testing.T) {
	c := testContent()
	l := NewLevel(5, 5, c.Tiles["floor"])
	rat := &Creature{Def: &content.MonsterDef{ID: "rat", Glyph: "r", HP: 3}, Pos: Pos{2, 2}, HP: 3}
	l.Creatures = append(l.Creatures, rat)
	if got := l.CreatureAt(Pos{2, 2}); got != rat {
		t.Fatalf("CreatureAt(2,2) = %v, want rat", got)
	}
	if got := l.CreatureAt(Pos{0, 0}); got != nil {
		t.Errorf("CreatureAt(0,0) = %v, want nil", got)
	}
	l.RemoveCreature(rat)
	if got := l.CreatureAt(Pos{2, 2}); got != nil {
		t.Error("rat should be gone after RemoveCreature")
	}
}
