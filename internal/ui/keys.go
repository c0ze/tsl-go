package ui

import "github.com/c0ze/tsl-go/internal/game"

// Front-ends normalise their key events to these names before handing them
// to MenuKey/TargetKey: a single character for printable keys, otherwise the
// browser's KeyboardEvent.key spelling (the terminal maps tcell keys onto it).
const (
	KeyEnter  = "Enter"
	KeyEscape = "Escape"
	KeyUp     = "ArrowUp"
	KeyDown   = "ArrowDown"
	KeyLeft   = "ArrowLeft"
	KeyRight  = "ArrowRight"
)

// PromptResult is what one key did to a menu or targeting prompt.
type PromptResult int

const (
	PromptContinue PromptResult = iota // still open (the selection or cursor may have moved)
	PromptPick                         // confirmed
	PromptCancel                       // dismissed
)

// MenuKey applies one key to a menu of items with selection sel, returning
// the new selection and what happened. A letter that labels an item always
// picks it — so j, k, and q select the 10th, 11th, and 17th items of a long
// list — and otherwise j/k and the arrows move, Enter picks, Esc/q cancels.
// A Yes/No prompt also takes y and n, as the C's prompt_yn does.
func MenuKey(key string, sel int, items []string) (int, PromptResult) {
	n := len(items)
	if n <= 0 {
		return 0, PromptCancel
	}
	if len(key) == 1 && key[0] >= 'a' && int(key[0]-'a') < n {
		return int(key[0] - 'a'), PromptPick
	}
	if n == 2 && items[0] == "Yes" && items[1] == "No" {
		switch key {
		case "y", "Y":
			return 0, PromptPick
		case "n", "N":
			return 1, PromptPick
		}
	}
	switch key {
	case KeyUp, "k":
		return (sel - 1 + n) % n, PromptContinue
	case KeyDown, "j":
		return (sel + 1) % n, PromptContinue
	case KeyEnter:
		return sel, PromptPick
	case KeyEscape, "q":
		return sel, PromptCancel
	}
	return sel, PromptContinue
}

// targetSteps maps cursor keys to their step: the vi keys, diagonals
// included, and the arrows.
var targetSteps = map[string]game.Pos{
	"h": {X: -1}, "l": {X: 1}, "k": {Y: -1}, "j": {Y: 1},
	"y": {X: -1, Y: -1}, "u": {X: 1, Y: -1}, "b": {X: -1, Y: 1}, "n": {X: 1, Y: 1},
	KeyLeft: {X: -1}, KeyRight: {X: 1}, KeyUp: {Y: -1}, KeyDown: {Y: 1},
}

// TargetKey applies one key to a targeting cursor at cur over a w×h map,
// returning the new (clamped) cursor and what happened.
func TargetKey(key string, cur game.Pos, w, h int) (game.Pos, PromptResult) {
	if d, ok := targetSteps[key]; ok {
		return ClampPos(game.Pos{X: cur.X + d.X, Y: cur.Y + d.Y}, w, h), PromptContinue
	}
	switch key {
	case KeyEnter:
		return cur, PromptPick
	case KeyEscape, "q":
		return cur, PromptCancel
	}
	return cur, PromptContinue
}

// ClampPos pins p inside a w×h map (a zero dimension leaves that axis free).
func ClampPos(p game.Pos, w, h int) game.Pos {
	p.X, p.Y = max(p.X, 0), max(p.Y, 0)
	if w > 0 {
		p.X = min(p.X, w-1)
	}
	if h > 0 {
		p.Y = min(p.Y, h-1)
	}
	return p
}
