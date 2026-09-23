package gen

import (
	"testing"

	"github.com/c0ze/tsl-go/data"
	"github.com/c0ze/tsl-go/internal/content"
	"github.com/c0ze/tsl-go/internal/game"
	"github.com/c0ze/tsl-go/internal/rng"
)

// Over many seeds of every shipped level, placement respects the terrain the
// C lays first (add_pools before add_boss/add_content): walkers and items on
// dry floor and nothing sitting on stairs. Water-bound creatures seek water
// within two tiles and, like the C's find_nearest_free_spot, settle on dry
// floor when there is none — so only the rate is logged.
func TestPlacementRespectsPools(t *testing.T) {
	c, err := content.Load(data.Files)
	if err != nil {
		t.Fatal(err)
	}
	seeds := 400
	if testing.Short() {
		seeds = 30
	}
	swimmers, wet := 0, 0
	for id, def := range c.Levels {
		for seed := 1; seed <= seeds; seed++ {
			lvl, err := LevelFromDef(rng.NewWithSeed(uint32(seed)), c, def)
			if err != nil {
				t.Fatalf("%s seed %d: %v", id, seed, err)
			}
			for _, m := range lvl.Creatures {
				if m.Def.Permaswim {
					swimmers++
					if lvl.At(m.Pos).Def.Water {
						wet++
					}
				}
				tile := lvl.At(m.Pos).Def
				switch {
				case !m.Def.Swim && (tile.Water || tile.Lava):
					t.Fatalf("%s seed %d: %s starts in %s at %v", id, seed, m.Def.ID, tile.ID, m.Pos)
				case lvl.PortalAt(m.Pos) != nil:
					t.Fatalf("%s seed %d: %s generated on the stairs at %v", id, seed, m.Def.ID, m.Pos)
				}
			}
			if def.Boss != "" && !hasCreature(lvl.Creatures, def.Boss) {
				t.Fatalf("%s seed %d: guaranteed boss %s missing", id, seed, def.Boss)
			}
			for _, it := range lvl.Items {
				if !lvl.Passable(it.Pos) {
					t.Fatalf("%s seed %d: %s out of reach on %s", id, seed, it.Def.ID, lvl.At(it.Pos).Def.ID)
				}
			}
			for _, p := range lvl.Portals {
				if !lvl.Passable(p.Pos) {
					t.Fatalf("%s seed %d: stairs at %v are %s", id, seed, p.Pos, lvl.At(p.Pos).Def.ID)
				}
			}
		}
	}
	t.Logf("water-bound creatures starting in water: %d/%d", wet, swimmers)
}

func hasCreature(cs []*game.Creature, id string) bool {
	for _, m := range cs {
		if m.Def.ID == id {
			return true
		}
	}
	return false
}
