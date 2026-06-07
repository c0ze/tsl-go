// Package gen builds dungeon levels procedurally from a seeded *rng.MT, so a
// given seed always yields the same level.
package gen

import (
	"fmt"

	"github.com/c0ze/tsl/internal/content"
	"github.com/c0ze/tsl/internal/game"
	"github.com/c0ze/tsl/internal/rng"
)

const (
	maxRooms = 12
	minRoomW = 4
	maxRoomW = 10
	minRoomH = 3
	maxRoomH = 7
)

type rect struct{ x, y, w, h int }

func (r rect) center() game.Pos { return game.Pos{X: r.x + r.w/2, Y: r.y + r.h/2} }

// intersects reports whether r and o overlap, treating a 1-tile gap as touching
// so rooms keep a wall between them.
func (r rect) intersects(o rect) bool {
	return r.x <= o.x+o.w && r.x+r.w >= o.x && r.y <= o.y+o.h && r.y+r.h >= o.y
}

// Rooms generates a rooms-and-corridors level of size w×h. It returns the level,
// the player start (center of the first room), and the down-stairs position
// (center of the last room). Each room is joined to the previous by a corridor,
// so the level is fully connected.
func Rooms(r *rng.MT, c *content.Content, w, h int) (*game.Level, game.Pos, game.Pos, error) {
	floor, wall, stairs := c.Tiles["floor"], c.Tiles["wall"], c.Tiles["stairs_down"]
	if floor == nil || wall == nil || stairs == nil {
		return nil, game.Pos{}, game.Pos{}, fmt.Errorf("gen: tiles floor/wall/stairs_down must all be defined")
	}

	lvl := game.NewLevel(w, h, wall)
	var rooms []rect
	for i := 0; i < maxRooms; i++ {
		rw := minRoomW + r.Intn(maxRoomW-minRoomW+1)
		rh := minRoomH + r.Intn(maxRoomH-minRoomH+1)
		if w-rw-1 < 1 || h-rh-1 < 1 {
			continue
		}
		room := rect{1 + r.Intn(w-rw-1), 1 + r.Intn(h-rh-1), rw, rh}
		overlaps := false
		for _, o := range rooms {
			if room.intersects(o) {
				overlaps = true
				break
			}
		}
		if overlaps {
			continue
		}
		carveRoom(lvl, room, floor)
		if len(rooms) > 0 {
			carveCorridor(r, lvl, rooms[len(rooms)-1].center(), room.center(), floor)
		}
		rooms = append(rooms, room)
	}
	if len(rooms) == 0 {
		return nil, game.Pos{}, game.Pos{}, fmt.Errorf("gen: no rooms placed (level too small)")
	}

	start := rooms[0].center()
	down := rooms[len(rooms)-1].center()
	lvl.Set(down, stairs)
	return lvl, start, down, nil
}

func carveRoom(lvl *game.Level, r rect, floor *content.TileDef) {
	for y := r.y; y < r.y+r.h; y++ {
		for x := r.x; x < r.x+r.w; x++ {
			lvl.Set(game.Pos{X: x, Y: y}, floor)
		}
	}
}

func carveCorridor(r *rng.MT, lvl *game.Level, a, b game.Pos, floor *content.TileDef) {
	if r.Intn(2) == 0 {
		carveH(lvl, a.X, b.X, a.Y, floor)
		carveV(lvl, a.Y, b.Y, b.X, floor)
	} else {
		carveV(lvl, a.Y, b.Y, a.X, floor)
		carveH(lvl, a.X, b.X, b.Y, floor)
	}
}

func carveH(lvl *game.Level, x0, x1, y int, floor *content.TileDef) {
	if x0 > x1 {
		x0, x1 = x1, x0
	}
	for x := x0; x <= x1; x++ {
		lvl.Set(game.Pos{X: x, Y: y}, floor)
	}
}

func carveV(lvl *game.Level, y0, y1, x int, floor *content.TileDef) {
	if y0 > y1 {
		y0, y1 = y1, y0
	}
	for y := y0; y <= y1; y++ {
		lvl.Set(game.Pos{X: x, Y: y}, floor)
	}
}
