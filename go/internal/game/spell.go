package game

// epRegenInterval is how many turns pass per point of EP regained.
const epRegenInterval = 3

// SpellInventory returns the spellbooks the player is carrying, in inventory
// order — the spells they can currently cast.
func (g *Game) SpellInventory() []*Item {
	var out []*Item
	for _, it := range g.Inventory {
		if it.Def != nil && it.Def.Kind == "spellbook" {
			out = append(out, it)
		}
	}
	return out
}

// CastSpell casts the spell in book: it spends the book's EP cost, invokes the
// spell's behavior, and passes a turn. A spell you can't afford is refused
// without spending EP or a turn.
func (g *Game) CastSpell(book *Item) {
	if g.Dead || g.Won {
		return
	}
	if book == nil || book.Def == nil || book.Def.Kind != "spellbook" {
		return
	}
	if g.EP < book.Def.Cost {
		g.log("You lack the energy to cast %s.", book.Def.Name)
		return
	}
	g.EP -= book.Def.Cost
	if b, ok := g.Behaviors[book.Def.Use]; ok {
		g.Messages = append(g.Messages, b(g, book)...)
	} else {
		g.log("The spell fizzles.")
	}
	g.advanceWorld()
}

// CastSpellAt casts a targeted attack spell from book at target: it checks EP,
// range, and line of sight (any failure refuses the cast without spending EP or
// a turn), then spends the EP cost, rolls the spell's damage against the creature
// there (killing it at 0 HP) or fizzles into empty space, and passes a turn.
func (g *Game) CastSpellAt(book *Item, target Pos) {
	if g.Dead || g.Won {
		return
	}
	if book == nil || book.Def == nil || book.Def.Kind != "spellbook" {
		return
	}
	if g.EP < book.Def.Cost {
		g.log("You lack the energy to cast %s.", book.Def.Name)
		return
	}
	if book.Def.Breath != "" { // a breath book exhales a cone toward the target
		g.EP -= book.Def.Cost
		g.playerBreathe(book.Def.Breath, target)
		g.advanceWorld()
		return
	}
	if book.Def.Beam { // a beam fires in the aimed direction, hitting all in its path
		g.EP -= book.Def.Cost
		g.fireBeam(book, target)
		g.advanceWorld()
		return
	}
	if chebyshev(g.Player, target) > book.Def.Ranged {
		g.log("That is out of range.")
		return
	}
	if !g.lineOfSight(g.Player, target) {
		g.log("You don't have a clear shot.")
		return
	}
	g.EP -= book.Def.Cost
	if m := g.Level.CreatureAt(target); m != nil {
		dmg := g.RNG.RollSpec(book.Def.Damage)
		m.HP -= dmg
		g.log("Your %s strikes the %s for %d.", book.Def.Name, m.Def.Name, dmg)
		if m.HP <= 0 {
			g.killCreature(m)
		}
	} else {
		g.log("The spell dissipates against nothing.")
	}
	g.advanceWorld()
}

// FlashRadius is how far the flash spell's blinding light reaches.
const FlashRadius = 4

// NoxiousRadius is how far the noxious cloud spell's poison spreads — tighter
// than the flash, since it deals damage rather than only controlling.
const NoxiousRadius = 3

// affectNearby applies status effect `kind` for `turns` to every creature within
// Chebyshev `radius` of the player, returning how many were affected — the shared
// core of the player's self-centred area spells.
func (g *Game) affectNearby(radius int, kind string, turns int) int {
	n := 0
	for _, m := range g.Level.Creatures {
		if chebyshev(m.Pos, g.Player) <= radius {
			m.AddEffect(kind, turns)
			n++
		}
	}
	return n
}

// FlashBlind blinds every creature within radius of the player for turns and
// reports how many were blinded — the flash spell's area effect.
func (g *Game) FlashBlind(radius, turns int) int {
	return g.affectNearby(radius, "blind", turns)
}

// PoisonNearby poisons every creature within radius of the player for turns and
// reports how many were caught — the noxious cloud spell's area effect. Poison
// then drains 1 HP/turn via tickCreatureEffects until it fades or kills.
func (g *Game) PoisonNearby(radius, turns int) int {
	return g.affectNearby(radius, "poison", turns)
}

// ScareRadius is how far the scroll of scare monster's terror spreads.
const ScareRadius = 4

// ScareNearby frightens every creature within radius of the player for turns and
// reports how many fled — the scroll of scare monster's area effect. A
// frightened creature flees the player (stepAway) instead of attacking.
func (g *Game) ScareNearby(radius, turns int) int {
	return g.affectNearby(radius, "fear", turns)
}

// fireBeam traces a straight 8-directional ray from the player toward target, up
// to the spell's range, damaging every creature it crosses (killing at 0 HP) and
// stopping at the first opaque tile — a wall or a closed door.
func (g *Game) fireBeam(book *Item, target Pos) {
	dx, dy := signOf(target.X-g.Player.X), signOf(target.Y-g.Player.Y)
	if dx == 0 && dy == 0 {
		g.log("The %s fizzles.", book.Def.Name)
		return
	}
	hits := 0
	x, y := g.Player.X, g.Player.Y
	for i := 0; i < book.Def.Ranged; i++ {
		x += dx
		y += dy
		p := Pos{X: x, Y: y}
		if !g.Level.InBounds(p) || !g.Level.At(p).Def.Transparent {
			break // a wall (or the level edge) stops the ray
		}
		if m := g.Level.CreatureAt(p); m != nil {
			dmg := g.RNG.RollSpec(book.Def.Damage)
			m.HP -= dmg
			g.log("Your %s rakes the %s for %d.", book.Def.Name, m.Def.Name, dmg)
			if m.HP <= 0 {
				g.killCreature(m)
			}
			hits++
		}
	}
	if hits == 0 {
		g.log("The ray strikes nothing.")
	}
}

// signOf returns -1, 0, or +1 matching the sign of n.
func signOf(n int) int {
	switch {
	case n > 0:
		return 1
	case n < 0:
		return -1
	default:
		return 0
	}
}

// regenEP restores one EP every epRegenInterval turns, clamped to EPMax. Called
// once per player turn. The counter pauses while EP is full.
func (g *Game) regenEP() {
	if g.EP >= g.EPMax {
		return
	}
	g.epTurn++
	if g.epTurn%epRegenInterval == 0 {
		g.EP++
	}
}
