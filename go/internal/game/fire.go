package game

// WieldedRanged reports whether the player is wielding a ranged weapon (one with
// a firing range), which is what the fire action requires.
func (g *Game) WieldedRanged() bool {
	return g.Weapon != nil && g.Weapon.Def != nil && g.Weapon.Def.Ranged > 0
}

// FireWeapon shoots the wielded ranged weapon at target. A shot is refused —
// without costing a turn — when the player has no ranged weapon, the target is
// beyond the weapon's range, or line of sight is blocked. A valid shot rolls the
// weapon's damage against the creature there (killing it at 0 HP) or sails into
// empty space, then passes a turn.
func (g *Game) FireWeapon(target Pos) {
	if g.Dead || g.Won {
		return
	}
	if !g.WieldedRanged() {
		g.log("You have no ranged weapon to fire.")
		return
	}
	if chebyshev(g.Player, target) > g.Weapon.Def.Ranged {
		g.log("That is out of range.")
		return
	}
	if !g.lineOfSight(g.Player, target) {
		g.log("You don't have a clear shot.")
		return
	}
	if m := g.Level.CreatureAt(target); m != nil {
		dmg := g.RNG.RollSpec(g.Weapon.Def.Damage)
		m.HP -= dmg
		g.log("You shoot the %s for %d.", m.Def.Name, dmg)
		if m.HP <= 0 {
			g.killCreature(m)
		}
	} else {
		g.log("Your shot flies into empty space.")
	}
	g.advanceWorld()
}
