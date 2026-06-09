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
	g.autoEquip(it)
	g.monstersAct()
}

// autoEquip wields/wears a just-picked-up weapon or armor when its slot is empty
// (the faithful 0.40 default; it never auto-downgrades an equipped item).
func (g *Game) autoEquip(it *Item) {
	switch it.Def.Kind {
	case "weapon":
		if g.Weapon == nil {
			g.Weapon = it
			g.log("You wield the %s.", it.Def.Name)
		}
	case "armor":
		if g.Armor == nil {
			g.Armor = it
			g.log("You wear the %s.", it.Def.Name)
		}
	}
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
	case "potion", "food", "scroll":
		g.identify(it) // using a consumable reveals what it was
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

// EdibleInventory returns the food items the player is carrying, in inventory
// order. The UI uses it to build the "eat what?" menu.
func (g *Game) EdibleInventory() []*Item {
	var food []*Item
	for _, it := range g.Inventory {
		if it.Def != nil && it.Def.Kind == "food" {
			food = append(food, it)
		}
	}
	return food
}

// WandInventory returns the wand items the player is carrying, in inventory order.
func (g *Game) WandInventory() []*Item {
	var wands []*Item
	for _, it := range g.Inventory {
		if it.Def != nil && it.Def.Kind == "wand" {
			wands = append(wands, it)
		}
	}
	return wands
}

// ReadableInventory returns the scroll items the player is carrying, in inventory
// order. The UI uses it to build the "read what?" menu.
func (g *Game) ReadableInventory() []*Item {
	var scrolls []*Item
	for _, it := range g.Inventory {
		if it.Def != nil && it.Def.Kind == "scroll" {
			scrolls = append(scrolls, it)
		}
	}
	return scrolls
}
