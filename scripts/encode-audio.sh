#!/usr/bin/env bash
# Encode the original Suno WAVs (assets/audio/src/<level_id>-<n>.wav) into the
# web-optimized MP3s the browser front-end streams (web/audio/<level_id>-<n>.mp3)
# and (re)build the manifest the JS player reads. Re-run after adding tracks.
#
# Bitrate override:  BITRATE=64k scripts/encode-audio.sh
set -euo pipefail

cd "$(dirname "$0")/.."
src="assets/audio/src"
out="web/audio"
bitrate="${BITRATE:-96k}"

command -v ffmpeg >/dev/null || { echo "ffmpeg not found on PATH" >&2; exit 1; }
mkdir -p "$out"

shopt -s nullglob
for f in "$src"/*.wav; do
  name="$(basename "${f%.wav}")"
  ffmpeg -nostdin -v error -y -i "$f" -c:a libmp3lame -b:a "$bitrate" -ac 2 "$out/$name.mp3"
  echo "encoded $name.mp3"
done

# manifest.json: { "<level_id>": ["<id>-1.mp3", ...], ... } in track order.
python3 - "$out" <<'PY'
import json, os, re, sys
out = sys.argv[1]
tracks = {}
for fn in sorted(os.listdir(out)):
    m = re.match(r"^(.*)-(\d+)\.mp3$", fn)
    if not m:
        continue
    level, n = m.group(1), int(m.group(2))
    tracks.setdefault(level, []).append((n, fn))
manifest = {lv: [fn for _, fn in sorted(v)] for lv, v in sorted(tracks.items())}
with open(os.path.join(out, "manifest.json"), "w") as fh:
    json.dump(manifest, fh, indent=2)
    fh.write("\n")
print("wrote manifest.json:", ", ".join(f"{k}x{len(v)}" for k, v in manifest.items()))
PY
