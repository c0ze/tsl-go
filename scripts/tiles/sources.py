"""Where every web tile sprite comes from.

Values are paths under assets/tiles/ (a single image, or a list of variants
the renderer picks between per cell), or "anim:<name>" for an Aseprite
animation in assets/tiles/anim/<name>.aseprite.

  dcss/  CC0 tiles from Dungeon Crawl Stone Soup (see assets/tiles/CREDITS.md),
         copied in by import_dcss.py (DCSS below maps them to the pack).
  gen/   sprites generated for this project, normalised by normalize.py.
  anim/  Aseprite sources for the animated tiles.

Colour-keyed entries follow internal/ui's palette order:
0 normal, 1 brown, 2 blue, 3 red, 4 green, 5 cyan, 6 magenta, 7 black.
"""

# dcss/<name>.png  <-  path inside the DCSS supplemental pack
# (name: (pack path, optional recolour hue shift in degrees))
DCSS = {
    # --- walls and floors by level theme
    **{f"wall_brick_{i}": (f"dungeon/wall/catacombs_{i}.png", None) for i in range(10)},
    **{f"floor_pebble_{i}": (f"dungeon/floor/pebble_brown_{i}.png", None) for i in range(9)},
    **{f"wall_bone_{i}": (f"dungeon/wall/undead_brown_{i}.png", None) for i in range(4)},
    **{f"floor_tomb_{i}": (f"dungeon/floor/tomb_{i}.png", None) for i in range(4)},
    **{f"wall_cave_{i}": (f"dungeon/wall/orc_{i}.png", None) for i in (1, 2)},
    **{f"floor_moss_{i}": (f"dungeon/floor/lair_{i}.png", None) for i in range(4)},
    **{f"wall_lab_{i}": (f"dungeon/wall/lab-metal_{i}.png", None) for i in range(4)},
    **{f"floor_lab_{i}": (f"dungeon/floor/rect_gray_{i}.png", None) for i in range(4)},
    **{f"wall_hub_{i}": (f"dungeon/wall/metal_wall_white_{i}.png", None) for i in range(3)},
    **{f"floor_mesh_{i}": (f"dungeon/floor/mesh_{i}.png", None) for i in range(4)},
    **{f"wall_rock_{i}": (f"dungeon/wall/lab-rock_{i}.png", None) for i in range(4)},
    **{f"floor_dirt_{i}": (f"dungeon/floor/grey_dirt_{i}.png", None) for i in range(4)},
    **{f"wall_shoal_{i}": (f"dungeon/wall/shoals_wall_{i}.png", None) for i in range(1, 5)},
    **{f"floor_mosaic_{i}": (f"dungeon/floor/mosaic_{i}.png", None) for i in range(4)},
    **{f"wall_ice_{i}": (f"dungeon/wall/zot_blue_{i}.png", None) for i in range(4)},
    **{f"floor_ice_{i}": (f"dungeon/floor/ice_{i}.png", None) for i in range(4)},
    **{f"wall_hell_{i}": (f"dungeon/wall/hell_{i}.png", None) for i in range(1, 5)},
    **{f"floor_infernal_{i}": (f"dungeon/floor/infernal_{i}.png", None) for i in range(1, 10)},
    **{f"wall_church_{i}": (f"dungeon/wall/church_{i}.png", None) for i in range(5)},
    **{f"floor_marble_{i}": (f"dungeon/floor/white_marble_{i}.png", None) for i in range(4)},
    # --- features
    "altar": ("dungeon/altars/shining_one.png", None),
    "trap_dart": ("dungeon/traps/zotdef_dart_trap.png", None),
    "trap_plate": ("dungeon/traps/pressure_plate.png", None),
    "trap_poly": ("dungeon/traps/teleport_permanent.png", None),
    "base_water": ("dungeon/water/shoals_deep_water_0.png", None),
    # --- items
    **{f"potion_{n}": (f"item/potion/{n}.png", None) for n in
       ("white", "brown", "brilliant_blue", "ruby", "murky", "cyan", "magenta", "black")},
    **{f"scroll_{n}": (f"item/scroll/scroll-{n}.png", None) for n in
       ("grey", "brown", "blue", "red", "green", "cyan", "purple")},
    **{f"book_{n}": (f"item/book/{n}.png", None) for n in
       ("light_gray", "light_brown", "dark_blue", "red", "dark_green", "cyan", "magenta", "dark_gray")},
    **{f"wand_{n}": (f"item/wand/gem_{n}.png", None) for n in
       ("silver", "wood", "lead", "copper", "bronze", "glass", "ivory", "iron")},
    "ring_ruby": ("item/ring/ruby.png", None),
    "ring_tourmaline": ("item/ring/tourmaline.png", None),
    "amulet": ("item/amulet/artefact/urand_four_winds.png", None),
    "weapon_dagger": ("item/weapon/dagger.png", None),
    "weapon_staff": ("item/weapon/quarterstaff.png", None),
    "weapon_crystal": ("item/weapon/greatsword_3_new.png", 180),
    "weapon_doom": ("item/weapon/long_sword_7.png", None),
    "bow": ("item/weapon/ranged/shortbow_1.png", None),
    "armor_leather": ("item/armor/torso/animal_skin_1.png", None),
    "armor_chain": ("item/armor/torso/ring_mail_1.png", None),
    "armor_scale": ("item/armor/torso/scale_mail_1.png", None),
    "armor_rune": ("item/armor/torso/robe_ego_1.png", None),
    "helmet": ("item/armor/headgear/helmet_1.png", None),
    "hat": ("item/armor/headgear/hat_1.png", None),
    "food_ration": ("item/food/meat_ration.png", None),
    # --- monsters (DCSS matches; the rest are generated)
    "m_gnoblin": ("monster/goblin.png", None),
    "m_merman": ("monster/merfolk_impaler.png", None),
    "m_bat": ("monster/animals/bat.png", None),
    "m_kobold": ("monster/kobold.png", None),
    "m_skeleton": ("monster/undead/skeletons/skeleton_humanoid_small.png", None),
    "m_wraith": ("monster/undead/shadow.png", None),
    "m_ogre": ("monster/ogre.png", None),
    "m_jackal": ("monster/animals/jackal.png", None),
    "m_electric_snake": ("monster/animals/sea_snake.png", None),
    "m_cave_snake": ("monster/aquatic/lava_snake.png", 110),
    "m_spider": ("monster/animals/spider.png", None),
    "m_troll": ("monster/deep_troll_berserker.png", None),
    "m_hellhound": ("monster/animals/hell_hound.png", None),
    "m_gloom_lord": ("monster/demons/shadow_fiend.png", None),
    "m_sentinel": ("monster/vault/vault_sentinel.png", None),
    "m_chrome_angel": ("monster/holy/angel.png", None),
    "m_horror": ("monster/aberration/unseen_horror.png", None),
    "m_mummylich": ("monster/undead/ancient_lich.png", None),
    "m_dragon": ("monster/dragons/golden_dragon.png", -40),
    "m_necromancer": ("monster/necromancer.png", None),
    "m_slime": ("monster/amorphous/azure_jelly.png", -100),
    "m_brain": ("monster/animals/brain_worm.png", None),
    "m_toad": ("monster/animals/blink_frog.png", None),
}

# Generated sprites: gen/<name>.png (agy, normalised to 32x32).
GEN = ["ratman", "ghoul", "graveling", "crypt_vermin", "scarecrow", "imp", "dire_wolf",
       "wisp", "frostling", "goatman", "tentacle", "technician", "burning_skull", "gaoler",
       "lurker", "sludge_dweller", "severed_hand", "flame_spirit", "mimic", "king_of_worms",
       "chainsaw_ogre", "zombie", "corpse", "mushroom", "torch", "key", "arrows", "cloak",
       "boots", "door_closed", "door_open", "stairs_down", "web_trap", "flash_trap", "player"]

# How normalize.py treats each generated sprite: terrain is a full-bleed
# tile, items are small centred icons, everything else a standing figure.
GEN_TILES = ["door_closed", "door_open", "stairs_down", "web_trap", "flash_trap"]
GEN_ITEMS = ["corpse", "mushroom", "torch", "key", "arrows", "cloak", "boots"]

# Level id -> (wall family, floor family): every dcss/<family>_*.png is a
# variant the renderer spreads across that level's cells.
THEMES = {
    "dungeon": ("wall_brick", "floor_pebble"),
    "catacombs": ("wall_bone", "floor_tomb"),
    "ominous_cave": ("wall_cave", "floor_moss"),
    "laboratory": ("wall_lab", "floor_lab"),
    "comm_hub": ("wall_hub", "floor_mesh"),
    "underpass": ("wall_rock", "floor_dirt"),
    "drowned_city": ("wall_shoal", "floor_mosaic"),
    "frozen_vault": ("wall_ice", "floor_ice"),
    "dragons_lair": ("wall_hell", "floor_infernal"),
    "chapel": ("wall_church", "floor_marble"),
}
