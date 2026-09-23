package game

import (
	"fmt"

	"github.com/c0ze/tsl-go/internal/content"
)

// Portal is a stair tile that leads to another level.
type Portal struct {
	Pos    Pos
	Target string // target level id
}

// PortalAt returns the portal at p, or nil.
func (l *Level) PortalAt(p Pos) *Portal {
	for i := range l.Portals {
		if l.Portals[i].Pos == p {
			return &l.Portals[i]
		}
	}
	return nil
}

// Dungeon is the level graph plus the cache of already-generated levels. Levels
// are generated on first visit and persisted, so returning to one shows the same
// state (dead monsters stay dead). build is injected by cmd (the gen package),
// keeping game free of a gen dependency.
type Dungeon struct {
	defs    map[string]*content.LevelDef
	cache   map[string]*Level
	current string
	build   func(*content.LevelDef) (*Level, error)
}

// NewDungeon creates the graph and enters the start level (generating it).
func NewDungeon(defs map[string]*content.LevelDef, startID string, build func(*content.LevelDef) (*Level, error)) (*Dungeon, error) {
	d := &Dungeon{defs: defs, cache: map[string]*Level{}, build: build}
	if err := d.enter(startID); err != nil {
		return nil, err
	}
	return d, nil
}

// enter makes id the current level, generating and caching it on first visit.
func (d *Dungeon) enter(id string) error {
	if _, ok := d.cache[id]; !ok {
		def := d.defs[id]
		if def == nil {
			return fmt.Errorf("dungeon: no level %q", id)
		}
		lvl, err := d.build(def)
		if err != nil {
			return err
		}
		lvl.ID = id
		d.cache[id] = lvl
	}
	d.current = id
	return nil
}

// Current returns the level the player is on.
func (d *Dungeon) Current() *Level { return d.cache[d.current] }

// Name returns the display name of the current level.
func (d *Dungeon) Name() string {
	if def := d.defs[d.current]; def != nil {
		return def.Name
	}
	return ""
}

// Travel takes the portal under the player to its target level (C stairs):
// the player arrives on that level's staircase leading back, everyone
// standing beside the stairs follows (C move_everyone, bar disguised
// mimics), and the climb costs the turn.
func (g *Game) Travel() {
	if g.Dead || g.Won {
		return
	}
	p := g.Level.PortalAt(g.Player)
	if p == nil {
		g.log("There are no stairs here.")
		return
	}
	from, fromPos := g.Level, g.Player
	g.Level.Return = g.Player // remember where we leave from
	if err := g.Dungeon.enter(p.Target); err != nil {
		g.log("The way is blocked.")
		return
	}
	g.Level = g.Dungeon.Current()
	if g.Level.entered {
		g.Player = g.Level.Return
	} else {
		g.Player = g.Level.Start
		g.Level.entered = true
	}
	for _, back := range g.Level.Portals { // C traverse_branch: the linked stair
		if back.Target == from.ID {
			g.Player = back.Pos
			break
		}
	}
	arrival := g.Player
	if g.Level.CreatureAt(arrival) != nil {
		// Someone is standing on the stairs: step off beside them rather
		// than share their tile (the C would overlap them).
		if spot, ok := g.nearestFreeSpot(arrival, g.walkerFits); ok {
			g.Player = spot
		}
	}
	g.bringFollowers(from, fromPos, arrival)
	def := g.Dungeon.defs[g.Level.ID]
	g.log("You enter %s.", def.Name)
	g.Sound("descend")
	g.advanceWorld()
}

// bringFollowers moves every creature adjacent to the stairs the player just
// took on level from onto the current level, around the arrival stairs (C
// move_everyone + find_nearest_free_spot). A disguised mimic stays put.
func (g *Game) bringFollowers(from *Level, stairs, arrival Pos) {
	for _, m := range append([]*Creature(nil), from.Creatures...) {
		if m.Disguised || chebyshev(m.Pos, stairs) != 1 {
			continue
		}
		fits := g.walkerFits
		// A swimmer lands in water (C is_swimming: a permaswimmer, or a
		// free-swimmer that is in the water as it follows).
		if m.Def.Permaswim || m.Def.Swim && from.At(m.Pos).Def.Water {
			fits = func(p Pos) bool { return g.Level.At(p).Def.Water && g.Level.CreatureAt(p) == nil && p != g.Player }
		}
		if spot, ok := g.nearestFreeSpot(arrival, fits); ok {
			from.RemoveCreature(m)
			m.Pos = spot
			g.Level.Creatures = append(g.Level.Creatures, m)
		}
	}
}

// walkerFits reports whether p is open floor with nobody on it, the player
// included.
func (g *Game) walkerFits(p Pos) bool {
	return g.Level.Passable(p) && g.Level.CreatureAt(p) == nil && p != g.Player
}

// nearestFreeSpot is the C's find_nearest_free_spot: the first of the boxes
// of radius 0, 1, and 2 around c holding tiles that fit yields one of them at
// random; failing that, any fitting tile on the level does (the C's
// find_random_free_spot). ok is false only when nothing fits anywhere.
func (g *Game) nearestFreeSpot(c Pos, fits func(Pos) bool) (Pos, bool) {
	var cands []Pos
	for radius := 0; radius <= 2 && len(cands) == 0; radius++ {
		for dy := -radius; dy <= radius; dy++ {
			for dx := -radius; dx <= radius; dx++ {
				if p := (Pos{X: c.X + dx, Y: c.Y + dy}); g.Level.InBounds(p) && fits(p) {
					cands = append(cands, p)
				}
			}
		}
	}
	if len(cands) == 0 {
		for y := 0; y < g.Level.H; y++ {
			for x := 0; x < g.Level.W; x++ {
				if p := (Pos{X: x, Y: y}); fits(p) {
					cands = append(cands, p)
				}
			}
		}
	}
	if len(cands) == 0 {
		return Pos{}, false
	}
	return cands[g.RNG.Intn(len(cands))], true
}

// EnterStart places the player on the current (start) level's entry tile and
// marks it visited. Called once after the dungeon is built.
func (g *Game) EnterStart() {
	g.Player = g.Level.Start
	g.Level.entered = true
}

// LocationName is the display name of the player's current level (for the HUD
// and morgue), or "" when no dungeon is wired (some unit tests).
func (g *Game) LocationName() string {
	if g.Dungeon != nil {
		return g.Dungeon.Name()
	}
	return ""
}
