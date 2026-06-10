package game

// Talk is the C's interaction verb (actions.c interact): address the first
// adjacent creature. A chat line passes the turn; an empty square, a sleeping
// neighbor, or a wordless one costs nothing — except the wordless one still
// takes your turn, since you did stop to ask (the C passes the turn for any
// awake interaction; its no-line default is literally a BUG marker, so ours
// is gentler).
func (g *Game) Talk() {
	if g.Dead || g.Won {
		return
	}
	var who *Creature
	for _, c := range g.Level.Creatures {
		if chebyshev(g.Player, c.Pos) == 1 {
			who = c
			break
		}
	}
	if who == nil {
		g.log("There is no one to talk to.")
		return
	}
	if who.HasEffect("sleep") {
		g.log("The %s appears to be asleep.", who.Def.Name)
		return // no turn (C interact returns false)
	}
	if who.Def.Chat != "" {
		g.log("%s", who.Def.Chat)
	} else {
		g.log("You get no reply.")
	}
	g.advanceWorld()
}
