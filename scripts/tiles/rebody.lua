-- Replace the "body" layer of an animation with a fresh still, in every frame,
-- keeping the hand-drawn layers (the torch flame) untouched.
--
--   aseprite -b --script-param sprite=assets/tiles/anim/player.aseprite \
--     --script-param body=assets/tiles/gen/player.png --script scripts/tiles/rebody.lua
local p = app.params
local spr = app.open(p.sprite)
local img = Image{ fromFile = p.body }
local body
for _, l in ipairs(spr.layers) do if l.name == "body" then body = l end end
assert(body, "no body layer")
for _, fr in ipairs(spr.frames) do
  spr:newCel(body, fr.frameNumber, img, Point(0, 0))
end
spr:saveAs(p.sprite)
