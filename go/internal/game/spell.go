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
