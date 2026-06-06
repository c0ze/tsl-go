// Package content loads validated game content (data) from TOML files.
package content

import (
	"fmt"
	"path/filepath"
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

// Rune returns the tile's glyph as a rune.
func (t *TileDef) Rune() rune {
	r, _ := utf8.DecodeRuneInString(t.Glyph)
	return r
}

// Content is the fully-loaded, validated game content.
type Content struct {
	Tiles map[string]*TileDef
}

type tilesFile struct {
	Tile map[string]*TileDef `toml:"tile"`
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
