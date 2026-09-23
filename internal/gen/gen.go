// Package gen builds dungeon levels procedurally from a seeded *rng.MT, so a
// given seed always yields the same level.
package gen

import (
	"fmt"

	"github.com/c0ze/tsl-go/internal/content"
	"github.com/c0ze/tsl-go/internal/game"
	"github.com/c0ze/tsl-go/internal/rng"
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

// carveRooms places up to maxRooms non-overlapping rooms into lvl (which starts
// filled with wall), joining each to the previous with a corridor so the result
// is fully connected, and returns the rooms in placement order.
func carveRooms(r *rng.MT, lvl *game.Level, floor *content.TileDef) []rect {
	w, h := lvl.W, lvl.H
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
	return rooms
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

// LevelFromDef builds a level described by def: rooms + corridors sized to the
// def, one portal per link (in distinct rooms), monsters drawn from the def's
// weighted spawn table, and the usual scattered items. The Start (first-arrival
// spawn) is the first room's center.
func LevelFromDef(r *rng.MT, c *content.Content, def *content.LevelDef) (*game.Level, error) {
	floor, wall, stairs := c.Tiles["floor"], c.Tiles["wall"], c.Tiles["stairs_down"]
	if floor == nil || wall == nil || stairs == nil {
		return nil, fmt.Errorf("gen: tiles floor/wall/stairs_down must all be defined")
	}
	lvl := game.NewLevel(def.W, def.H, wall)
	lvl.Dark = def.Dark
	rooms := carveRooms(r, lvl, floor)
	if len(rooms) == 0 {
		return nil, fmt.Errorf("gen: level %q too small for any rooms", def.ID)
	}
	lvl.Start = rooms[0].center()
	for i, target := range def.Links {
		pos := rooms[(i+1)%len(rooms)].center() // distinct rooms after the start
		lvl.Set(pos, stairs)
		lvl.Portals = append(lvl.Portals, game.Portal{Pos: pos, Target: target})
	}
	if def.Altar {
		altar := c.Tiles["altar"]
		if altar == nil {
			return nil, fmt.Errorf("gen: level %q has altar but no \"altar\" tile defined", def.ID)
		}
		lvl.Set(rooms[len(rooms)-1].center(), altar)
	}
	// Pools go in before anyone is placed, as the C's build_dungeon orders
	// it (add_pools, then add_boss and add_content): monsters and items then
	// land on dry floor, swimmers in the water.
	if def.Water > 0 {
		if err := placePools(r, c, lvl, rooms, def.Water, "water", 20); err != nil {
			return nil, err
		}
	}
	if def.Lava > 0 {
		// Lava clouds run smaller than water (C content.c: 5+rnd%9 vs 5+rnd%20).
		if err := placePools(r, c, lvl, rooms, def.Lava, "lava", 9); err != nil {
			return nil, err
		}
	}
	if def.Boss != "" {
		bdef := c.Monsters[def.Boss]
		if bdef == nil {
			return nil, fmt.Errorf("gen: level %q boss %q is not a defined monster", def.ID, def.Boss)
		}
		if pos, ok := bossSpot(r, lvl, rooms, bdef.Permaswim); ok {
			if bdef.Permaswim {
				// "Reset the lurkers pool" (C encounter_lurker): a water-bound
				// boss gets water forced under it, wherever it lands.
				water := c.Tiles["water"]
				if water == nil {
					return nil, fmt.Errorf("gen: level %q boss %q is permaswim but no \"water\" tile defined", def.ID, def.Boss)
				}
				lvl.Set(pos, water)
			}
			lvl.Creatures = append(lvl.Creatures, &game.Creature{Def: bdef, Pos: pos, HP: bdef.HP})
			placeRetinue(c, lvl, def, pos)
		}
	}
	if def.Doors {
		placeDoors(r, c, lvl, rooms)
	}
	placeSpawnMonsters(r, c, lvl, rooms, def)
	placeItems(r, c, lvl, rooms, lvl.Start)
	if def.Traps > 0 {
		placeTraps(r, c, lvl, rooms, def.Traps)
	}
	return lvl, nil
}
