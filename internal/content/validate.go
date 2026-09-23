package content

// Content validation: every def and cross-reference is checked at load so
// bad data fails fast instead of mid-game.

import (
	"fmt"
	"strconv"
	"strings"
	"unicode/utf8"
)

// validateTileRefs checks that every tile's opens_to / closes_to (when set)
// names a defined tile, so a door always has a state to become.
func validateTileRefs(c *Content) error {
	for id, t := range c.Tiles {
		if t.OpensTo != "" {
			if _, ok := c.Tiles[t.OpensTo]; !ok {
				return fmt.Errorf("tile %q: opens_to %q is not a defined tile", id, t.OpensTo)
			}
		}
		if t.ClosesTo != "" {
			if _, ok := c.Tiles[t.ClosesTo]; !ok {
				return fmt.Errorf("tile %q: closes_to %q is not a defined tile", id, t.ClosesTo)
			}
		}
	}
	return nil
}

// validateCorpseRefs checks that every monster's corpse (when set) names a
// defined food item, so bad content fails at load instead of at the kill.
func validateCorpseRefs(c *Content) error {
	for id, m := range c.Monsters {
		if m.Corpse == "" {
			continue
		}
		it, ok := c.Items[m.Corpse]
		if !ok {
			return fmt.Errorf("monster %q: corpse %q is not a defined item", id, m.Corpse)
		}
		if it.Kind != "food" {
			return fmt.Errorf("monster %q: corpse %q must be a food item, got kind %q", id, m.Corpse, it.Kind)
		}
	}
	return nil
}

// validateLevels checks the level graph: exactly one start level, links and
// spawn references resolve, and sizes/weights are sane. A dungeon with no
// levels defined is allowed (the file is optional).
func validateLevels(c *Content) error {
	if len(c.Levels) == 0 {
		return nil
	}
	starts := 0
	for id, l := range c.Levels {
		if l.Start {
			starts++
		}
		if l.W < 12 || l.H < 8 {
			return fmt.Errorf("level %q: too small (%dx%d), need at least 12x8", id, l.W, l.H)
		}
		if l.Monsters < 0 {
			return fmt.Errorf("level %q: monsters must be >= 0, got %d", id, l.Monsters)
		}
		for _, t := range l.Links {
			if _, ok := c.Levels[t]; !ok {
				return fmt.Errorf("level %q: link to unknown level %q", id, t)
			}
		}
		for _, s := range l.Spawn {
			if _, ok := c.Monsters[s.Monster]; !ok {
				return fmt.Errorf("level %q: spawn references unknown monster %q", id, s.Monster)
			}
			if s.Weight < 1 {
				return fmt.Errorf("level %q: spawn weight for %q must be >= 1, got %d", id, s.Monster, s.Weight)
			}
		}
		if l.Monsters > 0 && len(l.Spawn) == 0 {
			return fmt.Errorf("level %q: monsters > 0 but spawn table is empty", id)
		}
		if l.Boss != "" {
			if _, ok := c.Monsters[l.Boss]; !ok {
				return fmt.Errorf("level %q: boss %q is not a defined monster", id, l.Boss)
			}
		}
		if l.RetinueCount < 0 {
			return fmt.Errorf("level %q: retinue_count must be >= 0, got %d", id, l.RetinueCount)
		}
		if l.Retinue != "" {
			if _, ok := c.Monsters[l.Retinue]; !ok {
				return fmt.Errorf("level %q: retinue %q is not a defined monster", id, l.Retinue)
			}
			if l.Boss == "" {
				return fmt.Errorf("level %q: retinue set without a boss to escort", id)
			}
		}
		if l.RetinueCount > 0 && l.Retinue == "" {
			return fmt.Errorf("level %q: retinue_count > 0 without a retinue monster", id)
		}
		if l.Altar {
			if t, ok := c.Tiles["altar"]; !ok || !t.Win {
				return fmt.Errorf("level %q: altar set but no win tile %q is defined", id, "altar")
			}
		}
		if l.Traps < 0 {
			return fmt.Errorf("level %q: traps must be >= 0, got %d", id, l.Traps)
		}
		if l.Traps > 0 {
			if t, ok := c.Tiles["dart_trap"]; !ok || t.Effect == "" {
				return fmt.Errorf("level %q: traps set but no dart_trap effect tile is defined", id)
			}
		}
		if l.Water < 0 {
			return fmt.Errorf("level %q: water must be >= 0, got %d", id, l.Water)
		}
		if l.Water > 0 {
			if t, ok := c.Tiles["water"]; !ok || !t.Water {
				return fmt.Errorf("level %q: water pools set but no water tile is defined", id)
			}
		}
		if l.Lava < 0 {
			return fmt.Errorf("level %q: lava must be >= 0, got %d", id, l.Lava)
		}
		if l.Lava > 0 {
			if t, ok := c.Tiles["lava"]; !ok || !t.Lava {
				return fmt.Errorf("level %q: lava pools set but no lava tile is defined", id)
			}
		}
	}
	if starts != 1 {
		return fmt.Errorf("levels: need exactly one start level, found %d", starts)
	}
	return nil
}

func validateTile(t *TileDef) error {
	if utf8.RuneCountInString(t.Glyph) != 1 {
		return fmt.Errorf("glyph must be exactly one character, got %q", t.Glyph)
	}
	if !validColors[t.Color] {
		return fmt.Errorf("invalid color %q", t.Color)
	}
	if t.Effect != "" && t.EffectTurns <= 0 {
		return fmt.Errorf("tile effect %q needs effect_turns > 0", t.Effect)
	}
	if t.Damage != "" && !validDamageSpec(t.Damage) {
		return fmt.Errorf("tile damage %q is not a valid dice spec", t.Damage)
	}
	return nil
}

func validateMonster(m *MonsterDef) error {
	if utf8.RuneCountInString(m.Glyph) != 1 {
		return fmt.Errorf("glyph must be exactly one character, got %q", m.Glyph)
	}
	if !validColors[m.Color] {
		return fmt.Errorf("invalid color %q", m.Color)
	}
	if m.HP < 1 {
		return fmt.Errorf("hp must be >= 1, got %d", m.HP)
	}
	if m.Attack < 0 {
		return fmt.Errorf("attack must be >= 0, got %d", m.Attack)
	}
	if m.Dodge < 0 {
		return fmt.Errorf("dodge must be >= 0, got %d", m.Dodge)
	}
	if !validDamageSpec(m.Damage) {
		return fmt.Errorf("damage %q is not a valid dice spec", m.Damage)
	}
	if m.MinDepth < 0 {
		return fmt.Errorf("min_depth must be >= 0, got %d", m.MinDepth)
	}
	if m.Ranged < 0 {
		return fmt.Errorf("ranged must be >= 0, got %d", m.Ranged)
	}
	if m.Permaswim && !m.Swim {
		return fmt.Errorf("permaswim requires swim")
	}
	if m.Breath != "" && m.Breath != "fire" && m.Breath != "poison" {
		return fmt.Errorf("breath must be fire or poison, got %q", m.Breath)
	}
	if m.Effect != "" && m.EffectTurns <= 0 {
		return fmt.Errorf("melee effect %q needs effect_turns > 0", m.Effect)
	}
	return nil
}

func validateItem(i *ItemDef) error {
	if utf8.RuneCountInString(i.Glyph) != 1 {
		return fmt.Errorf("glyph must be exactly one character, got %q", i.Glyph)
	}
	if !validColors[i.Color] {
		return fmt.Errorf("invalid color %q", i.Color)
	}
	if !validItemKinds[i.Kind] {
		return fmt.Errorf("invalid kind %q", i.Kind)
	}
	switch i.Kind {
	case "potion":
		if strings.TrimSpace(i.Use) == "" {
			return fmt.Errorf("potion must have a non-empty use")
		}
	case "scroll":
		if strings.TrimSpace(i.Use) == "" {
			return fmt.Errorf("scroll must have a non-empty use")
		}
	case "light":
		if i.Light <= 0 {
			return fmt.Errorf("light item must provide light > 0")
		}
	case "spellbook":
		hasUse := strings.TrimSpace(i.Use) != ""
		hasAttack := i.Ranged > 0 && i.Damage != ""
		if !hasUse && !hasAttack && i.Breath == "" && !i.Deathspell {
			return fmt.Errorf("spellbook needs a use behavior, a ranged damage spec, a breath, or the deathspell")
		}
		if hasAttack && !validDamageSpec(i.Damage) {
			return fmt.Errorf("spellbook damage %q is not a valid dice spec", i.Damage)
		}
		if i.Cost <= 0 {
			return fmt.Errorf("spellbook must have an EP cost > 0")
		}
	case "weapon":
		if !validDamageSpec(i.Damage) {
			return fmt.Errorf("weapon damage %q is not a valid dice spec", i.Damage)
		}
	case "wand":
		if i.Damage == "" && i.Effect == "" {
			return fmt.Errorf("wand must have a damage spec or an effect")
		}
		if i.Damage != "" && !validDamageSpec(i.Damage) {
			return fmt.Errorf("wand damage %q is not a valid dice spec", i.Damage)
		}
	case "food":
		if strings.TrimSpace(i.Use) == "" {
			return fmt.Errorf("food must have a non-empty use")
		}
		if i.Power < 0 {
			return fmt.Errorf("food power must be >= 0, got %d", i.Power)
		}
	case "ring", "amulet":
		if i.Attack <= 0 && i.Dodge <= 0 {
			return fmt.Errorf("%s must grant an attack or dodge bonus", i.Kind)
		}
	}
	if i.Effect != "" && i.EffectTurns <= 0 {
		return fmt.Errorf("item effect %q needs effect_turns > 0", i.Effect)
	}
	if i.Ranged < 0 {
		return fmt.Errorf("ranged must be >= 0, got %d", i.Ranged)
	}
	if i.Weight < 0 {
		return fmt.Errorf("weight must be >= 0, got %d", i.Weight)
	}
	if i.Breath != "" && i.Breath != "fire" && i.Breath != "poison" {
		return fmt.Errorf("breath must be fire or poison, got %q", i.Breath)
	}
	if i.Light < 0 {
		return fmt.Errorf("light must be >= 0, got %d", i.Light)
	}
	if i.Beam { // a beam is a spellbook line attack: it needs the attack fields
		if i.Kind != "spellbook" {
			return fmt.Errorf("only a spellbook may be a beam")
		}
		if i.Ranged <= 0 || !validDamageSpec(i.Damage) {
			return fmt.Errorf("a beam spellbook needs ranged > 0 and a valid damage spec")
		}
	}
	return nil
}

// validDamageSpec reports whether s is a well-formed dice spec "NdS" or
// "NdS+M" / "NdS-M" (matching rng.RollSpec's grammar).
func validDamageSpec(s string) bool {
	d := strings.IndexByte(s, 'd')
	if d <= 0 || d >= len(s)-1 {
		return false
	}
	if n, err := strconv.Atoi(s[:d]); err != nil || n < 0 {
		return false
	}
	rest := s[d+1:]
	if rest[0] == '+' || rest[0] == '-' {
		return false
	}
	if i := strings.IndexAny(rest[1:], "+-"); i >= 0 {
		i++ // account for the rest[1:] offset
		if _, err := strconv.Atoi(rest[i:]); err != nil {
			return false
		}
		rest = rest[:i]
	}
	if sides, err := strconv.Atoi(rest); err != nil || sides < 1 {
		return false
	}
	return true
}
