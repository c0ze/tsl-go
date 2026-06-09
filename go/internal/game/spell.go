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
	g.monstersAct()
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
	g.monstersAct()
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
