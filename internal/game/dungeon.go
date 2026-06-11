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

// Travel takes the portal under the player to its target level, persisting the
// level being left and restoring (or first-placing) the player on arrival.
func (g *Game) Travel() {
	if g.Dead || g.Won {
		return
	}
	p := g.Level.PortalAt(g.Player)
	if p == nil {
		g.log("There are no stairs here.")
		return
	}
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
	def := g.Dungeon.defs[g.Level.ID]
	g.log("You enter %s.", def.Name)
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
