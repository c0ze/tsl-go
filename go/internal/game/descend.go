package game

import (
	"fmt"
	"strings"
)

// LevelGen builds the level for a given depth. It is injected by cmd (which has
// access to the gen package), so game never imports gen.
type LevelGen func(depth int) (*Level, Pos, error)

// MaxDepth is how deep the dungeon goes; descending from the deepest level wins.
const MaxDepth = 3

// Descend takes the down-stairs under the player to the next level. Descending
// from the deepest level wins the game. It passes no monster turn (it changes
// levels), and does not itself recompute FOV — the game loop does that.
func (g *Game) Descend() {
	if g.Dead || g.Won {
		return
	}
	if g.Level.At(g.Player).Def.ID != "stairs_down" {
		g.log("There are no stairs to descend here.")
		return
	}
	if g.Depth >= MaxDepth {
		g.Won = true
		g.log("You descend the final stairs and escape the dungeon. You win!")
		return
	}
	if g.NewLevelFn == nil {
		g.log("The way down is sealed.")
		return
	}
	lvl, start, err := g.NewLevelFn(g.Depth + 1)
	if err != nil {
		g.log("The way down is blocked.")
		return
	}
	g.Depth++
	g.Level = lvl
	g.Player = start
	g.log("You descend to depth %d.", g.Depth)
}

// MorgueText returns a plain-text summary of the run. It performs no I/O.
func (g *Game) MorgueText() string {
	var b strings.Builder
	fmt.Fprintf(&b, "The Slimy Lichmummy — morgue\n\n")
	fmt.Fprintf(&b, "Depth reached: %d\n", g.Depth)
	fmt.Fprintf(&b, "HP: %d/%d\n", g.PlayerHP, g.PlayerMax)
	switch {
	case g.Dead:
		cause := g.DeathCause
		if cause == "" {
			cause = "unknown causes"
		}
		fmt.Fprintf(&b, "Killed by: %s\n", cause)
	case g.Won:
		fmt.Fprintf(&b, "Result: escaped victorious\n")
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
