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
	Energy  int
	Effects []Effect // timed afflictions (e.g. poison from a venom wand)

	Ally     bool // charmed to the player's side (C attr_player_ally)
	Lifetime int  // ticks before a summon disappears (0 = permanent)

	Disguised  bool             // a mimic still wearing its glamour (C ai_mimic)
	DisguiseAs *content.ItemDef // the loot it poses as while disguised
}

// AddEffect applies a timed status effect to the creature, refreshing an
// already-active effect of the same kind to the longer duration.
func (m *Creature) AddEffect(kind string, turns int) {
	m.Effects = addEffect(m.Effects, kind, turns)
}

// SummonAlly conjures a charmed monster of the given def at the nearest free
// tile beside the player (C magic.c summon_familiar / find_nearest_free_spot),
// expiring after lifetime ticks. It reports whether a spot and def were found.
func (g *Game) SummonAlly(id string, lifetime int) bool {
	def := g.Content.Monsters[id]
	if def == nil {
		return false
	}
	for radius := 1; radius <= 3; radius++ {
		for dy := -radius; dy <= radius; dy++ {
			for dx := -radius; dx <= radius; dx++ {
				p := Pos{X: g.Player.X + dx, Y: g.Player.Y + dy}
				if chebyshev(p, g.Player) != radius {
					continue // ring order: nearest spots first
				}
				if g.Level.Passable(p) && g.Level.CreatureAt(p) == nil {
					g.Level.Creatures = append(g.Level.Creatures, &Creature{Def: def, Pos: p, HP: def.HP, Ally: true, Lifetime: lifetime})
					return true
				}
			}
		}
	}
	return false
}

// RemoveEffect drops an active effect by kind (e.g. a struck sleeper waking);
// an absent kind is a no-op.
func (m *Creature) RemoveEffect(kind string) {
	for i, e := range m.Effects {
		if e.Kind == kind {
			m.Effects = append(m.Effects[:i], m.Effects[i+1:]...)
			return
		}
	}
}

// HasEffect reports whether a timed effect of the given kind is currently active
// on the creature.
func (m *Creature) HasEffect(kind string) bool {
	for _, e := range m.Effects {
		if e.Kind == kind {
			return true
		}
	}
	return false
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
