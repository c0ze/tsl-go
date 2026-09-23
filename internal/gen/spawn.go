package gen

// Populating a level: floor loot, the spawn-table monsters, mimics, and a
// level's guaranteed boss with its retinue.

import (
	"sort"

	"github.com/c0ze/tsl-go/internal/content"
	"github.com/c0ze/tsl-go/internal/game"
	"github.com/c0ze/tsl-go/internal/rng"
)

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
		if pos == lvl.Start || lvl.CreatureAt(pos) != nil || lvl.PortalAt(pos) != nil {
			continue
		}
		mdef := c.Monsters[pickSpawn(r, def.Spawn, total)]
		if mdef.Permaswim {
			// A water-bound spawn takes the nearest free water within two
			// tiles, else dry floor like anyone (C find_nearest_free_spot).
			if w, ok := waterNear(lvl, pos, 2); ok && w != lvl.Start {
				pos = w
			}
		}
		if !lvl.Passable(pos) && !(mdef.Permaswim && lvl.At(pos).Def.Water) {
			continue
		}
		m := &game.Creature{Def: mdef, Pos: pos, HP: mdef.HP}
		disguiseMimic(r, c, m)
		lvl.Creatures = append(lvl.Creatures, m)
	}
}

// waterNear returns the nearest creature-free water tile within radius of p.
func waterNear(lvl *game.Level, p game.Pos, radius int) (game.Pos, bool) {
	for rad := 0; rad <= radius; rad++ {
		for dy := -rad; dy <= rad; dy++ {
			for dx := -rad; dx <= rad; dx++ {
				q := game.Pos{X: p.X + dx, Y: p.Y + dy}
				if max(abs(dx), abs(dy)) == rad && lvl.InBounds(q) && lvl.At(q).Def.Water && lvl.CreatureAt(q) == nil {
					return q, true
				}
			}
		}
	}
	return game.Pos{}, false
}

// bossSpot finds a tile for a guaranteed boss, free of creatures, the altar,
// the stairs, and the start: open floor, or water for a water-bound boss. It
// tries random spots in the last room, then scans that room, then the rest —
// the C's find_boss always has a reserved tile, so a flooded room must not
// cost the level its boss. Returns false only if no tile qualifies anywhere.
func bossSpot(r *rng.MT, lvl *game.Level, rooms []rect, swim bool) (game.Pos, bool) {
	fits := func(p game.Pos) bool {
		tile := lvl.At(p).Def
		return (lvl.Passable(p) || swim && tile.Water) && lvl.CreatureAt(p) == nil &&
			lvl.PortalAt(p) == nil && p != lvl.Start && tile.ID != "altar"
	}
	room := rooms[len(rooms)-1]
	for try := 0; try < 30; try++ {
		p := game.Pos{X: room.x + r.Intn(room.w), Y: room.y + r.Intn(room.h)}
		if fits(p) {
			return p, true
		}
	}
	for i := len(rooms) - 1; i >= 0; i-- {
		rm := rooms[i]
		for y := rm.y; y < rm.y+rm.h; y++ {
			for x := rm.x; x < rm.x+rm.w; x++ {
				if p := (game.Pos{X: x, Y: y}); fits(p) {
					return p, true
				}
			}
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
