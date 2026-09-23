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
		wield = g.DisplayName(g.Weapon)
	}
	if g.Armor != nil {
		worn = g.DisplayName(g.Armor)
	}
	fmt.Fprintf(&b, "Wielding: %s\nWearing: %s\n", wield, worn)
	if g.Ring != nil {
		fmt.Fprintf(&b, "On hand: %s\n", g.DisplayName(g.Ring))
	}
	if g.Amulet != nil {
		fmt.Fprintf(&b, "Around neck: %s\n", g.DisplayName(g.Amulet))
	}
	for _, s := range []struct {
		label string
		it    *Item
	}{{"On feet", g.Boots}, {"On head", g.Head}, {"Cloak", g.Cloak}} {
		if s.it != nil {
			fmt.Fprintf(&b, "%s: %s\n", s.label, g.DisplayName(s.it))
		}
	}
	fmt.Fprintf(&b, "Inventory (%d):\n", len(g.Inventory))
	for _, it := range g.Inventory {
		fmt.Fprintf(&b, "  - %s\n", g.DisplayName(it))
	}
	return b.String()
}
