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
	// A blink springs whatever waits at the destination (C cast_blink's
	// activate_trap) — unless the traveller is floating above it.
	if tile := g.Level.At(g.Player).Def; tile.Effect != "" && !g.HasEffect("levitate") {
		g.springTrapAt(g.Player)
	}
	return true
}

// RevealMap marks every tile on the current level as Seen (remembered/dimmed)
// without making it currently Visible — the effect of a scroll of magic mapping.
func (g *Game) RevealMap() {
	for i := range g.Level.tiles {
		g.Level.tiles[i].Seen = true
	}
}

// ForgetMap wipes the automap — every tile forgotten, the inverse of RevealMap
// (C magic.c amnesia: memory[y][x] = gent_blank).
func (g *Game) ForgetMap() {
	for i := range g.Level.tiles {
		g.Level.tiles[i].Seen = false
	}
}

// RevealTraps exposes every disguised, unrevealed trap on the level and puts
// it on the automap (C magic.c reveal_traps), returning how many were found.
func (g *Game) RevealTraps() int {
	found := 0
	for i := range g.Level.tiles {
		t := &g.Level.tiles[i]
		if t.Disguise != nil && !t.Revealed {
			t.Revealed = true
			t.Seen = true
			found++
		}
	}
	return found
}

// MarkRecall pins the player's current level and position as the recall
// destination (C teleport.c cast_mark).
func (g *Game) MarkRecall() {
	g.recallLevel = g.Level.ID
	g.recallPos = g.Player
	g.recallSet = true
}

// Recall snaps the player back to the pinned mark, crossing levels through the
// Dungeon's persisted cache like stairs do (C cast_recall's change_level). It
// reports whether the recall happened: with no mark set it fizzles — the C
// would land at its zero-initialized location; we decline instead. If a
// creature now stands on the mark, the player lands one ring outward.
func (g *Game) Recall() bool {
	if !g.recallSet {
		return false
	}
	fromID := g.Level.ID
	switched := false
	if g.recallLevel != fromID {
		if g.Dungeon == nil {
			return false
		}
		g.Level.Return = g.Player
		if err := g.Dungeon.enter(g.recallLevel); err != nil {
			return false
		}
		g.Level = g.Dungeon.Current()
		g.Level.entered = true
		switched = true
	}
	dst, ok := g.freeSpotNear(g.recallPos)
	if !ok {
		// The mark is buried in bodies. A fizzle must leave no trace, so
		// undo the level switch (re-entering a cached level cannot fail).
		if switched {
			g.Dungeon.enter(fromID)
			g.Level = g.Dungeon.Current()
			g.Player = g.Level.Return
		}
		return false
	}
	g.Player = dst
	return true
}

// freeSpotNear returns p itself when unoccupied, else a free passable tile one
// ring outward; ok is false when every candidate is taken.
func (g *Game) freeSpotNear(p Pos) (Pos, bool) {
	if g.Level.CreatureAt(p) == nil {
		return p, true
	}
	for dy := -1; dy <= 1; dy++ {
		for dx := -1; dx <= 1; dx++ {
			q := Pos{X: p.X + dx, Y: p.Y + dy}
			if q != p && g.Level.Passable(q) && g.Level.CreatureAt(q) == nil {
				return q, true
			}
		}
	}
	return Pos{}, false
}
