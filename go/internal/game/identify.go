package game

import "sort"

// appearancePools are the cosmetic names an unidentified item of each kind can
// wear, ported verbatim from the original common/rndnames.c. Identifying a type
// (by using it, or later a scroll of identify) reveals the real name for every
// item of that type.
var appearancePools = map[string][]string{
	"potion": {
		"muddy potion", "blood-red potion", "slimy green potion", "milky potion",
		"clear potion", "clotted potion", "golden potion", "silver potion",
		"shimmering potion", "brown potion", "sickly yellow potion",
		"effervescent potion", "bubbly potion", "rusty potion", "black potion",
		"crystal bottle", "cyan potion", "tear-shaped vial", "small crystal flask",
		"violet potion",
	},
	"scroll": {
		"scroll labeled DOOM", "scroll labeled HAHAHA", "scroll labeled CURSES",
		"scroll labeled FOR MY LOVE", "scroll labeled READ ME", "scroll labeled LOLWUT",
		"scroll labeled MORE", "scroll labeled LESS", "scroll labeled DEAR JOURNAL",
		"scroll labeled THE END", "scroll labeled TROLOLOLO", "scroll labeled WEXJUU",
		"scroll labeled VIED EMPINA", "scroll labeled SCIZZORZ", "scroll labeled SCROLLORZ",
		"scroll labeled KAOZ", "scroll labeled SIGSEGV", "scroll labeled MOVE ZIG",
		"scroll labeled INVOICE", "scroll labeled Q9000",
	},
	"wand": {
		"bone wand", "silver wand", "iron wand", "golden wand", "crystal wand",
		"ornamented wand", "jewelled wand", "black wand", "wooden wand", "ivory wand",
		"barbed wand", "peculiar wand", "twisted wand", "plain wand",
	},
}

// unidentifiable reports whether an item kind hides behind an appearance until
// its type is identified.
func unidentifiable(kind string) bool { return appearancePools[kind] != nil }

// AssignAppearances shuffles a cosmetic appearance onto every unidentifiable item
// type (potion/scroll/wand) for this game, so the appearance->effect mapping is
// randomized per seed. Ids are sorted before the Fisher-Yates shuffle, so a given
// RNG seed always yields the same assignment. Safe to call before content is set
// (it just produces empty maps), and leaving the maps nil reads as "everything
// identified", so the engine never crashes pre-init.
func (g *Game) AssignAppearances() {
	g.appearances = map[string]string{}
	if g.Identified == nil {
		g.Identified = map[string]bool{}
	}
	if g.Content == nil {
		return
	}
	byKind := map[string][]string{}
	for id, def := range g.Content.Items {
		if def != nil && unidentifiable(def.Kind) {
			byKind[def.Kind] = append(byKind[def.Kind], id)
		}
	}
	kinds := make([]string, 0, len(byKind))
	for kind := range byKind {
		kinds = append(kinds, kind)
	}
	sort.Strings(kinds) // process kinds in a fixed order so RNG use is deterministic
	for _, kind := range kinds {
		ids := byKind[kind]
		sort.Strings(ids)
		pool := append([]string(nil), appearancePools[kind]...)
		for i := len(pool) - 1; i > 0; i-- { // Fisher-Yates with the game RNG
			j := g.RNG.Intn(i + 1)
			pool[i], pool[j] = pool[j], pool[i]
		}
		for i, id := range ids {
			g.appearances[id] = pool[i%len(pool)]
		}
	}
}

// DisplayName is the name shown to the player: the item's shuffled appearance
// while its type is unidentified, otherwise its true name.
func (g *Game) DisplayName(it *Item) string {
	if it == nil || it.Def == nil {
		return "something"
	}
	if unidentifiable(it.Def.Kind) && !g.Identified[it.Def.ID] {
		if app, ok := g.appearances[it.Def.ID]; ok {
			return app
		}
	}
	return it.Def.Name
}

// identify marks an item's type as globally known (use-identify), announcing the
// reveal the first time it happens. A no-op for kinds that are never hidden.
func (g *Game) identify(it *Item) {
	if it == nil || it.Def == nil || !unidentifiable(it.Def.Kind) {
		return
	}
	if g.Identified == nil {
		g.Identified = map[string]bool{}
	}
	if g.Identified[it.Def.ID] {
		return
	}
	g.Identified[it.Def.ID] = true
	g.log("It was a %s!", it.Def.Name)
}
