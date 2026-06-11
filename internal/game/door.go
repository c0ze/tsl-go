package game

// openDoor opens a closed door at p — a tile whose def names an OpensTo state —
// by replacing it with that open-state tile. It reports whether a door was
// actually opened, returning false for any non-door tile (e.g. a plain wall),
// so callers can use it as the "is this a door?" branch of a blocked step.
func (g *Game) openDoor(p Pos) bool {
	if !g.Level.InBounds(p) {
		return false
	}
	t := g.Level.At(p)
	if t.Def.OpensTo == "" {
		return false
	}
	open, ok := g.Content.Tiles[t.Def.OpensTo]
	if !ok {
		return false
	}
	g.Level.Set(p, open)
	return true
}
