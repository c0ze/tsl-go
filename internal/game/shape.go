package game

import (
	"fmt"
	"sort"
	"strings"
)

// PolymorphRandom turns the player into a random roster form for the given
// turns (C shapeshf.c shapeshift_random — any shape in the roster), shared by
// the potion and the polymorph trap. It returns the transform line.
func (g *Game) PolymorphRandom(turns int) string {
	ids := make([]string, 0, len(g.Content.Monsters))
	for id := range g.Content.Monsters {
		ids = append(ids, id)
	}
	if len(ids) == 0 {
		return "You feel briefly unlike yourself."
	}
	sort.Strings(ids)
	g.Shape = g.Content.Monsters[ids[g.RNG.Intn(len(ids))]]
	g.AddEffect("polymorph", turns)
	return fmt.Sprintf("You transform into %s!", withArticle(g.Shape.Name))
}

// withArticle prefixes a creature name with "a" or "an" (the C's name_one).
func withArticle(name string) string {
	if name != "" && strings.ContainsRune("aeiouAEIOU", rune(name[0])) {
		return "an " + name
	}
	return "a " + name
}
