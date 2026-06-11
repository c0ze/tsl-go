// Package data embeds the game's TOML content so the binary is self-contained
// and runs from any working directory.
package data

import "embed"

//go:embed *.toml
var Files embed.FS
