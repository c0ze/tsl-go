/*
  gent.h

  This is an abstraction layer intended to separate map data from
  their on-screen representation. We can translate these to ASCII
  symbols (see glyph.*) or use them as index for bitmaps (sdlui).

  This file (there is no gent.c) provides declarations so the game
  logic should not need to negotiate with the UI exactly how a feature
  is to be visualised.

  This also enables some neat tricks like setting a fake graphical
  entity for mimics.
*/

#ifndef _GENT_H_
#define _GENT_H_

/* "Graphical entity" */
enum gent_t
{
  /* System */
  gent_undefined, /* ? */
  gent_blank, /* Just black */
  gent_obstacle, /* Something we can feel but not see */

  /* Tiles */
  gent_floor = 16,
  
  gent_wall_v = 32,
  gent_wall_h,
  gent_wall_es,
  gent_wall_sw,
  gent_wall_ne,
  gent_wall_nw,
  gent_wall_nes,
  gent_wall_nsw,
  gent_wall_new,
  gent_wall_esw,
  gent_wall_cross,

  gent_door_closed = 48,
  gent_door_open,
  gent_water,
  gent_block,
  gent_ice,
  gent_lava,
  gent_stairs,
  gent_pentagram,
  gent_capsule,
  gent_terminal,
  gent_forcefield,
  gent_generator,
  gent_internal_trap,
  gent_medkit,

  /* Traps */
  gent_trap_generic = 64,
  gent_dart_trap,
  gent_booby_trap,
  gent_blink_trap,
  gent_polymorph_trap,
  gent_glass_trap,
  gent_plate_trap,
  gent_flash_trap,
  gent_web,

  /* Creatures */
  gent_player = 96,
  gent_ghoul,
  gent_chrome_angel,
  gent_burning_skull,
  gent_crypt_vermin,
  gent_severed_hand_m, /* distinct from severed hand, the item */
  gent_graveling,
  gent_gnoblin,
  gent_hellhound,
  gent_flame_spirit,
  gent_floating_brain,
  gent_wolf,
  gent_frostling,
  gent_imp,
  gent_giant_slimy_toad,
  gent_chainsaw_ogre,
  gent_f_d_g,
  gent_sentinel,
  gent_elder_mummylich,
  gent_nameless_horror,
  gent_scarecrow,
  gent_drowned_one,
  gent_merman,
  gent_slime,
  gent_goatman,
  gent_gloom_lord,
  gent_ratman,
  gent_tentacle,
  gent_technician,
  gent_mimic,
  gent_electric_snake,
  gent_sludge_dweller,
  gent_phantasm,

  /* Uniques */
  gent_lurker = 144,
  gent_dragon,
  gent_gaoler,
  gent_king_of_worms,
  gent_necromancer,
  gent_caeltzan,
  gent_ybznek,
  gent_lognac,

  /* Items */
  gent_sword = 176,
  gent_spear,
  gent_axe,
  gent_club,
  gent_staff,
  gent_whip,

  gent_shotgun = 192,
  gent_shell,
  gent_pistol,
  gent_bullet,
  gent_bow,
  gent_arrow,
  gent_crossbow,
  gent_dart,
  gent_blowgun,
  gent_grenade,

/*  gent_rifle,*/

  /*gent_bolt,*/

/*  gent_dagger,*/
/*  gent_treasure,*/
/*  gent_club,
  gent_crowbar,
  gent_steel_pipe,*/

  /*gent_rags = 152,*/
  gent_light_armor = 208,
  gent_heavy_armor,
  gent_boots,
  gent_cloak,
  gent_robe,
  gent_goggles,
  gent_crown,
  gent_gas_mask,
  gent_amulet,

  gent_potion = 224,
  gent_scroll,
  gent_book,
  gent_wand,
  gent_dataprobe,
  gent_torch,
  gent_lantern,
  gent_fire_extinguisher,
  gent_mushroom,
  gent_mortar_pestle,
  gent_hacksaw,
  gent_bone,
  gent_prod,
  gent_key,

  gent_cheese = 240,
  gent_bread,
  gent_meat,
  gent_fish,
  gent_chickpeas,
  gent_falafel,
  gent_sausage,

  gent_decapitated_head = 256,
  gent_severed_hand,
  gent_ogre_corpse,
  gent_corpse,
  gent_carcass,

  /* Potions */
/*  gent_potion_muddy,
  gent_potion_blood,
  gent_potion_slimy,
  gent_potion_milky,
  gent_potion_clear,
  gent_potion_clotted,
  gent_potion_golden,
  gent_potion_silver,
  gent_potion_shimmering,
  gent_potion_brown,
  gent_potion_sickly,
  gent_potion_effervescent,
  gent_potion_bubbly,
  gent_potion_rusty,
  gent_potion_black,
  gent_potion_crystal_bottle,
  gent_potion_cyan,
  gent_potion_tear,
  gent_potion_crystal_flask,
  gent_potion_violet,*/

  /* Spell & missile effects */
  gent_arrow_n = 304,
  gent_arrow_ne,
  gent_arrow_e,
  gent_arrow_se,
  gent_arrow_s,
  gent_arrow_sw,
  gent_arrow_w,
  gent_arrow_nw,
  gent_spell_n,
  gent_spell_ne,
  gent_spell_e,
  gent_spell_se,
  gent_spell_s,
  gent_spell_sw,
  gent_spell_w,
  gent_spell_nw,

  gent_spell_poison = 320,
  gent_spell_fire,
  gent_spell_frost,
  gent_explosion,
  gent_flash,

  /* Shit */
  gent_eyeball,
  gent_cranium,
  gent_bone_dust,
  gent_mandrake_root,
  gent_beetle_shell,
  gent_mummy_wrapping,

  gent_max
};
typedef enum gent_t gent_t;

#endif
