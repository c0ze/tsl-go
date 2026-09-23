-- Assemble numbered frame PNGs into a timed, tagged Aseprite animation.
--
--   aseprite -b --script-param frames=DIR/name_ --script-param count=5 \
--     --script-param ms=180 --script-param tag=cycle \
--     --script-param out=assets/tiles/anim/name.aseprite \
--     --script scripts/tiles/assemble.lua
--
-- Reads DIR/name_0.png .. name_{count-1}.png into one "surface" layer, one
-- frame each, sets every frame's duration, tags the loop, and saves the
-- .aseprite source that build.py exports into the web sprite atlas.

local p = app.params
local count = tonumber(p.count)
local ms = tonumber(p.ms)

local first = Image{ fromFile = p.frames .. "0.png" }
local spr = Sprite(first.width, first.height, ColorMode.RGB)
local layer = spr.layers[1]
layer.name = "surface"

for i = 0, count - 1 do
  if i > 0 then spr:newEmptyFrame() end
  local img = Image{ fromFile = p.frames .. i .. ".png" }
  spr:newCel(layer, i + 1, img, Point(0, 0))
  spr.frames[i + 1].duration = ms / 1000
end

local tag = spr:newTag(1, count)
tag.name = p.tag
tag.aniDir = AniDir.FORWARD

spr:saveAs(p.out)
