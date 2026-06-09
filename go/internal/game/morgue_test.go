package game

import (
	"strings"
	"testing"

	"github.com/c0ze/tsl/internal/content"
)

func TestMorgueTextOnDeath(t *testing.T) {
	g := &Game{PlayerHP: 3, PlayerMax: 10, Dead: true, DeathCause: "ghoul"}
	txt := g.MorgueText()
	if !strings.Contains(txt, "Killed by: ghoul") || !strings.Contains(txt, "Location: the dungeon") {
		t.Errorf("morgue missing expected lines:\n%s", txt)
	}
}

func TestMorgueListsWornAccessories(t *testing.T) {
	g := &Game{
		PlayerHP: 5, PlayerMax: 10,
		Ring:   &Item{Def: &content.ItemDef{Name: "ring of protection", Kind: "ring", Dodge: 2}},
		Amulet: &Item{Def: &content.ItemDef{Name: "amulet of warding", Kind: "amulet", Dodge: 3}},
	}
	txt := g.MorgueText()
	if !strings.Contains(txt, "ring of protection") || !strings.Contains(txt, "amulet of warding") {
		t.Errorf("morgue should list worn accessories:\n%s", txt)
	}
}
