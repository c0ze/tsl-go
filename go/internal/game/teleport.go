package game

// Teleport relocates the player to a uniformly random passable, unoccupied tile
// (never the current tile, a portal, or a creature's tile). It reports whether
// the player moved; with no valid destination it stays put and returns false.
func (g *Game) Teleport() bool {
	var candidates []Pos
	for y := 0; y < g.Level.H; y++ {
		for x := 0; x < g.Level.W; x++ {
			p := Pos{X: x, Y: y}
			if p == g.Player || !g.Level.Passable(p) || g.Level.CreatureAt(p) != nil || g.Level.PortalAt(p) != nil {
				continue
			}
			candidates = append(candidates, p)
		}
	}
	if len(candidates) == 0 {
		return false
	}
	g.Player = candidates[g.RNG.Intn(len(candidates))]
	return true
}

// RevealMap marks every tile on the current level as Seen (remembered/dimmed)
// without making it currently Visible — the effect of a scroll of magic mapping.
func (g *Game) RevealMap() {
	for i := range g.Level.tiles {
		g.Level.tiles[i].Seen = true
	}
}
