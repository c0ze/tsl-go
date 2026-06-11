package game

// Spell memorization (#21, C reading.c read_book): a spellbook is read once
// and consumed — half the time you learn its spell permanently, half the
// time the book turns on you. Casting draws on Known, not the pack.

// hungryBookDamage approximates the C's virtual_hungry_book chain
// (1/1/1/1/1/2 — avg 1.17, max 2) as the max-preserving 1d2.
const (
	hungryBookDamage = "1d2"
	hungryBookTurns  = 100 // C rules.h HUNGRY_BOOK_LIFETIME
	darknessTurns    = 33  // C rules.h DEFAULT_BLIND_TIME
)

// readBook is the C's read_book: identify, refuse what's known, then the
// coin flip — learn (and grow) or suffer. The book burns up either way.
func (g *Game) readBook(book *Item) {
	g.identify(book)
	if g.Known[book.Def.ID] {
		g.log("You already know this.")
		return // free, and the book survives (C returns false)
	}
	if g.RNG.Intn(2) == 0 {
		g.log("This book is difficult to understand!")
		g.badBook(book)
	} else {
		if g.Known == nil {
			g.Known = map[string]bool{}
		}
		g.Known[book.Def.ID] = true
		g.EPMax++ // learning enlarges the mind (C: attr_ep_max += 1)
		g.log("You learn %s.", book.Def.Name)
	}
	g.removeInventory(book)
	g.advanceWorld()
}

// badBook is the 1d3 punishment for a difficult book (C reading.c bad_book).
func (g *Game) badBook(book *Item) {
	switch g.RNG.Intn(3) {
	case 0:
		g.hungryBook()
	case 1:
		if !g.HasEffect("blind") {
			g.darknessFalls()
			return
		}
		fallthrough // already blind: the C falls through to the escapee
	default:
		g.bookEscapee(book)
	}
}

// hungryBook latches a wretched temp weapon onto the reader's hands
// (C virtual_hungry_book via set_temp_weapon).
func (g *Game) hungryBook() {
	g.log("The book bites into your hand!")
	g.AddEffect("hungry_book", hungryBookTurns)
}

// darknessFalls blinds the reader (C effect_blindness, DEFAULT_BLIND_TIME).
func (g *Game) darknessFalls() {
	g.log("Darkness falls around you!")
	g.AddEffect("blind", darknessTurns)
}

// bookEscapee frees something hostile from the pages: a flame spirit from
// Breathe Fire, a frostling from Frost Ray, else a 1-in-4 nameless horror or
// an imp — with the summon lifetime, so it eventually returns to the void.
func (g *Game) bookEscapee(book *Item) {
	id := "imp"
	switch {
	case book.Def.ID == "book_breathe_fire":
		id = "flame_spirit"
	case book.Def.ID == "spellbook_frost_ray":
		id = "frostling"
	case g.RNG.Intn(2) == 0 && g.RNG.Intn(2) == 0:
		id = "nameless_horror"
	}
	def := g.Content.Monsters[id]
	if def == nil {
		return // content without the escapee: the pages stay shut
	}
	for radius := 1; radius <= 3; radius++ {
		for dy := -radius; dy <= radius; dy++ {
			for dx := -radius; dx <= radius; dx++ {
				p := Pos{X: g.Player.X + dx, Y: g.Player.Y + dy}
				if chebyshev(p, g.Player) != radius {
					continue
				}
				if g.Level.Passable(p) && g.Level.CreatureAt(p) == nil {
					g.Level.Creatures = append(g.Level.Creatures, &Creature{Def: def, Pos: p, HP: def.HP, Lifetime: 500})
					g.log("The %s escapes from the book!", def.Name)
					return
				}
			}
		}
	}
}
