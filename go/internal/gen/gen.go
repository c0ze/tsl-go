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
	rooms := carveRooms(r, lvl, floor)
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
		it := &game.Item{Def: def, Pos: pos}
		if def.Kind == "wand" || def.Kind == "ammo" {
			it.Charges = def.Power // wand charges / arrows in the bundle
		}
		lvl.Items = append(lvl.Items, it)
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
	if def.Boss != "" {
		bdef := c.Monsters[def.Boss]
		if bdef == nil {
			return nil, fmt.Errorf("gen: level %q boss %q is not a defined monster", def.ID, def.Boss)
		}
		if pos, ok := bossSpot(r, lvl, rooms[len(rooms)-1]); ok {
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
		placeDoors(c, lvl, rooms)
	}
	placeSpawnMonsters(r, c, lvl, rooms, def)
	placeItems(r, c, lvl, rooms, lvl.Start)
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
	if def.Traps > 0 {
		placeTraps(r, c, lvl, rooms, def.Traps)
	}
	return lvl, nil
}

// placeRetinue rings the boss with def.Retinue escorts at the nearest free
// tiles (C encounter_lurker: 8 tentacles at the nearest free spots), water
// before land so swimmers start wet.
func placeRetinue(c *content.Content, lvl *game.Level, def *content.LevelDef, boss game.Pos) {
	if def.Retinue == "" || def.RetinueCount <= 0 {
		return
	}
	rdef := c.Monsters[def.Retinue]
	if rdef == nil {
		return
	}
	placed := 0
	for _, wantWater := range []bool{true, false} {
		for radius := 1; radius <= 5 && placed < def.RetinueCount; radius++ {
			for dy := -radius; dy <= radius && placed < def.RetinueCount; dy++ {
				for dx := -radius; dx <= radius && placed < def.RetinueCount; dx++ {
					if max(abs(dx), abs(dy)) != radius {
						continue // ring only: nearest spots first
					}
					p := game.Pos{X: boss.X + dx, Y: boss.Y + dy}
					if !lvl.InBounds(p) || lvl.CreatureAt(p) != nil || p == lvl.Start || lvl.PortalAt(p) != nil {
						continue
					}
					tile := lvl.At(p).Def
					if tile.Water != wantWater || (!tile.Water && !tile.Passable) {
						continue
					}
					lvl.Creatures = append(lvl.Creatures, &game.Creature{Def: rdef, Pos: p, HP: rdef.HP})
					placed++
				}
			}
		}
	}
}

func abs(n int) int {
	if n < 0 {
		return -n
	}
	return n
}

// placePools carves n pools of the given tile (the C add_pools/area_cloud:
// water blobs of 5-24 tiles, lava 5-13) into room floors. Pools are
// impassable, so any one that would cut the start off from a portal is
// reverted — the C instead rejects whole unsolvable levels; reverting one
// pool keeps generation deterministic and cheap.
func placePools(r *rng.MT, c *content.Content, lvl *game.Level, rooms []rect, n int, tileID string, sizeSpread int) error {
	pool := c.Tiles[tileID]
	if pool == nil {
		return fmt.Errorf("gen: level wants %s pools but no %q tile defined", tileID, tileID)
	}
	floor := c.Tiles["floor"]
	for k := 0; k < n; k++ {
		room := rooms[r.Intn(len(rooms))]
		seed := game.Pos{X: room.x + r.Intn(room.w), Y: room.y + r.Intn(room.h)}
		if !poolable(lvl, seed) {
			continue
		}
		// Grow a cloud: each added tile is a random neighbour of the pool so far.
		size := 5 + r.Intn(sizeSpread)
		grown := []game.Pos{seed}
		lvl.Set(seed, pool)
		// Bounded tries: a pool hemmed in by walls simply stays small.
		for tries := 0; len(grown) < size && tries < 200; tries++ {
			from := grown[r.Intn(len(grown))]
			next := game.Pos{X: from.X + r.Intn(3) - 1, Y: from.Y + r.Intn(3) - 1}
			if !poolable(lvl, next) {
				continue
			}
			lvl.Set(next, pool)
			grown = append(grown, next)
		}
		if cutsOff(lvl) {
			for _, p := range grown {
				lvl.Set(p, floor)
			}
		}
	}
	return nil
}

// poolable reports whether p is plain floor a pool may flood: never the start,
// a portal, or any special tile.
func poolable(lvl *game.Level, p game.Pos) bool {
	return lvl.InBounds(p) && lvl.At(p).Def.ID == "floor" && p != lvl.Start && lvl.PortalAt(p) == nil
}

// cutsOff reports whether any portal is no longer walkable from the start.
func cutsOff(lvl *game.Level) bool {
	seen := map[game.Pos]bool{lvl.Start: true}
	frontier := []game.Pos{lvl.Start}
	for len(frontier) > 0 {
		p := frontier[len(frontier)-1]
		frontier = frontier[:len(frontier)-1]
		for dy := -1; dy <= 1; dy++ {
			for dx := -1; dx <= 1; dx++ {
				q := game.Pos{X: p.X + dx, Y: p.Y + dy}
				if !seen[q] && lvl.Passable(q) {
					seen[q] = true
					frontier = append(frontier, q)
				}
			}
		}
	}
	for _, portal := range lvl.Portals {
		if !seen[portal.Pos] {
			return true
		}
	}
	return false
}

// placeDoors converts each room's doorways — single-tile passages where a
// corridor punched through the room's wall ring — into closed doors. Doorways
// are detected on the original (door-free) layout so placement order can't
// affect the result; stairs, the altar, traps, portals, and the start tile are
// never overwritten.
func placeDoors(c *content.Content, lvl *game.Level, rooms []rect) {
	closed := c.Tiles["door_closed"]
	if closed == nil {
		return
	}
	doorways := map[game.Pos]bool{}
	for _, room := range rooms {
		for _, p := range boundaryRing(room) {
			if !lvl.InBounds(p) || lvl.At(p).Def.ID != "floor" {
				continue // only plain corridor floor becomes a door
			}
			if p == lvl.Start || lvl.PortalAt(p) != nil {
				continue
			}
			if isDoorway(lvl, p) {
				doorways[p] = true
			}
		}
	}
	for p := range doorways {
		lvl.Set(p, closed)
	}
}

// boundaryRing returns the tiles one step outside a room's interior (the wall
// ring), where corridors break through to form doorways.
func boundaryRing(r rect) []game.Pos {
	ps := make([]game.Pos, 0, 2*(r.w+r.h)+4)
	for x := r.x - 1; x <= r.x+r.w; x++ {
		ps = append(ps, game.Pos{X: x, Y: r.y - 1}, game.Pos{X: x, Y: r.y + r.h})
	}
	for y := r.y; y < r.y+r.h; y++ {
		ps = append(ps, game.Pos{X: r.x - 1, Y: y}, game.Pos{X: r.x + r.w, Y: y})
	}
	return ps
}

// isDoorway reports whether p is a clean single-tile passage: passable on
// exactly one axis (the corridor through the wall) with walls flanking it.
func isDoorway(lvl *game.Level, p game.Pos) bool {
	up := lvl.Passable(game.Pos{X: p.X, Y: p.Y - 1})
	down := lvl.Passable(game.Pos{X: p.X, Y: p.Y + 1})
	left := lvl.Passable(game.Pos{X: p.X - 1, Y: p.Y})
	right := lvl.Passable(game.Pos{X: p.X + 1, Y: p.Y})
	horizontal := left && right && !up && !down
	vertical := up && down && !left && !right
	return horizontal || vertical
}

// placeTraps scatters up to n dart_trap tiles onto plain floor tiles, avoiding
// the start, portals, and other special tiles.
func placeTraps(r *rng.MT, c *content.Content, lvl *game.Level, rooms []rect, n int) {
	trap := c.Tiles["dart_trap"]
	if trap == nil {
		return
	}
	for k := 0; k < n; k++ {
		room := rooms[r.Intn(len(rooms))]
		pos := game.Pos{X: room.x + r.Intn(room.w), Y: room.y + r.Intn(room.h)}
		if pos == lvl.Start || !lvl.Passable(pos) || lvl.PortalAt(pos) != nil {
			continue
		}
		if lvl.At(pos).Def.ID != "floor" {
			continue // don't overwrite stairs, the altar, etc.
		}
		floor := lvl.At(pos).Def
		lvl.Set(pos, trap)
		// Traps go down hidden, disguised as the floor they replaced, with a
		// 2d6 difficulty for passive spotting (C traps.c roll(2,6)).
		t := lvl.At(pos)
		t.Disguise = floor
		t.TrapDifficulty = 2 + r.Intn(6) + r.Intn(6)
	}
}

// disguiseMimic dresses a freshly spawned mimic as a random spawnable item
// (C monster.c: the build-time 1d4 disguise roll, item case).
func disguiseMimic(r *rng.MT, c *content.Content, m *game.Creature) {
	if !m.Def.Mimic {
		return
	}
	ids := make([]string, 0, len(c.Items))
	for id, it := range c.Items {
		if !it.NoSpawn {
			ids = append(ids, id)
		}
	}
	if len(ids) == 0 {
		return
	}
	sort.Strings(ids)
	m.Disguised = true
	m.DisguiseAs = c.Items[ids[r.Intn(len(ids))]]
}

// placeSpawnMonsters scatters def.Monsters monsters drawn from def's weighted
// spawn table across the rooms (never on the start tile).
func placeSpawnMonsters(r *rng.MT, c *content.Content, lvl *game.Level, rooms []rect, def *content.LevelDef) {
	if def.Monsters <= 0 || len(def.Spawn) == 0 {
		return
	}
	total := 0
	for _, s := range def.Spawn {
		total += s.Weight
	}
	for k := 0; k < def.Monsters; k++ {
		room := rooms[r.Intn(len(rooms))]
		pos := game.Pos{X: room.x + r.Intn(room.w), Y: room.y + r.Intn(room.h)}
		if pos == lvl.Start || !lvl.Passable(pos) || lvl.CreatureAt(pos) != nil {
			continue
		}
		mdef := c.Monsters[pickSpawn(r, def.Spawn, total)]
		m := &game.Creature{Def: mdef, Pos: pos, HP: mdef.HP}
		disguiseMimic(r, c, m)
		lvl.Creatures = append(lvl.Creatures, m)
	}
}

// bossSpot finds a passable tile in room free of the altar and other creatures,
// for placing a guaranteed boss. Returns false if none is found.
func bossSpot(r *rng.MT, lvl *game.Level, room rect) (game.Pos, bool) {
	for try := 0; try < 30; try++ {
		p := game.Pos{X: room.x + r.Intn(room.w), Y: room.y + r.Intn(room.h)}
		if lvl.Passable(p) && lvl.At(p).Def.ID != "altar" && lvl.CreatureAt(p) == nil {
			return p, true
		}
	}
	return game.Pos{}, false
}

// pickSpawn returns a monster id from the weighted spawn table (total is the sum
// of weights).
func pickSpawn(r *rng.MT, spawn []content.SpawnEntry, total int) string {
	n := r.Intn(total)
	for _, s := range spawn {
		if n < s.Weight {
			return s.Monster
		}
		n -= s.Weight
	}
	return spawn[len(spawn)-1].Monster
}
