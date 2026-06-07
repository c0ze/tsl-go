# Monster Roster (batch 1) Implementation Plan

> **For Claude:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task.

**Goal:** Expand the bestiary beyond rat + ghoul with a first batch of early-tier melee monsters, and fix the placeholder names/glyphs to match canon. (GitHub issue #13.)

**Architecture:** Pure content — the engine already supports arbitrary monsters, corpse drops, and eating (shipped in #12). This adds TOML data + a golden-data test that validates the shipped roster. No engine code changes.

**Tech Stack:** Go 1.21 (`GOTOOLCHAIN=local`), TOML data under `go/data/`.

**Scope decisions (documented for review):**
- **Faithful-ish, not 1:1.** The C combat is attribute-based (`attr_health`, `attr_speed`, skills, equipment); our model is hp/attack/dodge/damage/speed. So HP≈`attr_health`, speed≈`attr_speed` (compatible scale, ~100 = normal), and attack/dodge/damage are assigned by role. Tunable later.
- **Early-tier only.** No depth-scaling yet (that's #16), so the generator spawns uniformly. This batch stays in the rat/ghoul HP band (3–8). C's tanks (slime 18, gloom lord 28, chrome angel 60) and special-AI monsters (casters/ranged/blink/mimic: electric snake, sludge dweller, burning skull, imp, chainsaw ogre, floating brain, hellhound, …) are **deferred** until depth-scaled spawn tables + ability behaviors exist.
- **Canon fixes (#13):** rename `rat`→`ratman` (glyph `r` unchanged); change `ghoul` glyph `g`→`Z` (frees `g` for graveling).
- **Bestiary descriptions + an inspect command are deferred** (no inspect UI yet) — roster/stats now, flavour text later.

**Roster after this batch** (glyphs/HP/speed from `common/glyph.c` + `common/monster.c`):

| id | name | glyph | color | hp | atk | dodge | dmg | speed | corpse |
|----|------|-------|-------|----|----|-------|-----|-------|--------|
| ratman (was rat) | ratman | r | brown | 3 | 2 | 1 | 1d2 | 130 | ratman_corpse |
| ghoul | ghoul | Z | green | 8 | 4 | 2 | 1d4 | 80 | ghoul_corpse |
| graveling | graveling | g | brown | 5 | 3 | 1 | 1d4 | 60 | carcass |
| gnoblin | gnoblin | o | green | 4 | 3 | 2 | 1d3 | 85 | corpse |
| crypt_vermin | crypt vermin | v | normal | 6 | 2 | 2 | 1d2 | 130 | carcass |
| merman | merman | M | cyan | 8 | 4 | 2 | 1d4 | 65 | corpse |

New corpse foods (all `nospawn`, glyph `%`): `corpse` (+2, normal), `carcass` (+2, brown); rename `rat_corpse`→`ratman_corpse` (+3).

---

## Task 0: Branch
Already on `monster-roster` (off `master`).

## Task 1: Golden-roster test (failing first)

**Files:** Create `go/data/data_test.go`

**Step 1: Write the test.**
```go
package data

import (
	"testing"

	"github.com/c0ze/tsl/internal/content"
)

// TestEmbeddedRoster validates the actually-shipped content: the roster is
// present and the canon names/glyphs are correct.
func TestEmbeddedRoster(t *testing.T) {
	c, err := content.Load(Files)
	if err != nil {
		t.Fatalf("Load shipped content: %v", err)
	}
	if _, ok := c.Monsters["rat"]; ok {
		t.Error("monster id \"rat\" should be renamed to \"ratman\"")
	}
	for _, id := range []string{"ratman", "ghoul", "graveling", "gnoblin", "crypt_vermin", "merman"} {
		if _, ok := c.Monsters[id]; !ok {
			t.Errorf("missing monster %q", id)
		}
	}
	if g := c.Monsters["ghoul"]; g == nil || g.Rune() != 'Z' {
		t.Error("ghoul glyph should be 'Z' (canon)")
	}
	if r := c.Monsters["ratman"]; r == nil || r.Rune() != 'r' {
		t.Error("ratman glyph should be 'r'")
	}
}
```

**Step 2: Run, expect FAIL.**
```bash
cd /Users/arda/projects/tsl/go && GOTOOLCHAIN=local go test ./data/ -run TestEmbeddedRoster -v
```
Expected: FAIL (`rat` still present, `ratman`/new monsters missing).

## Task 2: Update the data

**Files:** `go/data/monsters.toml`, `go/data/items.toml`

**Step 1:** Rewrite `monsters.toml` so `[monster.ratman]` replaces `[monster.rat]` (glyph `r`, corpse `ratman_corpse`), `[monster.ghoul]` glyph becomes `Z`, and add `graveling`, `gnoblin`, `crypt_vermin`, `merman` per the table above.

**Step 2:** In `items.toml`, rename `[item.rat_corpse]`→`[item.ratman_corpse]` (name "ratman corpse"), and add generic `[item.corpse]` (+2, normal) and `[item.carcass]` (+2, brown), both `nospawn = true`, glyph `%`.

**Step 3: Run, expect PASS** (golden test + full validation via startup paths):
```bash
GOTOOLCHAIN=local go test ./data/ ./internal/content/ ./cmd/... -v 2>&1 | tail -15
```

## Task 3: Verify, push, PR
```bash
cd /Users/arda/projects/tsl/go && gofmt -l . && GOTOOLCHAIN=local go vet ./... && GOTOOLCHAIN=local go test ./... && GOTOOLCHAIN=local go build ./cmd/tsl && echo ALL_GREEN
git add -A && git commit -m "feat(data): monster roster batch 1 — ratman/ghoul canon + graveling/gnoblin/crypt vermin/merman"
git push -u origin monster-roster
gh pr create --title "Monster roster batch 1 (#13)" --body "<documents scope decisions, roster table, deferrals>"
```
Then CodeRabbit loop → merge `--delete-branch` → pull master.

## Done when
`#13` batch-1 merged; the dungeon now spawns ratman/ghoul/graveling/gnoblin/crypt vermin/merman, each dropping an edible corpse; suite green.
