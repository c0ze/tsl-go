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
}

// Rune returns the tile's glyph as a rune. Glyph is guaranteed by validateTile
// to contain exactly one UTF-8 rune, so the decode always succeeds.
func (t *TileDef) Rune() rune {
	r, _ := utf8.DecodeRuneInString(t.Glyph)
	return r
}

// MonsterDef defines a kind of monster.
type MonsterDef struct {
	ID     string `toml:"-"`
	Name   string `toml:"name"`
	Glyph  string `toml:"glyph"`
	Color  Color  `toml:"color"`
	HP     int    `toml:"hp"`
	Attack int    `toml:"attack"`
	Dodge  int    `toml:"dodge"`
	Damage string `toml:"damage"` // dice spec, e.g. "1d4"
}

// Rune returns the monster's glyph as a rune.
func (m *MonsterDef) Rune() rune {
	r, _ := utf8.DecodeRuneInString(m.Glyph)
	return r
}

// ItemDef defines a kind of item.
type ItemDef struct {
	ID     string `toml:"-"`
	Name   string `toml:"name"`
	Glyph  string `toml:"glyph"`
	Color  Color  `toml:"color"`
	Kind   string `toml:"kind"`   // "potion", "weapon", or "armor"
	Use    string `toml:"use"`    // behavior name (potions)
	Power  int    `toml:"power"`  // potion magnitude (heal amount)
	Attack int    `toml:"attack"` // weapon attack bonus
	Dodge  int    `toml:"dodge"`  // armor dodge bonus
	Damage string `toml:"damage"` // weapon damage spec
}

// Rune returns the item's glyph as a rune.
func (i *ItemDef) Rune() rune {
	r, _ := utf8.DecodeRuneInString(i.Glyph)
	return r
}

var validItemKinds = map[string]bool{"potion": true, "weapon": true, "armor": true}

// Content is the fully-loaded, validated game content.
type Content struct {
	Tiles    map[string]*TileDef
	Monsters map[string]*MonsterDef
	Items    map[string]*ItemDef
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

// Load reads and validates all content from fsys (tiles.toml is required;
// monsters.toml and items.toml are optional).
func Load(fsys fs.FS) (*Content, error) {
	c := &Content{
		Tiles:    map[string]*TileDef{},
		Monsters: map[string]*MonsterDef{},
		Items:    map[string]*ItemDef{},
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
			c.Items[id] = def
		}
	}
	return c, nil
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
	case "weapon":
		if !validDamageSpec(i.Damage) {
			return fmt.Errorf("weapon damage %q is not a valid dice spec", i.Damage)
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
