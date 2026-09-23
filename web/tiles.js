// Graphic-tile renderer for the web front-end. The wasm calls window.tslGrid()
// each frame with the raw cell grid (glyphs + colour/light/dim) and the level
// id; this draws it to a <canvas> from the sprite atlas (web/sprites.png,
// indexed by web/sprites.js — built by scripts/tiles/build.py from the CC0
// Dungeon Crawl Stone Soup tiles plus generated and Aseprite-animated ones),
// tinted by the per-cell torch light. Each level has its own wall and floor
// set; water, lava and the player's torch animate. A glyph with no sprite
// falls back to a coloured letter, so the game stays fully playable. A toggle
// button swaps between this and the ASCII <pre>.
(function () {
  "use strict";
  var PAL = ["#c8bea5", "#b07a3a", "#4a90d9", "#d6504a", "#6fae4a", "#3fb6c4", "#c060c0", "#282c30"]; // internal/ui.PaletteOrder
  var VIEWW = 23, VIEWH = 17; // the player-centred window, in cells

  var canvas = document.getElementById("tiles");
  var pre = document.getElementById("screen");
  var btn = document.getElementById("viewmode");
  var ctx = canvas.getContext("2d");

  var tiles = false;
  try { tiles = localStorage.getItem("tsl-tiles") === "1"; } catch (e) {}

  var SP = window.TSL_SPRITES || { cell: 32, sprites: {}, themes: {} };
  var TILE = SP.cell;
  var atlas = new Image();
  var ready = false;
  atlas.onload = function () { ready = true; draw(); };
  atlas.src = "sprites.png";

  var G = null; // latest grid: {w,h,g,color,bg,bcolor,light,dim,cx,cy,level}

  // Sprite frame for name at time t (ms), or null when the atlas lacks it.
  // Animated sprites loop through their Aseprite frame durations.
  function frameOf(name, t) {
    var s = SP.sprites[name];
    if (!s) return null;
    if (!s.ms) return s.f[0];
    var total = 0, i;
    for (i = 0; i < s.ms.length; i++) total += s.ms[i];
    var at = t % total;
    for (i = 0; i < s.ms.length; i++) { if (at < s.ms[i]) return s.f[i]; at -= s.ms[i]; }
    return s.f[0];
  }
  function blit(name, px, py, t) {
    var f = frameOf(name, t);
    if (!f) return false;
    ctx.drawImage(atlas, f[0], f[1], TILE, TILE, px, py, TILE, TILE);
    return true;
  }

  // A stable per-cell hash (on absolute grid coords) picks between variants,
  // so the dungeon varies without flickering as you move.
  function vhash(x, y) { var h = (x * 374761393 + y * 668265263) | 0; h = (h ^ (h >>> 13)) * 1274126177 | 0; return (h ^ (h >>> 16)) >>> 0; }
  function pick(list, x, y) { return list[vhash(x, y) % list.length]; }

  var WALL = { "■": 1, "│": 1, "─": 1, "└": 1, "┌": 1, "┐": 1, "┘": 1, "├": 1, "┤": 1, "┬": 1, "┴": 1, "┼": 1 };
  // Traps share '^' and differ by colour (data/tiles.toml); brown '^' is lava.
  var TRAP = { 3: "trap_dart", 0: "web_trap", 5: "flash_trap", 2: "trap_plate", 6: "trap_poly" };

  // terrainFor returns the full-cell sprite for a terrain glyph (null when it
  // isn't terrain). '+' and "'" collide with books and keys, so doors are
  // gated on brown (1) — entities never reach here, they're drawn on top.
  function terrainFor(glyph, c, x, y, theme) {
    if (glyph === "·") return pick(theme.floor, x, y);
    if (WALL[glyph]) return pick(theme.wall, x, y);
    if (glyph === "≈") return vhash(x, y) & 1 ? "water_a" : "water_b";
    if (glyph === "^") return c === 1 ? (vhash(x, y) & 1 ? "lava_a" : "lava_b") : (TRAP[c] || "trap_dart");
    if (glyph === "+" && c === 1) return "door_closed";
    if (glyph === "'" && c === 1) return "door_open";
    if (glyph === ">") return "stairs_down";
    if (glyph === "_") return "altar";
    return null;
  }

  // Items and monsters by glyph, colour-gated where one glyph means several
  // things. Arrays are indexed by the colour (internal/ui palette order).
  var BYCOLOR = {
    "!": ["potion_white", "potion_brown", "potion_brilliant_blue", "potion_ruby", "potion_murky", "potion_cyan", "potion_magenta", "potion_black"],
    "?": ["scroll_grey", "scroll_brown", "scroll_blue", "scroll_red", "scroll_green", "scroll_cyan", "scroll_purple", "scroll_grey"],
    "+": ["book_light_gray", "book_light_brown", "book_dark_blue", "book_red", "book_dark_green", "book_cyan", "book_magenta", "book_dark_gray"],
    "/": ["wand_silver", "wand_wood", "wand_lead", "wand_copper", "wand_bronze", "wand_glass", "wand_ivory", "wand_iron"],
    "=": ["ring_tourmaline", "ring_tourmaline", "ring_tourmaline", "ring_ruby", "ring_tourmaline", "ring_tourmaline", "ring_tourmaline", "ring_tourmaline"],
    ")": ["weapon_dagger", "weapon_staff", "weapon_dagger", "weapon_doom", "weapon_dagger", "weapon_crystal", "weapon_dagger", "weapon_dagger"],
    "[": ["armor_chain", "armor_leather", "armor_chain", "armor_leather", "armor_scale", "boots", "armor_rune", "armor_chain"],
    "]": ["helmet", "helmet", "helmet", "helmet", "helmet", "helmet", "helmet", "hat"],
    "%": ["corpse", "food_ration", "corpse", "mushroom", "corpse", "corpse", "corpse", "corpse"]
  };
  var ENTITY = {
    "@": "player", "}": "bow", "\"": "amulet", "(": "cloak", "~": "torch", ":": "arrows", "'": "key",
    "r": "ratman", "k": "m_kobold", "o": "m_gnoblin", "x": "m_slime", "z": "zombie", "D": "m_dragon",
    "Z": "ghoul", "g": "graveling", "v": "crypt_vermin", "T": "m_troll", "i": "imp", "a": "m_spider",
    "w": "wisp", "M": "m_merman", "C": "scarecrow", "d": "dire_wolf", "h": "m_hellhound",
    "f": "frostling", "l": "tentacle", "Y": "m_toad", "q": "burning_skull", "m": "mimic",
    "N": "m_necromancer", "E": "m_mummylich", "A": "m_chrome_angel", "K": "m_gloom_lord",
    "e": "m_sentinel", "t": "technician", "G": "gaoler", "H": "m_horror", "L": "lurker"
  };
  function entityFor(glyph, c) {
    switch (glyph) { // one glyph, two creatures (data/monsters.toml)
      case "S": return c === 5 ? "m_electric_snake" : "m_cave_snake";
      case "b": return c === 6 ? "m_brain" : "m_bat";
      case "O": return c === 3 ? "chainsaw_ogre" : "m_ogre";
      case "j": return c === 3 ? "flame_spirit" : "m_jackal";
      case "s": return c === 1 ? "sludge_dweller" : "m_skeleton";
      case "W": return c === 0 ? "king_of_worms" : "m_wraith";
      case "p": return c === 0 ? "severed_hand" : "goatman";
    }
    var byc = BYCOLOR[glyph];
    if (byc) return byc[c] || byc[0];
    return ENTITY[glyph] || null;
  }

  // The autotiling glyph encodes which neighbours are walls; shading the open
  // sides makes wall masses read as solid blocks with a lit top edge.
  var WALLOPEN = { "■": "NESW", "│": "EW", "─": "NS", "└": "SW", "┌": "NW", "┐": "NE", "┘": "SE", "├": "W", "┤": "E", "┬": "N", "┴": "S", "┼": "" };
  function drawWallEdges(px, py, glyph) {
    var open = WALLOPEN[glyph] || "";
    if (!open) return;
    var e = 3;
    ctx.fillStyle = "rgba(10,8,6,0.45)";
    if (open.indexOf("S") >= 0) ctx.fillRect(px, py + TILE - e, TILE, e);
    if (open.indexOf("W") >= 0) ctx.fillRect(px, py, 2, TILE);
    if (open.indexOf("E") >= 0) ctx.fillRect(px + TILE - 2, py, 2, TILE);
    if (open.indexOf("N") >= 0) { ctx.fillStyle = "rgba(230,215,180,0.18)"; ctx.fillRect(px, py, TILE, 2); }
  }

  // fit scales the canvas up to fill the room left above the HUD (pixelated
  // CSS scaling keeps the pixel art crisp; capped at 3x), or lets it shrink to
  // the screen width on a phone.
  function fit() {
    if (!canvas.width) return;
    var availW = document.documentElement.clientWidth - 32;
    var availH = window.innerHeight - 190;
    var k = Math.min(3, availW / canvas.width, availH / canvas.height);
    canvas.style.width = k > 1 ? Math.floor(canvas.width * k) + "px" : "";
  }
  window.addEventListener("resize", fit);

  var DEFAULT_THEME = "dungeon";
  var animTimer = null;

  function draw() {
    if (animTimer) { clearTimeout(animTimer); animTimer = null; }
    if (!tiles || !G || !ready) return;
    var t = performance.now();
    var theme = SP.themes[G.level] || SP.themes[DEFAULT_THEME] || { wall: [], floor: [] };
    var w = G.w, h = G.h, g = G.g, color = G.color, bg = G.bg, bcolor = G.bcolor, light = G.light, dim = G.dim;
    // The player is the single fully-lit cell (light 255 only at distance 0);
    // centre a window on it so tiles render large and the camera follows.
    var pcx = w >> 1, pcy = h >> 1;
    for (var k = 0; k < light.length; k++) { if (light[k] === 255) { pcx = k % w; pcy = (k / w) | 0; break; } }
    var vw = Math.min(VIEWW, w), vh = Math.min(VIEWH, h);
    var ox = Math.max(0, Math.min(pcx - (vw >> 1), w - vw));
    var oy = Math.max(0, Math.min(pcy - (vh >> 1), h - vh));
    if (canvas.width !== vw * TILE || canvas.height !== vh * TILE) {
      canvas.width = vw * TILE; canvas.height = vh * TILE;
      fit();
    }
    ctx.imageSmoothingEnabled = false;
    ctx.fillStyle = "#000";
    ctx.fillRect(0, 0, canvas.width, canvas.height);
    ctx.textAlign = "center";
    ctx.textBaseline = "middle";
    ctx.font = "bold " + Math.floor(TILE * 0.75) + "px ui-monospace, monospace";
    var animated = false;
    for (var vy = 0; vy < vh; vy++) {
      for (var vx = 0; vx < vw; vx++) {
        var gx = ox + vx, gy = oy + vy, i = gy * w + gx;
        var tg = g[i];                                      // top (composited) glyph
        if (tg === " " || tg === undefined) continue;       // unseen
        var bgl = (bg && bg[i] !== undefined) ? bg[i] : tg; // terrain beneath
        var px = vx * TILE, py = vy * TILE;
        var tn = terrainFor(bgl, bcolor[i], gx, gy, theme);
        if (!tn || !blit(tn, px, py, t)) {                  // un-tiled terrain: floor + letter
          blit(pick(theme.floor, gx, gy), px, py, t);
          if (tg === bgl) { ctx.fillStyle = PAL[bcolor[i]] || PAL[0]; ctx.fillText(bgl, px + TILE / 2, py + TILE / 2 + 1); }
        } else if (WALL[bgl]) {
          drawWallEdges(px, py, bgl);
        }
        if (tn && SP.sprites[tn] && SP.sprites[tn].ms && !dim[i]) animated = true;
        if (tg !== bgl) {                                   // an entity sits on the terrain
          var en = entityFor(tg, color[i]);
          if (en && blit(en, px, py, t)) {
            if (SP.sprites[en].ms) animated = true;
          } else {
            ctx.fillStyle = PAL[color[i]] || PAL[0];
            ctx.fillText(tg, px + TILE / 2, py + TILE / 2 + 1);
          }
        }
        var bri, ov;
        if (dim[i] === 1) { bri = 0.34; ov = "28,34,46"; }    // remembered: cool + dark
        else { bri = 0.30 + 0.70 * (light[i] / 255); ov = "6,5,3"; } // torch falloff
        ctx.fillStyle = "rgba(" + ov + "," + (1 - bri).toFixed(3) + ")";
        ctx.fillRect(px, py, TILE, TILE);
        if (gx === G.cx && gy === G.cy) {
          ctx.strokeStyle = "#ffd24a"; ctx.lineWidth = 2;
          ctx.strokeRect(px + 1, py + 1, TILE - 2, TILE - 2);
        }
      }
    }
    // Keep animating while something visible moves (water, lava, the torch);
    // ~12fps is plenty for 180-240ms frames and cheap on phones.
    if (animated && !document.hidden) animTimer = setTimeout(draw, 80);
  }

  window.tslGrid = function (w, h, top, color, base, bcolor, light, dim, cx, cy, level) {
    G = { w: w, h: h, g: Array.from(top), color: color, bg: Array.from(base), bcolor: bcolor, light: light, dim: dim, cx: cx, cy: cy, level: level };
    draw();
  };
  document.addEventListener("visibilitychange", function () { if (!document.hidden) draw(); });

  function apply() {
    if (tiles) { pre.hidden = true; canvas.hidden = false; draw(); }
    else { canvas.hidden = true; pre.hidden = false; draw(); }
    if (btn) btn.textContent = tiles ? "ASCII" : "Tiles";
  }
  if (btn) btn.addEventListener("click", function () {
    tiles = !tiles;
    try { localStorage.setItem("tsl-tiles", tiles ? "1" : "0"); } catch (e) {}
    apply();
  });

  apply();
})();
