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
	wasBurdened := g.burdened()
	g.Level.RemoveItem(it)
	if it.Def.Kind == "ammo" { // bundles merge into one quiver
		for _, held := range g.Inventory {
			if held.Def == it.Def {
				held.Charges += it.Charges
				g.log("You pick up the %s.", it.Def.Name)
				if !wasBurdened && g.burdened() {
					g.log("You stagger under your load!")
				}
				g.advanceWorld()
				return
			}
		}
	}
	g.Inventory = append(g.Inventory, it)
	g.log("You pick up the %s.", it.Def.Name)
	if !wasBurdened && g.burdened() {
		g.log("You stagger under your load!")
	}
	g.autoEquip(it)
	g.advanceWorld()
}

// autoEquip wields/wears a just-picked-up weapon or armor when its slot is empty
// (the faithful 0.40 default; it never auto-downgrades an equipped item).
func (g *Game) autoEquip(it *Item) {
	switch it.Def.Kind {
	case "boots":
		if g.Boots == nil {
			g.Boots = it
			g.log("You put on the %s.", it.Def.Name)
		}
	case "head":
		if g.Head == nil {
			g.Head = it
			g.log("You put on the %s.", it.Def.Name)
		}
	case "cloak":
		if g.Cloak == nil {
			g.Cloak = it
			g.log("You put on the %s.", it.Def.Name)
		}
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
	case "ring":
		if g.Ring == nil {
			g.Ring = it
			g.log("You put on the %s.", it.Def.Name)
		}
	case "amulet":
		if g.Amulet == nil {
			g.Amulet = it
			g.log("You put on the %s.", it.Def.Name)
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
	if it.Def.Kind == "spellbook" { // books are studied, not used (C read_book)
		g.readBook(it)
		return
	}
	switch it.Def.Kind {
	case "weapon":
		g.Weapon = it
		g.log("You wield the %s.", it.Def.Name)
	case "armor":
		g.Armor = it
		g.log("You wear the %s.", it.Def.Name)
	case "boots":
		g.Boots = it
		g.log("You put on the %s.", it.Def.Name)
	case "head":
		g.Head = it
		g.log("You put on the %s.", it.Def.Name)
	case "cloak":
		g.Cloak = it
		g.log("You put on the %s.", it.Def.Name)
	case "ring":
		g.Ring = it
		g.log("You put on the %s.", it.Def.Name)
	case "amulet":
		g.Amulet = it
		g.log("You put on the %s.", it.Def.Name)
	case "tool":
		// Keys are the C's key_open (doors.c:420): applying one unlocks an
		// adjacent locked door. Nothing else is an applicable tool yet.
		if it.Def.ID == "key" {
			for dy := -1; dy <= 1; dy++ {
				for dx := -1; dx <= 1; dx++ {
					q := Pos{g.Player.X + dx, g.Player.Y + dy}
					if (dx != 0 || dy != 0) && g.Level.InBounds(q) && g.Level.At(q).Def.Locked {
						g.UnlockDoor(q) // consumes a key and charges the turn
						return
					}
				}
			}
		}
		g.log("There is nothing there to unlock.")
		return // a cancelled apply costs nothing (C key_open returns false)
	case "potion", "food", "scroll":
		if it.Def.Kind != "scroll" && g.gasProtected() {
			g.log("You would have to remove your mask first.") // C: p_eat/p_drink while masked
			return
		}
		g.identify(it) // using a consumable reveals what it was
		if b, ok := g.Behaviors[it.Def.Use]; ok {
			g.Messages = append(g.Messages, b(g, it)...)
		} else {
			g.log("Nothing happens.")
		}
		g.removeInventory(it)
	}
	g.advanceWorld()
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
	wasBurdened := g.burdened()
	for i, x := range g.Inventory {
		if x == it {
			g.Inventory = append(g.Inventory[:i], g.Inventory[i+1:]...)
			break
		}
	}
	if wasBurdened && !g.burdened() {
		g.log("You are no longer burdened.")
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
		if it.Def != nil && (it.Def.Kind == "scroll" || it.Def.Kind == "spellbook") {
			scrolls = append(scrolls, it)
		}
	}
	return scrolls
}
