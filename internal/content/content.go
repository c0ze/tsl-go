// Package content loads validated game content (data) from TOML files.
package content

import (
	"errors"
	"fmt"
	"io/fs"
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
	Damage      string `toml:"damage"`       // straight trap damage dice (the electrified plate, #18)
	OpensTo     string `toml:"opens_to"`     // tile id this becomes when opened ("" = not a door)
	ClosesTo    string `toml:"closes_to"`    // tile id this becomes when closed ("" = not closable)
	Secret      bool   `toml:"secret"`       // looks like a wall until bumped (C tile_door_secret_*)
	Locked      bool   `toml:"locked"`       // a key, a crowbar, or force opens it (C tile_door_locked)
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
	NoDoors     bool   `toml:"no_doors"`     // can't open doors (C attr_p_open_doors)
	Wound       int    `toml:"wound"`        // % chance a landed hit opens a bleeding wound (C attr_i_wound of its unarmed weapon)
	WoundImmune bool   `toml:"wound_immune"` // never bleeds (C attr_wound_immunity)
	DoorNoise   string `toml:"door_noise"`   // what a heard no_doors creature does to a door ("" = scratching on)
	Chat        string `toml:"chat"`         // the line it gives a t)alking player (C actions.c interact)
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
	SpeedMod    int    `toml:"speed_mod"`    // worn speed bonus/penalty (C attr_speed mods, #14)
	SwimSkill   int    `toml:"swim_skill"`   // worn swimming skill (C attr_swimming mods)
	GasImmune   bool   `toml:"gas_immune"`   // blocks gases — and eating/drinking (C gas mask)
	Blindfold   bool   `toml:"blindfold"`    // the wearer is simply blind (C attr_blindness)
	Stealth     int    `toml:"stealth"`      // shrinks monster notice range (C attr_stealth, approx.)
	Wound       int    `toml:"wound"`        // weapon: % chance a landed hit opens a bleeding wound (C attr_i_wound)
}

// kindWeights are the C's per-kind WEIGHT_* defaults (rules.h); an item with
// no explicit weight inherits its kind's.
var kindWeights = map[string]int{

	"potion": 7, "scroll": 4, "wand": 21, "food": 8, "light": 12,
	"spellbook": 12, "ring": 5, "amulet": 5, "weapon": 22, "armor": 40, "ammo": 1, "boots": 35, "head": 20, "cloak": 25, "tool": 1,
}

// Rune returns the item's glyph as a rune.
func (i *ItemDef) Rune() rune {
	r, _ := utf8.DecodeRuneInString(i.Glyph)
	return r
}

var validItemKinds = map[string]bool{"potion": true, "weapon": true, "armor": true, "food": true, "wand": true, "scroll": true, "light": true, "spellbook": true, "ring": true, "amulet": true, "ammo": true, "boots": true, "head": true, "cloak": true, "tool": true}

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

func decodeTOML(fsys fs.FS, name string, v any) error {
	b, err := fs.ReadFile(fsys, name)
	if err != nil {
		return fmt.Errorf("reading %s: %w", name, err)
	}
	return strictDecode(name, b, v)
}

// strictDecode decodes TOML into v and rejects keys v has no field for, so a
// misspelled key (say "permaswimm") fails the load instead of silently
// defaulting — the fail-fast rule the validators follow.
func strictDecode(name string, b []byte, v any) error {
	md, err := toml.Decode(string(b), v)
	if err != nil {
		return fmt.Errorf("parsing %s: %w", name, err)
	}
	if keys := md.Undecoded(); len(keys) > 0 {
		return fmt.Errorf("parsing %s: unknown key %q", name, keys[0].String())
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
	if err := strictDecode(name, b, v); err != nil {
		return false, err
	}
	return true, nil
}
