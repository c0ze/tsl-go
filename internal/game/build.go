package game

import "github.com/c0ze/tsl-go/internal/content"

// NewLevel returns a level of size w×h with every tile set to fill.
func NewLevel(w, h int, fill *content.TileDef) *Level {
	l := &Level{W: w, H: h, tiles: make([]Tile, w*h)}
	for i := range l.tiles {
		l.tiles[i] = Tile{Def: fill}
	}
	return l
}

// Set replaces the tile definition at p, which must be in bounds.
func (l *Level) Set(p Pos, def *content.TileDef) {
	l.At(p).Def = def
}
