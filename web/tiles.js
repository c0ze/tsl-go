// Graphic-tile renderer for the web front-end. The wasm calls window.tslGrid()
// each frame with the raw cell grid (glyphs + colour/light/dim); this draws it
// to a <canvas> as SVG tiles, tinted by the per-cell torch light. A toggle
// button swaps between this and the ASCII <pre>. Terrain tiles fill the cell;
// entities (player/items/monsters) draw as transparent sprites over a floor
// tile, falling back to a coloured letter when no sprite exists yet — so the
// game stays fully playable. Mirrors internal/ui.PaletteOrder.
(function () {
  "use strict";
  var PAL = ["#c8bea5", "#b07a3a", "#4a90d9", "#d6504a", "#6fae4a", "#3fb6c4", "#c060c0", "#282c30"];
  var TILE = 32, VIEWW = 23, VIEWH = 17; // tile px and the player-centred window

  var canvas = document.getElementById("tiles");
  var pre = document.getElementById("screen");
  var btn = document.getElementById("viewmode");
  var ctx = canvas.getContext("2d");

  var tiles = false;
  try { tiles = localStorage.getItem("tsl-tiles") === "1"; } catch (e) {}

  var G = null; // latest grid: {w,h,g,color,light,dim,cx,cy}

  // ---- tile atlas: SVGs rasterised once to images. Terrain fills 100x100;
  //      entity sprites are transparent and centred. -----------------------
  var S = '<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 100 100">';
  var SVG = {
    floor: S + '<rect width="100" height="100" fill="#3f3a2e"/><rect x="4" y="4" width="44" height="44" rx="4" fill="#5b5446"/><rect x="52" y="4" width="44" height="44" rx="4" fill="#555041"/><rect x="4" y="52" width="44" height="44" rx="4" fill="#585143"/><rect x="52" y="52" width="44" height="44" rx="4" fill="#50493b"/><g stroke="#6c6450" stroke-width="1.5" opacity="0.55"><line x1="8" y1="8" x2="44" y2="8"/><line x1="56" y1="8" x2="92" y2="8"/><line x1="8" y1="56" x2="44" y2="56"/><line x1="56" y1="56" x2="92" y2="56"/></g></svg>',
    wall: S + '<rect width="100" height="100" fill="#756c57"/><rect width="100" height="7" fill="#8b8166"/><rect y="93" width="100" height="7" fill="#544d3b"/><g stroke="#544234" stroke-width="3"><line x1="0" y1="34" x2="100" y2="34"/><line x1="0" y1="67" x2="100" y2="67"/><line x1="50" y1="7" x2="50" y2="34"/><line x1="25" y1="34" x2="25" y2="67"/><line x1="75" y1="34" x2="75" y2="67"/><line x1="50" y1="67" x2="50" y2="93"/></g><g stroke="#8b8166" stroke-width="1" opacity="0.55"><line x1="0" y1="36" x2="100" y2="36"/><line x1="0" y1="69" x2="100" y2="69"/></g></svg>',
    water: S + '<rect width="100" height="100" fill="#235a8a"/><rect y="78" width="100" height="22" fill="#1c4870"/><g stroke="#4f8fc4" stroke-width="3" fill="none"><path d="M6,30 q12,-7 24,0 t24,0 t24,0 t24,0"/><path d="M6,55 q12,-7 24,0 t24,0 t24,0 t24,0"/></g><g stroke="#bfe2ff" stroke-width="2" opacity="0.5"><line x1="20" y1="20" x2="34" y2="20"/><line x1="64" y1="44" x2="78" y2="44"/></g></svg>',
    lava: S + '<rect width="100" height="100" fill="#a8380f"/><g fill="#6e2410"><circle cx="16" cy="18" r="7"/><circle cx="92" cy="88" r="8"/></g><g stroke="#ffae3a" stroke-width="3" fill="none"><path d="M14,30 L40,48 L24,70"/><path d="M60,20 L74,44 L62,74"/><path d="M82,52 L94,70"/></g><circle cx="40" cy="80" r="6" fill="#ffce6a"/><circle cx="78" cy="26" r="5" fill="#ffce6a"/></svg>',
    door: S + '<rect width="100" height="100" fill="#4a4438"/><rect x="16" y="8" width="68" height="92" rx="3" fill="#7a4f25" stroke="#16100a" stroke-width="4"/><line x1="24" y1="14" x2="24" y2="100" stroke="#9a6a38" stroke-width="2" opacity="0.6"/><g stroke="#5a3a1c" stroke-width="3"><line x1="40" y1="14" x2="40" y2="100"/><line x1="62" y1="14" x2="62" y2="100"/></g><g stroke="#3a2410" stroke-width="4"><line x1="18" y1="40" x2="82" y2="40"/><line x1="18" y1="74" x2="82" y2="74"/></g><circle cx="72" cy="58" r="5" fill="#e8c34a" stroke="#16100a" stroke-width="2"/></svg>',
    door_open: S + '<rect width="100" height="100" fill="#4a4438"/><rect x="16" y="8" width="68" height="92" rx="3" fill="#14110d"/><rect x="16" y="8" width="13" height="92" fill="#7a4f25" stroke="#16100a" stroke-width="3"/><rect x="71" y="8" width="13" height="92" fill="#5e3c1c" stroke="#16100a" stroke-width="3"/></svg>',
    stairs: S + '<rect width="100" height="100" fill="#3f3a2e"/><rect x="10" y="20" width="80" height="14" fill="#8a8068"/><rect x="18" y="34" width="64" height="13" fill="#746b55"/><rect x="26" y="47" width="48" height="13" fill="#5f5746"/><rect x="34" y="60" width="32" height="12" fill="#4b4537"/><rect x="42" y="72" width="16" height="14" fill="#2f2b20"/><g stroke="#9a9078" stroke-width="1" opacity="0.5"><line x1="10" y1="21" x2="90" y2="21"/><line x1="18" y1="35" x2="82" y2="35"/></g></svg>',

    player: S + '<path d="M30,86 Q50,30 70,86 Z" fill="#a8763a" stroke="#16100a" stroke-width="4" stroke-linejoin="round"/><circle cx="50" cy="34" r="15" fill="#a8763a" stroke="#16100a" stroke-width="4"/><circle cx="50" cy="38" r="9" fill="#e3b88e" stroke="#16100a" stroke-width="2"/><path d="M35,38 Q50,22 65,38" stroke="#16100a" stroke-width="4" fill="none"/><circle cx="46" cy="38" r="1.8" fill="#1a1208"/><circle cx="54" cy="38" r="1.8" fill="#1a1208"/><circle cx="82" cy="28" r="10" fill="#ff8c2a" opacity="0.4"/><line x1="72" y1="52" x2="82" y2="30" stroke="#6a4a2a" stroke-width="5" stroke-linecap="round"/><path d="M82,28 q-6,-9 0,-16 q6,7 0,16 z" fill="#ffb24a" stroke="#16100a" stroke-width="1.5"/><circle cx="82" cy="24" r="3.5" fill="#ffe08a"/></svg>',
    i_potion: S + '<ellipse cx="50" cy="70" rx="13" ry="5" fill="#a83468" opacity="0.5"/><circle cx="50" cy="62" r="18" fill="#d24a8a" stroke="#16100a" stroke-width="3"/><path d="M37,55 A18,18 0 0,1 62,53" stroke="#ee84b4" stroke-width="3" fill="none" opacity="0.6" stroke-linecap="round"/><circle cx="42" cy="56" r="2.5" fill="#ffffff" opacity="0.7"/><rect x="44" y="32" width="12" height="16" fill="#c0427d" stroke="#16100a" stroke-width="3"/><rect x="42" y="22" width="16" height="10" rx="2" fill="#8a5a2c" stroke="#16100a" stroke-width="3"/></svg>',
    i_scroll: S + '<rect x="24" y="28" width="52" height="11" rx="5" fill="#c0a972" stroke="#16100a" stroke-width="3"/><rect x="29" y="34" width="42" height="40" rx="2" fill="#d8c79a" stroke="#16100a" stroke-width="3"/><g stroke="#9a7a4a" stroke-width="2"><line x1="37" y1="46" x2="63" y2="46"/><line x1="37" y1="53" x2="63" y2="53"/><line x1="37" y1="60" x2="59" y2="60"/></g><rect x="24" y="70" width="52" height="11" rx="5" fill="#c0a972" stroke="#16100a" stroke-width="3"/></svg>',
    i_weapon: S + '<polygon points="50,22 56,64 44,64" fill="#ccd2dc" stroke="#16100a" stroke-width="2.5" stroke-linejoin="round"/><line x1="50" y1="28" x2="50" y2="60" stroke="#eef2f8" stroke-width="1.5" opacity="0.8"/><rect x="34" y="64" width="32" height="7" rx="2" fill="#c89a2e" stroke="#16100a" stroke-width="2.5"/><rect x="46" y="71" width="8" height="12" fill="#6a3a1a" stroke="#16100a" stroke-width="2"/><circle cx="50" cy="86" r="5" fill="#c89a2e" stroke="#16100a" stroke-width="2"/></svg>',
    i_bow: S + '<path d="M40,18 Q74,50 40,82" stroke="#16100a" stroke-width="7" fill="none" stroke-linecap="round"/><path d="M40,18 Q74,50 40,82" stroke="#9a6a38" stroke-width="4" fill="none" stroke-linecap="round"/><line x1="40" y1="18" x2="40" y2="82" stroke="#d8c79a" stroke-width="2"/><line x1="34" y1="50" x2="72" y2="50" stroke="#6a4a2a" stroke-width="2.5"/><polygon points="72,46 80,50 72,54" fill="#ccd2dc" stroke="#16100a" stroke-width="1.5"/></svg>',
    i_armor: S + '<path d="M30,30 Q50,24 70,30 L66,72 Q50,82 34,72 Z" fill="#9aa0aa" stroke="#16100a" stroke-width="3" stroke-linejoin="round"/><path d="M36,34 Q50,30 64,34" stroke="#d2d7de" stroke-width="2.5" fill="none" opacity="0.6"/><line x1="50" y1="33" x2="50" y2="74" stroke="#6a707a" stroke-width="2"/><circle cx="34" cy="32" r="3" fill="#c89a2e" stroke="#16100a" stroke-width="1.5"/><circle cx="66" cy="32" r="3" fill="#c89a2e" stroke="#16100a" stroke-width="1.5"/></svg>',
    i_food: S + '<rect x="22" y="50" width="18" height="8" rx="4" fill="#e8dcc0" stroke="#16100a" stroke-width="2.5"/><circle cx="24" cy="54" r="5" fill="#e8dcc0" stroke="#16100a" stroke-width="2"/><ellipse cx="54" cy="56" rx="20" ry="15" fill="#b5572e" stroke="#16100a" stroke-width="3"/><ellipse cx="48" cy="50" rx="8" ry="5" fill="#d57a4a" opacity="0.6"/></svg>',
    i_wand: S + '<line x1="32" y1="72" x2="64" y2="30" stroke="#16100a" stroke-width="7" stroke-linecap="round"/><line x1="32" y1="72" x2="64" y2="30" stroke="#8a5a2c" stroke-width="4" stroke-linecap="round"/><circle cx="66" cy="27" r="14" fill="#7a3a9a" opacity="0.4"/><circle cx="66" cy="27" r="8" fill="#b76ad6" stroke="#16100a" stroke-width="2"/><g stroke="#e6c0f6" stroke-width="2"><line x1="36" y1="28" x2="44" y2="28"/><line x1="40" y1="24" x2="40" y2="32"/></g></svg>',
    i_ring: S + '<circle cx="50" cy="58" r="16" fill="none" stroke="#16100a" stroke-width="10"/><circle cx="50" cy="58" r="16" fill="none" stroke="#e8c34a" stroke-width="6"/><polygon points="50,28 60,40 50,52 40,40" fill="#4a90d9" stroke="#16100a" stroke-width="2"/><line x1="46" y1="38" x2="50" y2="34" stroke="#bfe2ff" stroke-width="2"/></svg>',
    i_light: S + '<line x1="50" y1="84" x2="50" y2="46" stroke="#16100a" stroke-width="8" stroke-linecap="round"/><line x1="50" y1="84" x2="50" y2="46" stroke="#6a4a2a" stroke-width="5" stroke-linecap="round"/><rect x="42" y="40" width="16" height="12" rx="2" fill="#5e3c1c" stroke="#16100a" stroke-width="2"/><circle cx="50" cy="28" r="16" fill="#ff8c2a" opacity="0.4"/><path d="M50,44 q-9,-14 0,-26 q9,11 0,26 z" fill="#ffb24a" stroke="#16100a" stroke-width="1.5"/><circle cx="50" cy="28" r="5" fill="#ffe08a"/></svg>',
    i_book: S + '<rect x="30" y="28" width="40" height="48" rx="3" fill="#3a6ab0" stroke="#16100a" stroke-width="3"/><rect x="66" y="30" width="6" height="44" fill="#d8d2c0" stroke="#16100a" stroke-width="1.5"/><line x1="35" y1="32" x2="35" y2="72" stroke="#6a9ad8" stroke-width="2" opacity="0.6"/><circle cx="48" cy="52" r="6" fill="none" stroke="#ffd24a" stroke-width="2.5"/></svg>',
    m_rat: S + '<path d="M68,54 q24,0 26,20" stroke="#16100a" stroke-width="5" fill="none"/><path d="M68,54 q24,0 26,20" stroke="#7a6a54" stroke-width="2.5" fill="none"/><ellipse cx="50" cy="56" rx="23" ry="15" fill="#968570" stroke="#16100a" stroke-width="3"/><ellipse cx="44" cy="50" rx="11" ry="5" fill="#a89579" opacity="0.5"/><circle cx="28" cy="52" r="12" fill="#9c8b76" stroke="#16100a" stroke-width="3"/><circle cx="23" cy="41" r="6" fill="#8a7a66" stroke="#16100a" stroke-width="2"/><circle cx="24" cy="51" r="2" fill="#161009"/><circle cx="17" cy="56" r="2.5" fill="#d28a8a" stroke="#16100a" stroke-width="1"/></svg>',
    m_bat: S + '<path d="M50,48 Q26,28 12,44 Q24,48 20,58 Q34,54 50,54 Z" fill="#3fb6c4" stroke="#16100a" stroke-width="3" stroke-linejoin="round"/><path d="M50,48 Q74,28 88,44 Q76,48 80,58 Q66,54 50,54 Z" fill="#3fb6c4" stroke="#16100a" stroke-width="3" stroke-linejoin="round"/><ellipse cx="50" cy="52" rx="9" ry="11" fill="#3fb6c4" stroke="#16100a" stroke-width="3"/><polygon points="44,42 46,32 50,44" fill="#3fb6c4" stroke="#16100a" stroke-width="1.5"/><polygon points="56,42 54,32 50,44" fill="#3fb6c4" stroke="#16100a" stroke-width="1.5"/><circle cx="46" cy="50" r="1.8" fill="#161009"/><circle cx="54" cy="50" r="1.8" fill="#161009"/></svg>',
    m_kobold: S + '<polygon points="32,40 26,22 40,42" fill="#5e9a3c" stroke="#16100a" stroke-width="2" stroke-linejoin="round"/><polygon points="68,40 74,22 60,42" fill="#5e9a3c" stroke="#16100a" stroke-width="2" stroke-linejoin="round"/><circle cx="50" cy="52" r="20" fill="#6fae4a" stroke="#16100a" stroke-width="3"/><path d="M36,40 Q50,34 64,40" stroke="#8fce6a" stroke-width="3" fill="none" opacity="0.5"/><path d="M34,60 Q50,72 66,60" fill="#4e8a2c" opacity="0.5"/><circle cx="42" cy="50" r="4" fill="#ffd24a" stroke="#16100a" stroke-width="1.4"/><circle cx="58" cy="50" r="4" fill="#ffd24a" stroke="#16100a" stroke-width="1.4"/><circle cx="42" cy="51" r="1.6" fill="#1a1208"/><circle cx="58" cy="51" r="1.6" fill="#1a1208"/><path d="M43,62 Q50,67 57,62" stroke="#16100a" stroke-width="1.8" fill="none"/><polygon points="48,64 52,64 50,69" fill="#eef2d0"/></svg>',
    m_gnoblin: S + '<line x1="62" y1="30" x2="66" y2="60" stroke="#6a4a2a" stroke-width="6" stroke-linecap="round"/><circle cx="62" cy="26" r="8" fill="#8a5e2c" stroke="#16100a" stroke-width="2.5"/><path d="M36,46 L52,46 L50,72 L38,72 Z" fill="#5e8a3a" stroke="#16100a" stroke-width="3" stroke-linejoin="round"/><polygon points="33,34 27,24 39,36" fill="#5e9a3c" stroke="#16100a" stroke-width="1.5"/><circle cx="44" cy="36" r="11" fill="#6fae4a" stroke="#16100a" stroke-width="3"/><circle cx="40" cy="35" r="3" fill="#ffd24a" stroke="#16100a" stroke-width="1"/><circle cx="48" cy="35" r="3" fill="#ffd24a" stroke="#16100a" stroke-width="1"/></svg>',
    m_snake: S + '<path d="M22,74 Q52,74 50,48 Q48,28 70,28" stroke="#16100a" stroke-width="13" fill="none" stroke-linecap="round"/><path d="M22,74 Q52,74 50,48 Q48,28 70,28" stroke="#6fae4a" stroke-width="9" fill="none" stroke-linecap="round"/><circle cx="72" cy="28" r="10" fill="#6fae4a" stroke="#16100a" stroke-width="2.5"/><circle cx="76" cy="25" r="2" fill="#ffd24a"/><path d="M82,28 l9,-3 m-9,3 l9,3" stroke="#d6504a" stroke-width="1.5" fill="none"/></svg>',
    m_jackal: S + '<g stroke="#8a5e2c" stroke-width="4" stroke-linecap="round"><line x1="38" y1="58" x2="36" y2="74"/><line x1="48" y1="60" x2="48" y2="76"/><line x1="58" y1="60" x2="60" y2="76"/><line x1="66" y1="56" x2="68" y2="72"/></g><ellipse cx="50" cy="54" rx="22" ry="12" fill="#a8763a" stroke="#16100a" stroke-width="3"/><circle cx="72" cy="48" r="10" fill="#a8763a" stroke="#16100a" stroke-width="2.5"/><polygon points="80,48 92,50 80,54" fill="#a8763a" stroke="#16100a" stroke-width="1.5"/><polygon points="68,40 70,28 76,42" fill="#a8763a" stroke="#16100a" stroke-width="1.5"/><circle cx="74" cy="46" r="1.8" fill="#161009"/></svg>',
    m_slime: S + '<path d="M22,70 Q16,42 50,40 Q84,42 78,70 Q62,76 50,73 Q38,76 22,70 Z" fill="#6fae4a" stroke="#16100a" stroke-width="3" stroke-linejoin="round"/><ellipse cx="40" cy="50" rx="11" ry="6" fill="#9fd06a" opacity="0.6"/><circle cx="42" cy="58" r="5" fill="#eef7e0" stroke="#16100a" stroke-width="2"/><circle cx="60" cy="58" r="5" fill="#eef7e0" stroke="#16100a" stroke-width="2"/><circle cx="42" cy="59" r="2" fill="#1a1208"/><circle cx="60" cy="59" r="2" fill="#1a1208"/></svg>',
    m_zombie: S + '<line x1="44" y1="48" x2="28" y2="58" stroke="#6a8050" stroke-width="7" stroke-linecap="round"/><line x1="56" y1="48" x2="40" y2="60" stroke="#6a8050" stroke-width="7" stroke-linecap="round"/><path d="M40,40 L60,40 L57,74 L43,74 Z" fill="#6a8050" stroke="#16100a" stroke-width="3" stroke-linejoin="round"/><circle cx="50" cy="28" r="11" fill="#8aa06a" stroke="#16100a" stroke-width="3"/><circle cx="46" cy="27" r="2" fill="#161009"/><circle cx="54" cy="27" r="2" fill="#161009"/><path d="M45,33 L55,33" stroke="#16100a" stroke-width="1.5"/></svg>',
    m_skeleton: S + '<line x1="50" y1="44" x2="50" y2="72" stroke="#d8d2c0" stroke-width="4"/><g stroke="#d8d2c0" stroke-width="3" fill="none"><path d="M50,50 Q40,52 38,58"/><path d="M50,50 Q60,52 62,58"/><path d="M50,60 Q42,62 40,68"/><path d="M50,60 Q58,62 60,68"/></g><circle cx="50" cy="30" r="12" fill="#e8e4d8" stroke="#16100a" stroke-width="3"/><circle cx="45" cy="30" r="3" fill="#161009"/><circle cx="55" cy="30" r="3" fill="#161009"/><polygon points="50,34 47,40 53,40" fill="#161009"/></svg>',
    m_dragon: S + '<polygon points="66,40 88,32 81,59" fill="#8a2f1f" stroke="#16100a" stroke-width="2.5" stroke-linejoin="round"/><polygon points="70,38 84,33 78,46" fill="#aa3d28" opacity="0.6"/><path d="M26,58 L10,52 L19,65 Z" fill="#cf4a30" stroke="#16100a" stroke-width="2.5" stroke-linejoin="round"/><path d="M26,58 Q28,40 50,40 Q76,40 82,56 Q74,72 50,74 Q30,72 26,58 Z" fill="#cf4a30" stroke="#16100a" stroke-width="3" stroke-linejoin="round"/><path d="M30,66 Q50,76 74,64 Q68,74 50,74 Q34,74 30,66 Z" fill="#8f2a18" opacity="0.5"/><path d="M28,50 Q44,46 60,50" stroke="#e8664a" stroke-width="3" fill="none" opacity="0.6" stroke-linecap="round"/><polygon points="66,42 74,26 70,44" fill="#e8c08e" stroke="#16100a" stroke-width="2" stroke-linejoin="round"/><ellipse cx="58" cy="54" rx="4" ry="5" fill="#ffe27a" stroke="#16100a" stroke-width="1.5"/><rect x="57" y="50" width="2" height="9" fill="#16100a"/></svg>',
    m_ghoul: S + '<line x1="42" y1="46" x2="26" y2="40" stroke="#5e7a4a" stroke-width="6" stroke-linecap="round"/><line x1="58" y1="46" x2="74" y2="40" stroke="#5e7a4a" stroke-width="6" stroke-linecap="round"/><g stroke="#16100a" stroke-width="2"><line x1="24" y1="38" x2="20" y2="34"/><line x1="76" y1="38" x2="80" y2="34"/></g><path d="M40,42 L60,42 L56,74 L44,74 Z" fill="#5e7a4a" stroke="#16100a" stroke-width="3" stroke-linejoin="round"/><ellipse cx="50" cy="28" rx="10" ry="12" fill="#7a9a5a" stroke="#16100a" stroke-width="3"/><circle cx="46" cy="28" r="2.5" fill="#d6504a"/><circle cx="54" cy="28" r="2.5" fill="#d6504a"/><path d="M44,34 Q50,38 56,34" stroke="#16100a" stroke-width="1.5" fill="none"/></svg>',
    m_graveling: S + '<path d="M30,50 Q34,40 50,40 Q66,40 70,50 L66,72 Q50,78 34,72 Z" fill="#9a6a3a" stroke="#16100a" stroke-width="3" stroke-linejoin="round"/><ellipse cx="42" cy="50" rx="9" ry="5" fill="#b4844c" opacity="0.5"/><circle cx="43" cy="54" r="4" fill="#ffd24a" stroke="#16100a" stroke-width="1.4"/><circle cx="57" cy="54" r="4" fill="#ffd24a" stroke="#16100a" stroke-width="1.4"/><circle cx="43" cy="55" r="1.6" fill="#1a1208"/><circle cx="57" cy="55" r="1.6" fill="#1a1208"/><path d="M42,64 L46,62 L50,64 L54,62 L58,64" stroke="#16100a" stroke-width="1.5" fill="none"/></svg>',
    m_vermin: S + '<ellipse cx="50" cy="54" rx="20" ry="14" fill="#8a8884" stroke="#16100a" stroke-width="3"/><line x1="50" y1="40" x2="50" y2="68" stroke="#16100a" stroke-width="2"/><g stroke="#5a5854" stroke-width="3" stroke-linecap="round"><line x1="32" y1="48" x2="22" y2="42"/><line x1="32" y1="54" x2="20" y2="54"/><line x1="32" y1="60" x2="22" y2="66"/><line x1="68" y1="48" x2="78" y2="42"/><line x1="68" y1="54" x2="80" y2="54"/><line x1="68" y1="60" x2="78" y2="66"/></g><circle cx="44" cy="46" r="2" fill="#161009"/><circle cx="56" cy="46" r="2" fill="#161009"/></svg>',
    m_ogre: S + '<line x1="64" y1="34" x2="74" y2="66" stroke="#6a4a2a" stroke-width="8" stroke-linecap="round"/><ellipse cx="74" cy="30" rx="11" ry="9" fill="#8a5e2c" stroke="#16100a" stroke-width="2.5"/><path d="M34,40 Q50,34 66,40 L62,76 Q50,82 38,76 Z" fill="#a8763a" stroke="#16100a" stroke-width="3" stroke-linejoin="round"/><line x1="36" y1="48" x2="24" y2="62" stroke="#a8763a" stroke-width="8" stroke-linecap="round"/><circle cx="50" cy="28" r="12" fill="#b4844c" stroke="#16100a" stroke-width="3"/><circle cx="45" cy="28" r="2.5" fill="#161009"/><circle cx="55" cy="28" r="2.5" fill="#161009"/><polygon points="46,34 48,40 50,34" fill="#eef2d0"/></svg>',
    m_troll: S + '<path d="M38,36 Q50,30 62,36 L58,78 Q50,84 42,78 Z" fill="#5e8a4a" stroke="#16100a" stroke-width="3" stroke-linejoin="round"/><line x1="40" y1="44" x2="28" y2="64" stroke="#5e8a4a" stroke-width="8" stroke-linecap="round"/><line x1="60" y1="44" x2="72" y2="64" stroke="#5e8a4a" stroke-width="8" stroke-linecap="round"/><circle cx="50" cy="26" r="11" fill="#6fae4a" stroke="#16100a" stroke-width="3"/><circle cx="46" cy="26" r="2.5" fill="#161009"/><circle cx="54" cy="26" r="2.5" fill="#161009"/><polygon points="45,33 47,28 49,33" fill="#eef2d0"/><polygon points="51,33 53,28 55,33" fill="#eef2d0"/></svg>',
    m_imp: S + '<path d="M48,46 Q30,34 22,46 Q32,48 30,56 Q40,52 48,54 Z" fill="#a32d2d" stroke="#16100a" stroke-width="2.5" stroke-linejoin="round"/><path d="M52,46 Q70,34 78,46 Q68,48 70,56 Q60,52 52,54 Z" fill="#a32d2d" stroke="#16100a" stroke-width="2.5" stroke-linejoin="round"/><circle cx="50" cy="50" r="13" fill="#d6504a" stroke="#16100a" stroke-width="3"/><polygon points="40,42 36,30 46,44" fill="#d6504a" stroke="#16100a" stroke-width="1.5"/><polygon points="60,42 64,30 54,44" fill="#d6504a" stroke="#16100a" stroke-width="1.5"/><circle cx="45" cy="50" r="2.5" fill="#ffd24a"/><circle cx="55" cy="50" r="2.5" fill="#ffd24a"/><path d="M44,56 Q50,60 56,56" stroke="#16100a" stroke-width="1.5" fill="none"/></svg>',
    m_spider: S + '<g stroke="#161009" stroke-width="2.5" fill="none" stroke-linecap="round"><path d="M40,52 Q28,46 22,52"/><path d="M40,56 Q26,56 20,62"/><path d="M40,60 Q28,66 24,72"/><path d="M60,52 Q72,46 78,52"/><path d="M60,56 Q74,56 80,62"/><path d="M60,60 Q72,66 76,72"/></g><ellipse cx="50" cy="58" rx="15" ry="13" fill="#c060c0" stroke="#16100a" stroke-width="3"/><circle cx="50" cy="42" r="9" fill="#c060c0" stroke="#16100a" stroke-width="2.5"/><circle cx="46" cy="40" r="2" fill="#ffd24a"/><circle cx="54" cy="40" r="2" fill="#ffd24a"/></svg>',
    m_wisp: S + '<circle cx="50" cy="50" r="18" fill="#3fb6c4" opacity="0.3"/><circle cx="50" cy="50" r="11" fill="#7fe0ea" stroke="#16100a" stroke-width="2"/><circle cx="50" cy="50" r="5" fill="#eafcff"/><g stroke="#7fe0ea" stroke-width="2"><line x1="50" y1="26" x2="50" y2="20"/><line x1="50" y1="74" x2="50" y2="80"/><line x1="26" y1="50" x2="20" y2="50"/><line x1="74" y1="50" x2="80" y2="50"/></g></svg>',
    m_merman: S + '<line x1="68" y1="26" x2="68" y2="70" stroke="#8a8884" stroke-width="3"/><path d="M62,26 L62,18 M68,26 L68,16 M74,26 L74,18" stroke="#8a8884" stroke-width="3" fill="none"/><path d="M40,40 Q50,34 60,40 L56,70 Q50,76 44,70 Z" fill="#2f8fa0" stroke="#16100a" stroke-width="3" stroke-linejoin="round"/><path d="M44,70 Q40,80 50,82 Q60,80 56,70 Z" fill="#2f8fa0" stroke="#16100a" stroke-width="2.5"/><circle cx="50" cy="30" r="10" fill="#3fb6c4" stroke="#16100a" stroke-width="3"/><circle cx="46" cy="30" r="2" fill="#161009"/><circle cx="54" cy="30" r="2" fill="#161009"/></svg>',
    m_scarecrow: S + '<line x1="24" y1="42" x2="76" y2="42" stroke="#6a4a2a" stroke-width="4" stroke-linecap="round"/><path d="M42,38 L58,38 L54,74 L46,74 Z" fill="#b08a4a" stroke="#16100a" stroke-width="3" stroke-linejoin="round"/><g stroke="#c9a25a" stroke-width="2"><line x1="46" y1="74" x2="42" y2="82"/><line x1="54" y1="74" x2="58" y2="82"/></g><circle cx="50" cy="28" r="10" fill="#c9a25a" stroke="#16100a" stroke-width="3"/><polygon points="40,18 50,24 60,18 56,28 44,28" fill="#8a6a2c" stroke="#16100a" stroke-width="2" stroke-linejoin="round"/><path d="M46,28 l-3,3 m3,-3 l3,3" stroke="#16100a" stroke-width="1.5"/><path d="M54,28 l-3,3 m3,-3 l3,3" stroke="#16100a" stroke-width="1.5"/></svg>',
    m_wraith: S + '<path d="M30,40 Q50,24 70,40 L70,72 Q64,66 58,74 Q52,66 50,76 Q48,66 42,74 Q36,66 30,72 Z" fill="#7a3a8a" stroke="#16100a" stroke-width="3" stroke-linejoin="round" opacity="0.92"/><path d="M40,42 Q50,34 60,42" stroke="#16100a" stroke-width="3" fill="none"/><circle cx="44" cy="44" r="2.5" fill="#e0a0f0"/><circle cx="56" cy="44" r="2.5" fill="#e0a0f0"/></svg>',
    m_direwolf: S + '<g stroke="#5e3c1c" stroke-width="5" stroke-linecap="round"><line x1="34" y1="58" x2="32" y2="76"/><line x1="46" y1="60" x2="46" y2="78"/><line x1="58" y1="60" x2="60" y2="78"/><line x1="68" y1="56" x2="70" y2="74"/></g><ellipse cx="52" cy="54" rx="24" ry="13" fill="#7a5230" stroke="#16100a" stroke-width="3"/><circle cx="76" cy="46" r="11" fill="#7a5230" stroke="#16100a" stroke-width="2.5"/><polygon points="84,46 96,48 84,52" fill="#7a5230" stroke="#16100a" stroke-width="1.5"/><polygon points="70,38 72,24 80,40" fill="#7a5230" stroke="#16100a" stroke-width="1.5"/><circle cx="78" cy="44" r="1.8" fill="#d6504a"/></svg>',
    m_hellhound: S + '<g stroke="#7a1f1f" stroke-width="5" stroke-linecap="round"><line x1="36" y1="58" x2="34" y2="76"/><line x1="48" y1="60" x2="48" y2="78"/><line x1="58" y1="60" x2="60" y2="78"/><line x1="68" y1="56" x2="70" y2="74"/></g><ellipse cx="52" cy="54" rx="22" ry="12" fill="#a32d2d" stroke="#16100a" stroke-width="3"/><circle cx="74" cy="48" r="10" fill="#a32d2d" stroke="#16100a" stroke-width="2.5"/><polygon points="82,48 92,50 82,54" fill="#a32d2d" stroke="#16100a" stroke-width="1.5"/><path d="M40,46 q-4,-12 4,-18 q2,8 6,6 q-2,10 -10,12 z" fill="#ffae3a" opacity="0.85"/><circle cx="76" cy="46" r="2" fill="#ffd24a"/></svg>',
    m_frostling: S + '<path d="M36,46 Q50,40 64,46 L60,72 Q50,78 40,72 Z" fill="#8fd0e0" stroke="#16100a" stroke-width="3" stroke-linejoin="round"/><circle cx="50" cy="34" r="11" fill="#bfeaf2" stroke="#16100a" stroke-width="3"/><polygon points="40,26 50,10 60,26" fill="#bfeaf2" stroke="#16100a" stroke-width="2" stroke-linejoin="round"/><circle cx="46" cy="34" r="2.5" fill="#2f6f8a"/><circle cx="54" cy="34" r="2.5" fill="#2f6f8a"/><g stroke="#eafcff" stroke-width="1.5"><line x1="30" y1="54" x2="24" y2="54"/><line x1="70" y1="54" x2="76" y2="54"/></g></svg>',
    m_goatman: S + '<path d="M40,42 L60,42 L56,74 L44,74 Z" fill="#8a5e2c" stroke="#16100a" stroke-width="3" stroke-linejoin="round"/><circle cx="50" cy="30" r="11" fill="#a8763a" stroke="#16100a" stroke-width="3"/><path d="M42,24 Q34,14 38,8" stroke="#e8dcc0" stroke-width="4" fill="none" stroke-linecap="round"/><path d="M58,24 Q66,14 62,8" stroke="#e8dcc0" stroke-width="4" fill="none" stroke-linecap="round"/><circle cx="46" cy="30" r="2" fill="#161009"/><circle cx="54" cy="30" r="2" fill="#161009"/><path d="M47,36 L53,36" stroke="#16100a" stroke-width="1.5"/></svg>',
    m_tentacle: S + '<path d="M50,78 Q40,60 52,48 Q64,38 50,24" stroke="#16100a" stroke-width="14" fill="none" stroke-linecap="round"/><path d="M50,78 Q40,60 52,48 Q64,38 50,24" stroke="#4e8a2c" stroke-width="10" fill="none" stroke-linecap="round"/><g fill="#7fbf4f"><circle cx="46" cy="56" r="3"/><circle cx="56" cy="44" r="3"/><circle cx="50" cy="30" r="3"/></g></svg>',
    m_toad: S + '<ellipse cx="50" cy="58" rx="26" ry="18" fill="#5e8a3a" stroke="#16100a" stroke-width="3"/><ellipse cx="40" cy="50" rx="10" ry="6" fill="#7fae4f" opacity="0.5"/><circle cx="38" cy="44" r="7" fill="#6fae4a" stroke="#16100a" stroke-width="2.5"/><circle cx="62" cy="44" r="7" fill="#6fae4a" stroke="#16100a" stroke-width="2.5"/><circle cx="38" cy="44" r="2.5" fill="#161009"/><circle cx="62" cy="44" r="2.5" fill="#161009"/><path d="M34,64 Q50,72 66,64" stroke="#16100a" stroke-width="2" fill="none"/></svg>',
    m_burnskull: S + '<path d="M38,34 q-6,-14 2,-22 q2,8 6,6 q0,8 6,8 q-2,10 -14,8 z" fill="#ffae3a" opacity="0.8"/><path d="M62,34 q6,-14 -2,-22 q-2,8 -6,6 q0,8 -6,8 q2,10 14,8 z" fill="#ffae3a" opacity="0.8"/><circle cx="50" cy="46" r="14" fill="#e8e4d8" stroke="#16100a" stroke-width="3"/><circle cx="44" cy="44" r="4" fill="#d6504a"/><circle cx="56" cy="44" r="4" fill="#d6504a"/><polygon points="50,50 46,56 54,56" fill="#161009"/><path d="M40,60 L60,60" stroke="#e8e4d8" stroke-width="4"/><g stroke="#16100a" stroke-width="1.2"><line x1="44" y1="60" x2="44" y2="66"/><line x1="50" y1="60" x2="50" y2="67"/><line x1="56" y1="60" x2="56" y2="66"/></g></svg>',
    m_mimic: S + '<path d="M26,46 Q26,38 34,38 L66,38 Q74,38 74,46 L74,72 L26,72 Z" fill="#7a4f25" stroke="#16100a" stroke-width="3" stroke-linejoin="round"/><rect x="22" y="44" width="56" height="10" fill="#5e3c1c" stroke="#16100a" stroke-width="2.5"/><g fill="#eef2d0" stroke="#16100a" stroke-width="1"><polygon points="30,44 34,52 38,44"/><polygon points="44,44 48,52 52,44"/><polygon points="58,44 62,52 66,44"/><polygon points="34,54 38,46 30,46"/><polygon points="48,54 52,46 44,46"/></g><circle cx="40" cy="64" r="3" fill="#d6504a"/><circle cx="60" cy="64" r="3" fill="#d6504a"/><rect x="46" y="58" width="8" height="6" rx="1" fill="#c89a2e" stroke="#16100a" stroke-width="1.5"/></svg>',
    m_necromancer: S + '<path d="M32,42 Q50,26 68,42 L64,76 Q50,82 36,76 Z" fill="#4a4850" stroke="#16100a" stroke-width="3" stroke-linejoin="round"/><path d="M40,44 Q50,36 60,44 L58,58 L42,58 Z" fill="#2a2830" stroke="#16100a" stroke-width="2"/><circle cx="46" cy="50" r="2.5" fill="#7fe0ea"/><circle cx="54" cy="50" r="2.5" fill="#7fe0ea"/><circle cx="72" cy="60" r="6" fill="#7fe0ea" opacity="0.6"/><circle cx="72" cy="60" r="3" fill="#bfffff"/></svg>',
    m_mummylich: S + '<path d="M38,40 L62,40 L58,78 L42,78 Z" fill="#c8b890" stroke="#16100a" stroke-width="3" stroke-linejoin="round"/><g stroke="#9a8a64" stroke-width="2"><line x1="40" y1="50" x2="60" y2="48"/><line x1="40" y1="58" x2="60" y2="60"/><line x1="42" y1="68" x2="58" y2="66"/></g><ellipse cx="50" cy="30" rx="11" ry="13" fill="#d8c8a0" stroke="#16100a" stroke-width="3"/><g stroke="#9a8a64" stroke-width="2"><line x1="40" y1="28" x2="60" y2="26"/><line x1="40" y1="34" x2="60" y2="36"/></g><circle cx="45" cy="30" r="5" fill="#c060c0" opacity="0.4"/><circle cx="55" cy="30" r="5" fill="#c060c0" opacity="0.4"/><circle cx="45" cy="30" r="3" fill="#c060c0"/><circle cx="55" cy="30" r="3" fill="#c060c0"/></svg>',
    m_angel: S + '<path d="M50,40 Q28,30 16,42 Q30,42 26,52 Q40,46 50,50 Z" fill="#cfeaf2" stroke="#16100a" stroke-width="2.5" stroke-linejoin="round"/><path d="M50,40 Q72,30 84,42 Q70,42 74,52 Q60,46 50,50 Z" fill="#cfeaf2" stroke="#16100a" stroke-width="2.5" stroke-linejoin="round"/><path d="M42,44 L58,44 L55,74 L45,74 Z" fill="#9ac4d2" stroke="#16100a" stroke-width="3" stroke-linejoin="round"/><circle cx="50" cy="32" r="9" fill="#cfeaf2" stroke="#16100a" stroke-width="3"/><ellipse cx="50" cy="20" rx="9" ry="3" fill="none" stroke="#ffd24a" stroke-width="2.5"/><circle cx="46" cy="32" r="1.8" fill="#2f6f8a"/><circle cx="54" cy="32" r="1.8" fill="#2f6f8a"/></svg>',
    m_gloomlord: S + '<path d="M30,44 Q50,26 70,44 L66,76 Q50,82 34,76 Z" fill="#3a2a44" stroke="#16100a" stroke-width="3" stroke-linejoin="round"/><path d="M38,46 Q50,36 62,46 L60,60 L40,60 Z" fill="#1a1422" stroke="#16100a" stroke-width="2"/><circle cx="45" cy="52" r="6" fill="#c060c0" opacity="0.35"/><circle cx="55" cy="52" r="6" fill="#c060c0" opacity="0.35"/><circle cx="45" cy="52" r="3" fill="#c060c0"/><circle cx="55" cy="52" r="3" fill="#c060c0"/></svg>',
    m_sentinel: S + '<polygon points="50,22 78,50 50,78 22,50" fill="#2f8fa0" stroke="#16100a" stroke-width="3" stroke-linejoin="round"/><polygon points="50,30 70,50 50,70 30,50" fill="#3fb6c4" stroke="#16100a" stroke-width="1.5"/><circle cx="50" cy="50" r="9" fill="#eafcff" stroke="#16100a" stroke-width="2"/><circle cx="50" cy="50" r="4" fill="#16100a"/><circle cx="48" cy="48" r="1.5" fill="#eafcff"/></svg>',
    m_technician: S + '<path d="M40,42 L60,42 L57,76 L43,76 Z" fill="#6a6862" stroke="#16100a" stroke-width="3" stroke-linejoin="round"/><line x1="58" y1="48" x2="72" y2="38" stroke="#6a6862" stroke-width="6" stroke-linecap="round"/><rect x="68" y="30" width="8" height="12" rx="2" fill="#c89a2e" stroke="#16100a" stroke-width="2"/><circle cx="50" cy="30" r="10" fill="#8a8884" stroke="#16100a" stroke-width="3"/><rect x="42" y="26" width="16" height="6" rx="2" fill="#4a90d9" stroke="#16100a" stroke-width="1.5"/></svg>',
    m_gaoler: S + '<path d="M36,40 Q50,32 64,40 L60,78 Q50,84 40,78 Z" fill="#5a5854" stroke="#16100a" stroke-width="3" stroke-linejoin="round"/><circle cx="50" cy="28" r="11" fill="#8a8884" stroke="#16100a" stroke-width="3"/><circle cx="46" cy="28" r="2" fill="#161009"/><circle cx="54" cy="28" r="2" fill="#161009"/><circle cx="70" cy="58" r="6" fill="none" stroke="#c89a2e" stroke-width="3"/><line x1="70" y1="62" x2="70" y2="74" stroke="#c89a2e" stroke-width="3"/><line x1="70" y1="68" x2="75" y2="68" stroke="#c89a2e" stroke-width="2.5"/></svg>',
    m_horror: S + '<path d="M24,58 Q20,38 38,36 Q44,22 56,32 Q74,28 76,48 Q86,56 74,68 Q70,80 56,72 Q48,82 38,72 Q24,72 24,58 Z" fill="#6a2a6a" stroke="#16100a" stroke-width="3" stroke-linejoin="round"/><circle cx="40" cy="50" r="4" fill="#e0a0f0"/><circle cx="58" cy="46" r="4" fill="#e0a0f0"/><circle cx="50" cy="62" r="3" fill="#e0a0f0"/><g stroke="#9a4a9a" stroke-width="3" stroke-linecap="round"><line x1="30" y1="66" x2="26" y2="78"/><line x1="56" y1="70" x2="58" y2="82"/></g></svg>',
    m_lurker: S + '<path d="M26,56 Q22,40 40,40 Q50,28 60,40 Q78,40 74,56 Q78,70 62,68 Q50,74 38,68 Q22,70 26,56 Z" fill="#3a6a3a" stroke="#16100a" stroke-width="3" stroke-linejoin="round"/><g stroke="#2a5a2a" stroke-width="5" stroke-linecap="round"><path d="M38,68 Q34,80 40,84"/><path d="M50,70 Q50,82 54,86"/><path d="M62,68 Q66,80 60,84"/></g><circle cx="42" cy="50" r="4" fill="#ffd24a"/><circle cx="58" cy="50" r="4" fill="#ffd24a"/><circle cx="42" cy="50" r="1.8" fill="#161009"/><circle cx="58" cy="50" r="1.8" fill="#161009"/></svg>'
  };
  var atlas = {};
  var ready = false;
  function loadAtlas() {
    var ids = Object.keys(SVG), pending = ids.length;
    if (!pending) { ready = true; return; }
    ids.forEach(function (id) {
      var img = new Image();
      img.onload = function () { atlas[id] = img; if (--pending === 0) { ready = true; draw(); } };
      img.onerror = function () { if (--pending === 0) { ready = true; draw(); } };
      img.src = "data:image/svg+xml;utf8," + encodeURIComponent(SVG[id]);
    });
  }

  var WALL = { "■": 1, "│": 1, "─": 1, "└": 1, "┌": 1, "┐": 1, "┘": 1, "├": 1, "┤": 1, "┬": 1, "┴": 1, "┼": 1 };

  // terrainFor returns the full-cell tile for terrain glyphs (null otherwise).
  // '+' and "'" collide with the spellbook/tool items, so gate on brown (1).
  function terrainFor(glyph, colorIdx) {
    if (glyph === "·") return "floor";
    if (glyph === "≈") return "water";
    if (WALL[glyph]) return "wall";
    if (glyph === "+" && colorIdx === 1) return "door";
    if (glyph === "'" && colorIdx === 1) return "door_open";
    if (glyph === ">") return "stairs";
    if (glyph === "^" && colorIdx === 1) return "lava"; // brown ^ = lava
    return null;
  }

  // entityFor returns the transparent sprite for the player and items (null
  // otherwise -> coloured-letter fallback). Monsters are still letters.
  var ENTITY = {
    "@": "player", "!": "i_potion", "?": "i_scroll", ")": "i_weapon", "}": "i_bow",
    "[": "i_armor", "%": "i_food", "/": "i_wand", "=": "i_ring", "~": "i_light", "+": "i_book",
    "r": "m_rat", "b": "m_bat", "k": "m_kobold", "o": "m_gnoblin", "S": "m_snake",
    "j": "m_jackal", "x": "m_slime", "z": "m_zombie", "s": "m_skeleton", "D": "m_dragon",
    "Z": "m_ghoul", "g": "m_graveling", "v": "m_vermin", "O": "m_ogre", "T": "m_troll",
    "i": "m_imp", "a": "m_spider", "w": "m_wisp", "M": "m_merman", "C": "m_scarecrow",
    "W": "m_wraith", "d": "m_direwolf", "h": "m_hellhound", "f": "m_frostling", "p": "m_goatman",
    "l": "m_tentacle", "Y": "m_toad", "q": "m_burnskull", "m": "m_mimic", "N": "m_necromancer",
    "E": "m_mummylich", "A": "m_angel",
    "K": "m_gloomlord", "e": "m_sentinel", "t": "m_technician", "G": "m_gaoler", "H": "m_horror", "L": "m_lurker"
  };
  function entityFor(glyph) { return ENTITY[glyph] || null; }

  // The autotiling glyph encodes which neighbours are walls; the open sides are
  // the rest. Shading those edges makes wall masses read as connected stone.
  var WALLOPEN = { "■": "NESW", "│": "EW", "─": "NS", "└": "SW", "┌": "NW", "┐": "NE", "┘": "SE", "├": "W", "┤": "E", "┬": "N", "┴": "S", "┼": "" };
  function drawWallEdges(px, py, glyph) {
    var open = WALLOPEN[glyph] || "";
    if (!open) return;
    var e = Math.max(2, Math.round(TILE * 0.17));
    ctx.fillStyle = "rgba(18,14,9,0.5)";
    if (open.indexOf("N") >= 0) ctx.fillRect(px, py, TILE, e);
    if (open.indexOf("S") >= 0) ctx.fillRect(px, py + TILE - e, TILE, e);
    if (open.indexOf("W") >= 0) ctx.fillRect(px, py, e, TILE);
    if (open.indexOf("E") >= 0) ctx.fillRect(px + TILE - e, py, e, TILE);
    if (open.indexOf("N") >= 0) { ctx.fillStyle = "rgba(184,170,136,0.35)"; ctx.fillRect(px, py + e, TILE, 2); }
  }

  function draw() {
    if (!tiles || !G || !ready) return;
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
    }
    ctx.fillStyle = "#000";
    ctx.fillRect(0, 0, canvas.width, canvas.height);
    ctx.textAlign = "center";
    ctx.textBaseline = "middle";
    ctx.font = Math.floor(TILE * 0.82) + "px ui-monospace, monospace";
    for (var vy = 0; vy < vh; vy++) {
      for (var vx = 0; vx < vw; vx++) {
        var gx = ox + vx, gy = oy + vy, i = gy * w + gx;
        var tg = g[i];                                      // top (composited) glyph
        if (tg === " " || tg === undefined) continue;       // unseen
        var bgl = (bg && bg[i] !== undefined) ? bg[i] : tg; // terrain beneath
        var px = vx * TILE, py = vy * TILE;
        var t = terrainFor(bgl, bcolor[i]);
        if (t && atlas[t]) {
          ctx.drawImage(atlas[t], px, py, TILE, TILE);
          if (t === "wall") drawWallEdges(px, py, bgl);
        } else {
          if (atlas.floor) ctx.drawImage(atlas.floor, px, py, TILE, TILE);
          else { ctx.fillStyle = "#2a251d"; ctx.fillRect(px, py, TILE, TILE); }
          if (tg === bgl) { ctx.fillStyle = PAL[bcolor[i]] || PAL[0]; ctx.fillText(bgl, px + TILE / 2, py + TILE / 2 + 1); } // un-tiled terrain (trap/altar)
        }
        if (tg !== bgl) {                                   // an entity sits on the terrain
          var e = entityFor(tg);
          if (e && atlas[e]) ctx.drawImage(atlas[e], px, py, TILE, TILE);
          else { ctx.fillStyle = PAL[color[i]] || PAL[0]; ctx.fillText(tg, px + TILE / 2, py + TILE / 2 + 1); }
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
  }

  window.tslGrid = function (w, h, top, color, base, bcolor, light, dim, cx, cy) {
    G = { w: w, h: h, g: Array.from(top), color: color, bg: Array.from(base), bcolor: bcolor, light: light, dim: dim, cx: cx, cy: cy };
    draw();
  };

  function apply() {
    if (tiles) { pre.hidden = true; canvas.hidden = false; draw(); }
    else { canvas.hidden = true; pre.hidden = false; }
    if (btn) btn.textContent = tiles ? "ASCII" : "Tiles";
  }
  if (btn) btn.addEventListener("click", function () {
    tiles = !tiles;
    try { localStorage.setItem("tsl-tiles", tiles ? "1" : "0"); } catch (e) {}
    apply();
  });

  loadAtlas();
  apply();
})();
