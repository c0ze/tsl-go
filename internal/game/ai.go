package game

// Monster behaviour (C ai.c): pursuit, flight, confusion, allies, and the
// terrain rules for where a creature may step.

const senseRange = 8 // how close a monster must be to notice the player

// monsterAct is a single monster action: attack the player if adjacent, else
// step toward the player if within sense range.
func (g *Game) monsterAct(m *Creature) {
	if m.Disguised {
		return // a mimic in its glamour does nothing at all (C ai_mimic)
	}
	if m.HasEffect("web") {
		// The monster's turn is its struggle (C struggle_web).
		for i := range m.Effects {
			if m.Effects[i].Kind == "web" {
				if m.Effects[i].Turns < webStruggle {
					m.RemoveEffect("web")
				} else {
					m.Effects[i].Turns -= webStruggle
				}
				break
			}
		}
		return
	}
	if m.HasEffect("sleep") {
		return // asleep: it loses the turn outright (C game.c effect_sleep skip)
	}
	if m.HasEffect("confuse") {
		g.stepRandom(m) // disoriented: it lurches at random and can't press an attack
		return
	}
	if m.HasEffect("fear") {
		g.stepAway(m, g.Player) // frightened: it flees the player instead of attacking
		return
	}
	if m.Ally {
		g.allyAct(m)
		return
	}
	dist := chebyshev(m.Pos, g.Player)
	if dist == 1 {
		g.monsterAttacks(m)
		return
	}
	// A distant player doesn't shield an ally at the flank (C enemies split).
	for _, c := range g.Level.Creatures {
		if c.Ally && chebyshev(m.Pos, c.Pos) == 1 {
			g.monsterFights(m, c)
			return
		}
	}
	if m.HasEffect("blind") {
		return // blinded: it can flail at an adjacent foe but can't track at range
	}
	if r := breathRange(m); r > 0 && dist <= r && g.lineOfSight(m.Pos, g.Player) {
		g.breathe(m)
		return
	}
	if m.Def.Ranged > 0 && dist <= m.Def.Ranged && g.lineOfSight(m.Pos, g.Player) {
		g.rangedAttack(m)
		return
	}
	if dist <= g.noticeRange() {
		g.stepToward(m, g.Player)
	}
}

// allyAct is a charmed creature's turn (C charm/ai_offensive): bite the
// adjacent hostile, else close on the nearest one in sense range, else heel
// to the player.
func (g *Game) allyAct(m *Creature) {
	var target *Creature
	best := senseRange + 1
	for _, c := range g.Level.Creatures {
		if c == m || c.Ally {
			continue
		}
		if d := chebyshev(m.Pos, c.Pos); d < best {
			best, target = d, c
		}
	}
	if target != nil && best == 1 {
		g.monsterFights(m, target)
		return
	}
	if target != nil {
		g.stepToward(m, target.Pos)
		return
	}
	if chebyshev(m.Pos, g.Player) > 2 {
		g.stepToward(m, g.Player)
	}
}

// monsterFights resolves one creature-vs-creature swing — an ally biting a
// hostile or the reverse — on the same attack-vs-dodge model as every other
// melee in the port.
func (g *Game) monsterFights(a, d *Creature) {
	g.revealMimic(d)
	if !g.RNG.Chance(a.Def.Attack, d.Def.Dodge) {
		g.log("The %s misses the %s.", a.Def.Name, d.Def.Name)
		return
	}
	d.RemoveEffect("sleep") // a landed hit wakes a sleeper (C combat.c)
	dmg := g.RNG.RollSpec(a.Def.Damage)
	d.HP -= dmg
	g.log("The %s hits the %s for %d.", a.Def.Name, d.Def.Name, dmg)
	if d.HP <= 0 {
		g.killCreature(d)
	} else if g.rollWound(a.Def.Wound) {
		g.woundCreature(d)
	}
}

// creatureCanEnter is the terrain half of a monster's move (C move_creature):
// a permaswimmer refuses anything but water, a free-swimmer takes water or
// land, and everyone else needs walkable ground.
func (g *Game) creatureCanEnter(m *Creature, dst Pos) bool {
	if !g.Level.InBounds(dst) {
		return false
	}
	if m.Def.Mimic {
		return false // rooted in place (C attr_p_move)
	}
	if m.Def.Permaswim {
		return g.Level.At(dst).Def.Water
	}
	if m.Def.Swim && g.Level.At(dst).Def.Water {
		return true
	}
	return g.Level.Passable(dst)
}

func (g *Game) stepToward(m *Creature, target Pos) {
	dst := Pos{m.Pos.X + signOf(target.X-m.Pos.X), m.Pos.Y + signOf(target.Y-m.Pos.Y)}
	if dst == g.Player || g.Level.CreatureAt(dst) != nil {
		return
	}
	if !g.creatureCanEnter(m, dst) {
		// A refused step still tries a door in the way — even for a creature
		// that couldn't walk through it (C pursue: move_creature fails, then
		// open_door), unless it lacks the knack for doors.
		g.monsterOpenDoor(m, dst)
		return
	}
	m.Pos = dst
	g.creatureBleeds(m)
}

// stepAway moves m one tile directly away from `from` — a frightened creature's
// flight. If the retreat tile is blocked (a wall, another creature, or the
// player), the cornered creature holds its ground.
func (g *Game) stepAway(m *Creature, from Pos) {
	dx, dy := signOf(m.Pos.X-from.X), signOf(m.Pos.Y-from.Y)
	if dx == 0 && dy == 0 {
		return // on top of the player (shouldn't happen): nowhere to flee
	}
	dst := Pos{m.Pos.X + dx, m.Pos.Y + dy}
	if dst == g.Player || g.Level.CreatureAt(dst) != nil || !g.creatureCanEnter(m, dst) {
		return
	}
	m.Pos = dst
	g.creatureBleeds(m)
}

// stepRandom moves m one tile in a random direction — a confused creature's
// aimless lurch. It won't stumble onto the player (so a confused creature never
// attacks), onto another creature, or into a wall; those just cost the turn.
func (g *Game) stepRandom(m *Creature) {
	dx, dy := g.RNG.Intn(3)-1, g.RNG.Intn(3)-1
	if dx == 0 && dy == 0 {
		return // stumbles in place
	}
	dst := Pos{m.Pos.X + dx, m.Pos.Y + dy}
	if dst == g.Player || g.Level.CreatureAt(dst) != nil || !g.creatureCanEnter(m, dst) {
		return
	}
	m.Pos = dst
	g.creatureBleeds(m)
}

func chebyshev(a, b Pos) int {
	dx, dy := a.X-b.X, a.Y-b.Y
	if dx < 0 {
		dx = -dx
	}
	if dy < 0 {
		dy = -dy
	}
	if dx > dy {
		return dx
	}
	return dy
}
