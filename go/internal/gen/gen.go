// Package gen builds dungeon levels procedurally from a seeded *rng.MT, so a
// given seed always yields the same level.
package gen

import (
	"fmt"
	"sort"

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
func Rooms(r *rng.MT, c *content.Content, w, h, depth int) (*game.Level, game.Pos, game.Pos, error) {
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
	placeMonsters(r, c, lvl, rooms, start, depth)
	placeItems(r, c, lvl, rooms, start)
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

// placeItems drops up to one item into each room except the starting room.
func placeItems(r *rng.MT, c *content.Content, lvl *game.Level, rooms []rect, start game.Pos) {
	ids := make([]string, 0, len(c.Items))
	for id, it := range c.Items {
		if it.NoSpawn { // corpses & other drop-only items never spawn as floor loot
			continue
		}
		ids = append(ids, id)
	}
	if len(ids) == 0 {
		return
	}
	sort.Strings(ids)
	for i, room := range rooms {
		if i == 0 || r.Intn(2) == 0 { // ~half the rooms, never the start
			continue
		}
		pos := game.Pos{X: room.x + r.Intn(room.w), Y: room.y + r.Intn(room.h)}
		if pos == start || !lvl.Passable(pos) || lvl.ItemAt(pos) != nil {
			continue
		}
		def := c.Items[ids[r.Intn(len(ids))]]
		lvl.Items = append(lvl.Items, &game.Item{Def: def, Pos: pos})
	}
}

// placeMonsters drops 0-2 monsters into each room except the starting room.
func placeMonsters(r *rng.MT, c *content.Content, lvl *game.Level, rooms []rect, start game.Pos, depth int) {
	ids := make([]string, 0, len(c.Monsters))
	for id, m := range c.Monsters {
		if m.MinDepth > depth {
			continue // not deep enough for this monster yet
		}
		ids = append(ids, id)
	}
	if len(ids) == 0 {
		return
	}
	sort.Strings(ids) // deterministic ordering for a given seed
	for i, room := range rooms {
		if i == 0 {
			continue // keep the starting room clear
		}
		n := r.Intn(3) // 0..2
		for k := 0; k < n; k++ {
			pos := game.Pos{X: room.x + r.Intn(room.w), Y: room.y + r.Intn(room.h)}
			if pos == start || !lvl.Passable(pos) || lvl.CreatureAt(pos) != nil {
				continue
			}
			def := c.Monsters[ids[r.Intn(len(ids))]]
			lvl.Creatures = append(lvl.Creatures, &game.Creature{Def: def, Pos: pos, HP: def.HP})
		}
	}
}
