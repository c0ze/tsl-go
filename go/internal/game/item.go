package game

import (
	"fmt"

	"github.com/c0ze/tsl/internal/content"
)

// Item is an item instance (on the ground while Pos is meaningful, or carried).
type Item struct {
	Def *content.ItemDef
	Pos Pos
}

// Behavior is an item effect, looked up by name from Game.Behaviors and invoked
// when an item is used. It returns log messages. Declared here (the engine
// core) so behaviors can be implemented elsewhere and injected without a cycle.
type Behavior func(g *Game, it *Item) []string

// ItemAt returns the first item lying on p, or nil.
func (l *Level) ItemAt(p Pos) *Item {
	for _, it := range l.Items {
		if it.Pos == p {
			return it
		}
	}
	return nil
}

// RemoveItem removes it from the ground (no-op if absent).
func (l *Level) RemoveItem(it *Item) {
	for i, x := range l.Items {
		if x == it {
			l.Items = append(l.Items[:i], l.Items[i+1:]...)
			return
		}
	}
}

// ValidateItemUses checks that every consumable item's `use` names a registered
// behavior, so malformed content fails fast at startup instead of silently
// doing nothing at runtime.
func ValidateItemUses(c *content.Content, reg map[string]Behavior) error {
	for id, it := range c.Items {
		if it.Kind == "potion" || it.Kind == "food" {
			if _, ok := reg[it.Use]; !ok {
				return fmt.Errorf("item %q references unknown behavior %q", id, it.Use)
			}
		}
	}
	return nil
}
