package game

// PlayerPickup picks up the item under the player, then passes the turn.
func (g *Game) PlayerPickup() {
	if g.Dead {
		return
	}
	it := g.Level.ItemAt(g.Player)
	if it == nil {
		g.log("There is nothing here to pick up.")
		return
	}
	g.Level.RemoveItem(it)
	g.Inventory = append(g.Inventory, it)
	g.log("You pick up the %s.", it.Def.Name)
	g.monstersAct()
}

// PlayerUse equips a weapon/armor or invokes a consumable's behavior, then
// passes the turn.
func (g *Game) PlayerUse(it *Item) {
	if g.Dead {
		return
	}
	if it == nil || it.Def == nil || !g.hasInventoryItem(it) {
		g.log("You aren't carrying that.")
		return
	}
	switch it.Def.Kind {
	case "weapon":
		g.Weapon = it
		g.log("You wield the %s.", it.Def.Name)
	case "armor":
		g.Armor = it
		g.log("You wear the %s.", it.Def.Name)
	case "potion":
		if b, ok := g.Behaviors[it.Def.Use]; ok {
			g.Messages = append(g.Messages, b(g, it)...)
		} else {
			g.log("Nothing happens.")
		}
		g.removeInventory(it)
	}
	g.monstersAct()
}

func (g *Game) hasInventoryItem(it *Item) bool {
	for _, x := range g.Inventory {
		if x == it {
			return true
		}
	}
	return false
}

func (g *Game) removeInventory(it *Item) {
	for i, x := range g.Inventory {
		if x == it {
			g.Inventory = append(g.Inventory[:i], g.Inventory[i+1:]...)
			return
		}
	}
}
