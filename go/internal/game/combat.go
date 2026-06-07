package game

import "fmt"

// Player melee stats (constant until the player becomes attribute-driven).
const (
	playerAttack = 5
	playerDodge  = 3
	playerDamage = "1d4"
)

const senseRange = 8 // how close a monster must be to notice the player

// log appends a message to the game log.
func (g *Game) log(format string, args ...any) {
	g.Messages = append(g.Messages, fmt.Sprintf(format, args...))
}

// PlayerStep performs the player's turn: bump-attack a monster in direction d,
// otherwise move. Then every monster acts.
func (g *Game) PlayerStep(d Direction) {
	if g.Dead {
		return
	}
	dx, dy := d.Delta()
	dst := Pos{g.Player.X + dx, g.Player.Y + dy}
	acted := false
	if m := g.Level.CreatureAt(dst); m != nil {
		g.playerAttacks(m)
		acted = true
	} else if g.Move(d) {
		acted = true
	}
	if acted { // a blocked move into a wall doesn't pass the turn
		g.monstersAct()
	}
}

func (g *Game) playerAttacks(m *Creature) {
	if !g.RNG.Chance(playerAttack, m.Def.Dodge) {
		g.log("You miss the %s.", m.Def.Name)
		return
	}
	dmg := g.RNG.RollSpec(playerDamage)
	m.HP -= dmg
	g.log("You hit the %s for %d.", m.Def.Name, dmg)
	if m.HP <= 0 {
		g.log("The %s dies.", m.Def.Name)
		g.Level.RemoveCreature(m)
	}
}

func (g *Game) monsterAttacks(m *Creature) {
	if !g.RNG.Chance(m.Def.Attack, playerDodge) {
		g.log("The %s misses you.", m.Def.Name)
		return
	}
	dmg := g.RNG.RollSpec(m.Def.Damage)
	g.PlayerHP -= dmg
	g.log("The %s hits you for %d.", m.Def.Name, dmg)
	if g.PlayerHP <= 0 {
		g.PlayerHP = 0
		g.Dead = true
		g.log("You die.")
	}
}

// monstersAct gives each living monster a turn: attack the player if adjacent,
// else step toward the player if within sense range, else idle.
func (g *Game) monstersAct() {
	// snapshot to avoid mutation surprises when creatures are removed
	snapshot := make([]*Creature, len(g.Level.Creatures))
	copy(snapshot, g.Level.Creatures)
	for _, m := range snapshot {
		if g.Dead {
			return
		}
		if g.Level.CreatureAt(m.Pos) != m {
			continue // already removed this turn
		}
		if chebyshev(m.Pos, g.Player) == 1 {
			g.monsterAttacks(m)
			continue
		}
		if chebyshev(m.Pos, g.Player) <= senseRange {
			g.stepToward(m, g.Player)
		}
	}
}

func (g *Game) stepToward(m *Creature, target Pos) {
	step := func(a, b int) int {
		switch {
		case b > a:
			return 1
		case b < a:
			return -1
		default:
			return 0
		}
	}
	dst := Pos{m.Pos.X + step(m.Pos.X, target.X), m.Pos.Y + step(m.Pos.Y, target.Y)}
	if dst == g.Player || !g.Level.Passable(dst) || g.Level.CreatureAt(dst) != nil {
		return
	}
	m.Pos = dst
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
