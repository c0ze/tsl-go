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

// revealSecretDoor handles a bump into a secret door: the C reveals it with
// "You find a secret door!" (player.c:1620) and maybe_locked_door rolls 50/50
// locked vs closed (doors.c). Reports whether one was revealed — the
// discovering bump costs the turn. (The C's blind two-bump discovery is not
// reproduced; we reveal on the first bump regardless.)
func (g *Game) revealSecretDoor(p Pos) bool {
	if !g.Level.InBounds(p) || !g.Level.At(p).Def.Secret {
		return false
	}
	g.log("You find a secret door!")
	if g.RNG.Intn(2) == 0 {
		g.Level.Set(p, g.Content.Tiles["door_locked"])
	} else {
		g.Level.Set(p, g.Content.Tiles["door_closed"])
	}
	return true
}

// bumpLockedDoor notes a bump into a locked door ("It is locked.",
// doors.c:278) and parks the position for the front-end's unlock/force prompt
// chain. The bump itself passes no turn; UnlockDoor/ForceDoor charge it.
func (g *Game) bumpLockedDoor(p Pos) bool {
	if !g.Level.InBounds(p) || !g.Level.At(p).Def.Locked {
		return false
	}
	g.log("It is locked.")
	g.lockedBump = &p
	return true
}

// TakeLockedBump returns and clears the position of the locked door the
// player just bumped, if any.
func (g *Game) TakeLockedBump() (Pos, bool) {
	if g.lockedBump == nil {
		return Pos{}, false
	}
	p := *g.lockedBump
	g.lockedBump = nil
	return p, true
}

// KeyCount is how many keys the player carries (the C stacks them and offers
// the count in the unlock prompt).
func (g *Game) KeyCount() int {
	n := 0
	for _, it := range g.Inventory {
		if it.Def != nil && it.Def.ID == "key" {
			n++
		}
	}
	return n
}

// HasCrowbar reports whether the player carries a crowbar — it changes both
// the force prompt and the outcome (doors.c:300).
func (g *Game) HasCrowbar() bool {
	for _, it := range g.Inventory {
		if it.Def != nil && it.Def.ID == "crowbar" {
			return true
		}
	}
	return false
}

// UnlockDoor spends one key on the locked door at p — the C's unlock_door
// destroys the key and sets the tile straight to open (doors.c:470).
func (g *Game) UnlockDoor(p Pos) {
	if g.Dead || g.Won || !g.Level.InBounds(p) || !g.Level.At(p).Def.Locked {
		return
	}
	for _, it := range g.Inventory {
		if it.Def != nil && it.Def.ID == "key" {
			g.removeInventory(it)
			g.Level.Set(p, g.Content.Tiles["door_open"])
			g.log("You unlock the door.")
			g.advanceWorld()
			return
		}
	}
}

// loudNoiseRadius approximates the C's draw_attention(noise_loud), which
// floods 60 tiles out from the source; a radius-7 disc is close enough.
const loudNoiseRadius = 7

func (g *Game) loudNoise(p Pos) {
	for _, m := range g.Level.Creatures {
		if chebyshev(p, m.Pos) <= loudNoiseRadius {
			m.RemoveEffect("sleep")
		}
	}
}

// ForceDoor breaks the locked door at p: a crowbar always works, bare hands
// at the C's roll_xn(5,3) = 5 in 8 (doors.c:307). Success destroys the door
// outright (the tile becomes floor); failure is loud. Either way the attempt
// costs the turn.
func (g *Game) ForceDoor(p Pos) {
	if g.Dead || g.Won || !g.Level.InBounds(p) || !g.Level.At(p).Def.Locked {
		return
	}
	switch {
	case g.HasCrowbar():
		g.log("You break the door open with your crowbar!")
		g.Level.Set(p, g.Content.Tiles["floor"])
	case g.RNG.Intn(8) < 5:
		g.log("You break the door open!")
		g.Level.Set(p, g.Content.Tiles["floor"])
	default:
		g.log("You fail to force the door.")
		g.loudNoise(p)
	}
	g.advanceWorld()
}

// CloseDoor closes the open door at p (the C's close_door): anything standing
// or lying in the doorway blocks it.
func (g *Game) CloseDoor(p Pos) {
	if g.Dead || g.Won || !g.Level.InBounds(p) {
		return
	}
	t := g.Level.At(p)
	if t.Def.ClosesTo == "" {
		if t.Def.OpensTo != "" {
			g.log("It is already closed.")
		} else {
			g.log("There is no door there.")
		}
		return
	}
	if g.Level.CreatureAt(p) != nil || g.Level.ItemAt(p) != nil || p == g.Player {
		g.log("There is something in the way.")
		return
	}
	closed, ok := g.Content.Tiles[t.Def.ClosesTo]
	if !ok {
		return
	}
	g.Level.Set(p, closed)
	g.advanceWorld()
}
