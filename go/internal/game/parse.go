package game

import (
	"fmt"

	"github.com/c0ze/tsl/internal/content"
)

// ParseLevel builds a level from ASCII rows (all rows must be equal width and
// ASCII-only). Each rune is mapped through legend to a tile id that must exist
// in c. The '@' rune marks the player start; legend must also map '@' to the
// tile id that lies underneath the player. Returns the level and start position.
func ParseLevel(c *content.Content, rows []string, legend map[rune]string) (*Level, Pos, error) {
	h := len(rows)
	if h == 0 {
		return nil, Pos{}, fmt.Errorf("no rows")
	}
	w := len(rows[0])
	l := &Level{W: w, H: h, tiles: make([]Tile, w*h)}
	start := Pos{}
	for y, row := range rows {
		if len(row) != w {
			return nil, Pos{}, fmt.Errorf("row %d width %d, want %d", y, len(row), w)
		}
		for x := 0; x < len(row); x++ {
			r := rune(row[x])
			if r == '@' {
				start = Pos{x, y}
			}
			id, ok := legend[r]
			if !ok {
				return nil, Pos{}, fmt.Errorf("no legend entry for %q at (%d,%d)", r, x, y)
			}
			def, ok := c.Tiles[id]
			if !ok {
				return nil, Pos{}, fmt.Errorf("unknown tile id %q", id)
			}
			l.tiles[y*w+x] = Tile{Def: def}
		}
	}
	return l, start, nil
}
