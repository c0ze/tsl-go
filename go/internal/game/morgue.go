package game

import (
	"fmt"
	"strings"
)

// MorgueText returns a plain-text summary of the run. It performs no I/O.
func (g *Game) MorgueText() string {
	var b strings.Builder
	fmt.Fprintf(&b, "The Slimy Lichmummy — morgue\n\n")
	loc := g.LocationName()
	if loc == "" {
		loc = "the dungeon"
	}
	fmt.Fprintf(&b, "Location: %s\n", loc)
	fmt.Fprintf(&b, "HP: %d/%d\n", g.PlayerHP, g.PlayerMax)
	switch {
	case g.Dead:
		cause := g.DeathCause
		if cause == "" {
			cause = "unknown causes"
		}
		fmt.Fprintf(&b, "Killed by: %s\n", cause)
	case g.Won:
		fmt.Fprintf(&b, "Result: ascended to demigodhood\n")
	}
	wield, worn := "nothing", "nothing"
	if g.Weapon != nil {
		wield = g.Weapon.Def.Name
	}
	if g.Armor != nil {
		worn = g.Armor.Def.Name
	}
	fmt.Fprintf(&b, "Wielding: %s\nWearing: %s\n", wield, worn)
	fmt.Fprintf(&b, "Inventory (%d):\n", len(g.Inventory))
	for _, it := range g.Inventory {
		fmt.Fprintf(&b, "  - %s\n", it.Def.Name)
	}
	return b.String()
}
