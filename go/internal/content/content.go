// Package content loads validated game content (data) from TOML files.
package content

import (
	"errors"
	"fmt"
	"io/fs"
	"strconv"
	"strings"
	"unicode/utf8"

	"github.com/BurntSushi/toml"
)

// Color is a named glyph color mirroring the original game's palette.
type Color string

const (
	ColorNormal  Color = "normal"
	ColorBrown   Color = "brown"
	ColorBlue    Color = "blue"
	ColorRed     Color = "red"
	ColorGreen   Color = "green"
	ColorCyan    Color = "cyan"
	ColorMagenta Color = "magenta"
	ColorBlack   Color = "black"
)

var validColors = map[Color]bool{
	ColorNormal: true, ColorBrown: true, ColorBlue: true, ColorRed: true,
	ColorGreen: true, ColorCyan: true, ColorMagenta: true, ColorBlack: true,
}

// TileDef defines a kind of map tile.
type TileDef struct {
	ID          string `toml:"-"`
	Glyph       string `toml:"glyph"`
	Color       Color  `toml:"color"`
	Passable    bool   `toml:"passable"`
	Transparent bool   `toml:"transparent"`
	Win         bool   `toml:"win"`          // stepping onto this tile wins (the ascension altar)
	Water       bool   `toml:"water"`        // deep water: impassable on foot, drowns waders (#18)
	Lava        bool   `toml:"lava"`         // molten rock: impassable on foot, burns per turn (#18)
	Effect      string `toml:"effect"`       // status effect applied when stepped on ("" = none)
	EffectTurns int    `toml:"effect_turns"` // duration of Effect
	OpensTo     string `toml:"opens_to"`     // tile id this becomes when opened ("" = not a door)
}

// Rune returns the tile's glyph as a rune. Glyph is guaranteed by validateTile
// to contain exactly one UTF-8 rune, so the decode always succeeds.
func (t *TileDef) Rune() rune {
	r, _ := utf8.DecodeRuneInString(t.Glyph)
	return r
}

// MonsterDef defines a kind of monster.
type MonsterDef struct {
	ID          string `toml:"-"`
	Name        string `toml:"name"`
	Glyph       string `toml:"glyph"`
	Color       Color  `toml:"color"`
	HP          int    `toml:"hp"`
	Attack      int    `toml:"attack"`
	Dodge       int    `toml:"dodge"`
	Damage      string `toml:"damage"`       // dice spec, e.g. "1d4"
	Speed       int    `toml:"speed"`        // energy gained per turn; <= 0 defaults to 100
	Corpse      string `toml:"corpse"`       // item id dropped on death ("" = none); must be a food item
	MinDepth    int    `toml:"min_depth"`    // earliest depth this monster spawns (0/1 = from depth 1)
	Ranged      int    `toml:"ranged"`       // ranged attack distance in tiles (0 = melee only)
	Swim        bool   `toml:"swim"`         // free_swim: may enter deep water (#18)
	Permaswim   bool   `toml:"permaswim"`    // water only — it won't leave its pool (implies swim)
	Effect      string `toml:"effect"`       // status effect a landed melee hit applies ("" = none)
	EffectTurns int    `toml:"effect_turns"` // duration of Effect
	Breath      string `toml:"breath"`       // cone attack: "fire", "poison", or "" (#19)
	Mimic       bool   `toml:"mimic"`        // spawns disguised as loot, rooted in place (#13)
}

// Rune returns the monster's glyph as a rune.
func (m *MonsterDef) Rune() rune {
	r, _ := utf8.DecodeRuneInString(m.Glyph)
	return r
}

// ItemDef defines a kind of item.
type ItemDef struct {
	ID          string `toml:"-"`
	Name        string `toml:"name"`
	Glyph       string `toml:"glyph"`
	Color       Color  `toml:"color"`
	Kind        string `toml:"kind"`         // "potion", "weapon", "armor", "food", or "wand"
	Use         string `toml:"use"`          // behavior name (potions/food)
	Power       int    `toml:"power"`        // potion/food heal magnitude, or wand charges
	Attack      int    `toml:"attack"`       // weapon attack bonus
	Dodge       int    `toml:"dodge"`        // armor dodge bonus
	Damage      string `toml:"damage"`       // weapon/wand damage spec
	Effect      string `toml:"effect"`       // status effect applied on use ("" = none); e.g. a venom wand
	EffectTurns int    `toml:"effect_turns"` // duration of Effect
	Ranged      int    `toml:"ranged"`       // weapon firing range in tiles (0 = melee only)
	Light       int    `toml:"light"`        // vision radius provided while carried (0 = none)
	Cost        int    `toml:"cost"`         // EP cost to cast (spellbooks)
	Beam        bool   `toml:"beam"`         // a spell that strikes every creature in a line
	NoSpawn     bool   `toml:"nospawn"`      // exclude from random floor loot (e.g. corpses)
	Weight      int    `toml:"weight"`       // carry weight (0 = kind default, C rules.h WEIGHT_*)
	Breath      string `toml:"breath"`       // spellbook cone: "fire", "poison", or "" (#19)
	Deathspell  bool   `toml:"deathspell"`   // the touch-range coin flip (C magic.c deathspell)
}

// kindWeights are the C's per-kind WEIGHT_* defaults (rules.h); an item with
// no explicit weight inherits its kind's.
var kindWeights = map[string]int{
	"potion": 7, "scroll": 4, "wand": 21, "food": 8, "light": 12,
	"spellbook": 12, "ring": 5, "amulet": 5, "weapon": 22, "armor": 40, "ammo": 1,
}

// Rune returns the item's glyph as a rune.
func (i *ItemDef) Rune() rune {
	r, _ := utf8.DecodeRuneInString(i.Glyph)
	return r
}

var validItemKinds = map[string]bool{"potion": true, "weapon": true, "armor": true, "food": true, "wand": true, "scroll": true, "light": true, "spellbook": true, "ring": true, "amulet": true, "ammo": true}

// SpawnEntry is one weighted entry in a level's monster spawn table.
type SpawnEntry struct {
	Monster string `toml:"monster"`
	Weight  int    `toml:"weight"`
}

// LevelDef defines a named dungeon level and its place in the graph.
type LevelDef struct {
	ID           string       `toml:"-"`
	Name         string       `toml:"name"`
	W            int          `toml:"width"`
	H            int          `toml:"height"`
	Start        bool         `toml:"start"` // the single entry level
	Links        []string     `toml:"links"` // ids of connected levels
	Monsters     int          `toml:"monsters"`
	Spawn        []SpawnEntry `toml:"spawn"`
	Altar        bool         `toml:"altar"`         // place an ascension altar (a win tile)
	Boss         string       `toml:"boss"`          // a guaranteed monster placed once on the level
	Retinue      string       `toml:"retinue"`       // escort monster spawned around the boss ("" = none)
	RetinueCount int          `toml:"retinue_count"` // how many escorts (C encounter_lurker spawns 8)
	Traps        int          `toml:"traps"`         // number of dart_trap tiles to scatter
	Water        int          `toml:"water"`         // number of water pools to carve (C level->water)
	Lava         int          `toml:"lava"`          // number of lava pools to carve (C level->lava)
	Doors        bool         `toml:"doors"`         // place closed doors in room doorways
	Dark         bool         `toml:"dark"`          // unlit level: the player sees only a small radius
}

// Content is the fully-loaded, validated game content.
type Content struct {
	Tiles    map[string]*TileDef
	Monsters map[string]*MonsterDef
	Items    map[string]*ItemDef
	Levels   map[string]*LevelDef
}

type tilesFile struct {
	Tile map[string]*TileDef `toml:"tile"`
}

type monstersFile struct {
	Monster map[string]*MonsterDef `toml:"monster"`
}

type itemsFile struct {
	Item map[string]*ItemDef `toml:"item"`
}

type levelsFile struct {
	Level map[string]*LevelDef `toml:"level"`
}

// Load reads and validates all content from fsys (tiles.toml is required;
// monsters.toml and items.toml are optional).
func Load(fsys fs.FS) (*Content, error) {
	c := &Content{
		Tiles:    map[string]*TileDef{},
		Monsters: map[string]*MonsterDef{},
		Items:    map[string]*ItemDef{},
		Levels:   map[string]*LevelDef{},
	}

	var tf tilesFile
	if err := decodeTOML(fsys, "tiles.toml", &tf); err != nil {
		return nil, err
	}
	for id, def := range tf.Tile {
		def.ID = id
		if err := validateTile(def); err != nil {
			return nil, fmt.Errorf("tile %q: %w", id, err)
		}
		c.Tiles[id] = def
	}
	if len(c.Tiles) == 0 {
		return nil, fmt.Errorf("tiles.toml: no tiles defined")
	}
	if err := validateTileRefs(c); err != nil {
		return nil, err
	}

	var mf monstersFile
	if ok, err := decodeOptionalTOML(fsys, "monsters.toml", &mf); err != nil {
		return nil, err
	} else if ok {
		for id, def := range mf.Monster {
			def.ID = id
			if err := validateMonster(def); err != nil {
				return nil, fmt.Errorf("monster %q: %w", id, err)
			}
			c.Monsters[id] = def
		}
	}

	var inf itemsFile
	if ok, err := decodeOptionalTOML(fsys, "items.toml", &inf); err != nil {
		return nil, err
	} else if ok {
		for id, def := range inf.Item {
			def.ID = id
			if err := validateItem(def); err != nil {
				return nil, fmt.Errorf("item %q: %w", id, err)
			}
			if def.Weight == 0 {
				def.Weight = kindWeights[def.Kind]
			}
			c.Items[id] = def
		}
	}

	var lf levelsFile
	if ok, err := decodeOptionalTOML(fsys, "levels.toml", &lf); err != nil {
		return nil, err
	} else if ok {
		for id, def := range lf.Level {
			def.ID = id
			c.Levels[id] = def
		}
	}

	if err := validateCorpseRefs(c); err != nil {
		return nil, err
	}
	if err := validateLevels(c); err != nil {
		return nil, err
	}
	return c, nil
}

// validateTileRefs checks that every tile's opens_to (when set) names a defined
// tile, so a door always has an open state to become.
func validateTileRefs(c *Content) error {
	for id, t := range c.Tiles {
		if t.OpensTo == "" {
			continue
		}
		if _, ok := c.Tiles[t.OpensTo]; !ok {
			return fmt.Errorf("tile %q: opens_to %q is not a defined tile", id, t.OpensTo)
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

func decodeTOML(fsys fs.FS, name string, v any) error {
	b, err := fs.ReadFile(fsys, name)
	if err != nil {
		return fmt.Errorf("reading %s: %w", name, err)
	}
	if _, err := toml.Decode(string(b), v); err != nil {
		return fmt.Errorf("parsing %s: %w", name, err)
	}
	return nil
}

// decodeOptionalTOML decodes name if present; (false, nil) if it does not exist.
func decodeOptionalTOML(fsys fs.FS, name string, v any) (bool, error) {
	b, err := fs.ReadFile(fsys, name)
	if errors.Is(err, fs.ErrNotExist) {
		return false, nil
	}
	if err != nil {
		return false, fmt.Errorf("reading %s: %w", name, err)
	}
	if _, err := toml.Decode(string(b), v); err != nil {
		return false, fmt.Errorf("parsing %s: %w", name, err)
	}
	return true, nil
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
