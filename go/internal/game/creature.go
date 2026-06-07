package game

import "github.com/c0ze/tsl/internal/content"

// Faction marks whose side a creature is on.
type Faction int

const (
	FactionEnemy Faction = iota
	FactionPlayer
)

// Creature is a monster on the level (the player is modelled separately on Game
// for now). HP is current hit points; Def carries its stats and glyph.
type Creature struct {
	Def     *content.MonsterDef
	Pos     Pos
	HP      int
	Faction Faction
}

// CreatureAt returns the creature standing on p, or nil.
func (l *Level) CreatureAt(p Pos) *Creature {
	for _, cr := range l.Creatures {
		if cr.Pos == p {
			return cr
		}
	}
	return nil
}

// RemoveCreature removes cr from the level (no-op if absent).
func (l *Level) RemoveCreature(cr *Creature) {
	for i, x := range l.Creatures {
		if x == cr {
			l.Creatures = append(l.Creatures[:i], l.Creatures[i+1:]...)
			return
		}
	}
}
