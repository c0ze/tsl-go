package rng

import "strconv"

// Roll returns the sum of n dice each in [1, sides] (the C roll()).
func (g *MT) Roll(n, sides int) int {
	if sides < 1 {
		return n
	}
	sum := 0
	for i := 0; i < n; i++ {
		sum += 1 + g.Intn(sides)
	}
	return sum
}

// Chance returns true with probability x/(x+n) (the C roll_xn). Non-positive x
// or n are clamped to >=1 by shifting the other up, matching the original.
func (g *MT) Chance(x, n int) bool {
	if x < 1 {
		n += 1 - x
		x = 1
	}
	if n < 1 {
		x += 1 - n
		n = 1
	}
	return g.Intn(x+n) < x
}

// RollSpec rolls a dice spec "NdS" or "NdS+M" / "NdS-M" (the C sroll). It
// returns 0 on a malformed spec.
func (g *MT) RollSpec(spec string) int {
	d := indexByte(spec, 'd')
	if d <= 0 || d >= len(spec)-1 {
		return 0
	}
	n, err := strconv.Atoi(spec[:d])
	if err != nil {
		return 0
	}
	rest := spec[d+1:]
	mod := 0
	if i := indexAnySign(rest); i >= 0 {
		m, err := strconv.Atoi(rest[i:])
		if err != nil {
			return 0
		}
		mod = m
		rest = rest[:i]
	}
	sides, err := strconv.Atoi(rest)
	if err != nil {
		return 0
	}
	return g.Roll(n, sides) + mod
}

func indexByte(s string, b byte) int {
	for i := 0; i < len(s); i++ {
		if s[i] == b {
			return i
		}
	}
	return -1
}

func indexAnySign(s string) int {
	for i := 1; i < len(s); i++ { // start at 1: a leading sign isn't a modifier
		if s[i] == '+' || s[i] == '-' {
			return i
		}
	}
	return -1
}
