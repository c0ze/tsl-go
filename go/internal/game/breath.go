package game

// Breath weapons (#19, C breath.c): cone attacks for fire- and
// poison-breathing creatures. Every creature caught in the cone is hit —
// friendly fire included, no dodge — each taking its own damage roll, as in
// the C (sroll() is called per target inside breath_weapon's loop).
const (
	breathFireRange    = 3     // C missile.c get_spell_range(m_breathe_fire)
	breathFireDamage   = "2d4" // C rules.h BREATHE_FIRE_DAMAGE
	breathPoisonRange  = 4     // C get_spell_range(m_noxious_breath)
	breathPoisonDamage = "1d2" // C NOXIOUS_BREATH_DAMAGE
	breathPoisonTurns  = 12    // C NOXIOUS_BREATH_POISON
)

// breathRange is how far a monster's breath reaches, or 0 for non-breathers.
func breathRange(m *Creature) int {
	switch m.Def.Breath {
	case "fire":
		return breathFireRange
	case "poison":
		return breathPoisonRange
	}
	return 0
}

// breathable reports whether a breath can roll across p: open ground, water
// and lava (the C's is_flyable), or any tile someone is standing on.
func (g *Game) breathable(p Pos) bool {
	if !g.Level.InBounds(p) {
		return false
	}
	def := g.Level.At(p).Def
	return def.Passable || def.Water || def.Lava || g.Level.CreatureAt(p) != nil
}

// breathCone collects the tiles of a breath cone (C area_cone): rng steps
// along (dx,dy), each step swelling sideways with two perpendicular rays —
// note the C's swell math narrows the tip back to a point. Walls stop the
// spine and the rays alike.
func (g *Game) breathCone(origin Pos, dx, dy, rng int) []Pos {
	var tiles []Pos
	seen := map[Pos]bool{}
	add := func(p Pos) {
		if !seen[p] {
			seen[p] = true
			tiles = append(tiles, p)
		}
	}
	ray := func(from Pos, rdx, rdy, n int) {
		p := from
		for i := 0; i < n; i++ {
			p = Pos{X: p.X + rdx, Y: p.Y + rdy}
			if !g.breathable(p) {
				return
			}
			add(p)
		}
	}
	p := origin
	swell := 0
	for left := rng; left > 0; left-- {
		swell++
		if swell > left-1 {
			swell = left - 1 // C: range--; swell++; swell = MIN(range, swell)
		}
		p = Pos{X: p.X + dx, Y: p.Y + dy}
		if !g.breathable(p) {
			break
		}
		add(p)
		ray(p, -dy, dx, swell)
		ray(p, dy, -dx, swell)
	}
	return tiles
}

// breathe is a breather's turn when the player is in range with line of
// sight: exhale along the dominant direction and hit everything in the cone
// (C breath.c breath_weapon).
func (g *Game) breathe(m *Creature) {
	kind := m.Def.Breath
	if kind == "fire" {
		g.log("The %s breathes fire!", m.Def.Name)
	} else {
		g.log("The %s breathes poison!", m.Def.Name)
	}
	dx, dy := signOf(g.Player.X-m.Pos.X), signOf(g.Player.Y-m.Pos.Y)
	for _, p := range g.breathCone(m.Pos, dx, dy, breathRange(m)) {
		if p == g.Player {
			if kind == "fire" {
				g.log("You get burned!")
				g.HurtPlayer(g.RNG.RollSpec(breathFireDamage), m.Def.Name)
			} else {
				g.log("You inhale the vile fumes!")
				g.HurtPlayer(g.RNG.RollSpec(breathPoisonDamage), m.Def.Name)
				if !g.Dead {
					g.AddEffect("poison", breathPoisonTurns)
				}
			}
			continue
		}
		if c := g.Level.CreatureAt(p); c != nil && c != m {
			if kind == "fire" {
				c.HP -= g.RNG.RollSpec(breathFireDamage)
			} else {
				c.HP -= g.RNG.RollSpec(breathPoisonDamage)
				c.AddEffect("poison", breathPoisonTurns)
			}
			if c.HP <= 0 {
				g.killCreature(c)
			}
		}
	}
}

// playerBreathe is the player's exhale (C breath.c, the is_player branch):
// a cone toward the chosen target hitting every creature in it — your own
// allies included — with the same per-target rolls the monsters use.
func (g *Game) playerBreathe(kind string, target Pos) {
	rng := breathFireRange
	if kind == "fire" {
		g.log("You breathe fire!")
	} else {
		g.log("You breathe poison!")
		rng = breathPoisonRange
	}
	dx, dy := signOf(target.X-g.Player.X), signOf(target.Y-g.Player.Y)
	for _, p := range g.breathCone(g.Player, dx, dy, rng) {
		c := g.Level.CreatureAt(p)
		if c == nil {
			continue
		}
		if kind == "fire" {
			c.HP -= g.RNG.RollSpec(breathFireDamage)
		} else {
			c.HP -= g.RNG.RollSpec(breathPoisonDamage)
			c.AddEffect("poison", breathPoisonTurns)
		}
		if c.HP <= 0 {
			g.killCreature(c)
		}
	}
}
