#!/usr/bin/env bash
# Copy the adaptive level scores and their player into web/music/.
#
# The music is composed in cozy-tracker (songs/tsl_<level>.gen.js); build it
# there first:
#   for f in songs/tsl_*.gen.js; do n=$(basename "$f" .gen.js)
#     node "$f" && node tools/json2it.js "songs/$n.json" "build/$n.it"; done
# then run this with COZY pointing at that checkout (default: ../../music/cozy-tracker).
set -euo pipefail

cd "$(dirname "$0")/.."
cozy="${COZY:-../../music/cozy-tracker}"
out="web/music"
[ -f "$cozy/player/cozy-adaptive.js" ] || { echo "no cozy-tracker at $cozy (set COZY=...)" >&2; exit 1; }

mkdir -p "$out/vendor/chiptune3"
cp "$cozy/player/cozy-adaptive.js" "$out/"
cp "$cozy"/player/vendor/chiptune3/{chiptune3.js,chiptune3.worklet.js,libopenmpt.worklet.js,LICENSE} "$out/vendor/chiptune3/"

shopt -s nullglob
n=0
for it in "$cozy"/build/tsl_*.it; do
  level="$(basename "${it%.it}")"; level="${level#tsl_}"
  manifest="${it%.it}.cozy.json"
  [ -f "$manifest" ] || { echo "skip $level: no $manifest" >&2; continue; }
  cp "$it" "$out/$level.it"
  cp "$manifest" "$out/$level.cozy.json"
  n=$((n + 1))
done
echo "synced $n level scores into $out/"
