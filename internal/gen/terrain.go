package gen

// Terrain features laid into the carved rooms: pools of water and lava,
// doors (including secret and locked ones), and hidden traps.

import (
	"fmt"
	"sort"

	"github.com/c0ze/tsl-go/internal/content"
	"github.com/c0ze/tsl-go/internal/game"
	"github.com/c0ze/tsl-go/internal/rng"
)

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
// corridor punched through the room's wall ring — into doors, porting the
// C's replace_doors (doors.c, called from level.c:363): every doorway starts
// as a candidate, 3+1d5 per level stay secret, the rest convert 50% to bare
// floor or a closed door, and half the closed doors lock (maybe_locked_door).
// Doorways are detected on the original (door-free) layout so placement order
// can't affect the result; stairs, the altar, traps, portals, and the start
// tile are never overwritten.
func placeDoors(r *rng.MT, c *content.Content, lvl *game.Level, rooms []rect) {
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
	spots := make([]game.Pos, 0, len(doorways))
	for p := range doorways {
		spots = append(spots, p)
	}
	sort.Slice(spots, func(i, j int) bool { // deterministic order for the RNG stream
		if spots[i].Y != spots[j].Y {
			return spots[i].Y < spots[j].Y
		}
		return spots[i].X < spots[j].X
	})
	secret, locked := c.Tiles["door_secret"], c.Tiles["door_locked"]
	if secret == nil || locked == nil { // content without the chain: plain doors
		for _, p := range spots {
			lvl.Set(p, closed)
		}
		return
	}
	for i := len(spots) - 1; i > 0; i-- { // who stays secret is the C's random pick
		j := r.Intn(i + 1)
		spots[i], spots[j] = spots[j], spots[i]
	}
	keep := 3 + r.Intn(5) + 1 // doors_to_keep = 3 + roll(1,5)
	for i, p := range spots {
		switch {
		case i < keep:
			lvl.Set(p, secret)
		case r.Intn(2) == 0:
			// converted to plain floor: a doorway with no door at all
		case r.Intn(2) == 0:
			lvl.Set(p, locked)
		default:
			lvl.Set(p, closed)
		}
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

// trapTileIDs are the trap varieties placeTraps deals from, uniformly when
// defined (the C weights these per level in places.c encounter tables — the
// uniform deal is the documented approximation).
var trapTileIDs = []string{"dart_trap", "web_trap", "flash_trap", "plate_trap", "polymorph_trap"}

// placeTraps scatters up to n trap tiles onto plain floor, avoiding the
// start, portals, and other special tiles; traps go down hidden.
func placeTraps(r *rng.MT, c *content.Content, lvl *game.Level, rooms []rect, n int) {
	var kinds []*content.TileDef
	for _, id := range trapTileIDs {
		if t := c.Tiles[id]; t != nil {
			kinds = append(kinds, t)
		}
	}
	if len(kinds) == 0 {
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
		lvl.Set(pos, kinds[r.Intn(len(kinds))])
		// Traps go down hidden, disguised as the floor they replaced, with a
		// 2d6 difficulty for passive spotting (C traps.c roll(2,6)).
		t := lvl.At(pos)
		t.Disguise = floor
		t.TrapDifficulty = 2 + r.Intn(6) + r.Intn(6)
	}
}
