// Package content loads validated game content (data) from TOML files.
package content

import (
	"errors"
	"fmt"
	"os"
	"path/filepath"
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

// Content is the fully-loaded, validated game content.
type Content struct {
	Tiles    map[string]*TileDef
	Monsters map[string]*MonsterDef
}

type tilesFile struct {
	Tile map[string]*TileDef `toml:"tile"`
}

type monstersFile struct {
	Monster map[string]*MonsterDef `toml:"monster"`
}

// Load reads and validates all content files in dir.
func Load(dir string) (*Content, error) {
	c := &Content{Tiles: map[string]*TileDef{}}

	var tf tilesFile
	path := filepath.Join(dir, "tiles.toml")
	if _, err := toml.DecodeFile(path, &tf); err != nil {
		return nil, fmt.Errorf("loading %s: %w", path, err)
	}
	for id, def := range tf.Tile {
		def.ID = id
		if err := validateTile(def); err != nil {
			return nil, fmt.Errorf("tile %q in %s: %w", id, path, err)
		}
		c.Tiles[id] = def
	}
	if len(c.Tiles) == 0 {
		return nil, fmt.Errorf("%s: no tiles defined", path)
	}
	c.Monsters = map[string]*MonsterDef{}
	mpath := filepath.Join(dir, "monsters.toml")
	if _, err := os.Stat(mpath); err == nil {
		var mf monstersFile
		if _, err := toml.DecodeFile(mpath, &mf); err != nil {
			return nil, fmt.Errorf("loading %s: %w", mpath, err)
		}
		for id, def := range mf.Monster {
			def.ID = id
			if err := validateMonster(def); err != nil {
				return nil, fmt.Errorf("monster %q in %s: %w", id, mpath, err)
			}
			c.Monsters[id] = def
		}
	} else if !errors.Is(err, os.ErrNotExist) {
		return nil, fmt.Errorf("stat %s: %w", mpath, err)
	}
	return c, nil
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
	if strings.TrimSpace(m.Damage) == "" {
		return fmt.Errorf("damage must not be empty")
	}
	return nil
}
