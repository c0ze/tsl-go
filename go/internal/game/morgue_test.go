package game

import (
	"strings"
	"testing"
)

func TestMorgueTextOnDeath(t *testing.T) {
	g := &Game{PlayerHP: 3, PlayerMax: 10, Dead: true, DeathCause: "ghoul"}
	txt := g.MorgueText()
	if !strings.Contains(txt, "Killed by: ghoul") || !strings.Contains(txt, "Location: the dungeon") {
		t.Errorf("morgue missing expected lines:\n%s", txt)
	}
}
