#include <stdlib.h>
#include <string.h>

#include "main.h"
#include "glyph.h"
#include "options.h"
#include "rules.h"



/*
  Sets up the list of graphical entity -> glyph mappings.
*/
void init_glyph_map()
{
  unsigned int i;

  for (i = 0; i < gent_max; i++)
  {
    glyph_map[i] = ACS_BLOCK;
    glyph_convert[i][0] = '\0';
  }
  
  /* */
  ext_glyph(gent_undefined,       '?', A_REVERSE, " ");
  set_glyph(gent_blank,           ' ', A_NORMAL);
  set_glyph(gent_obstacle,        '#', A_NORMAL);

  set_glyph(gent_medkit,          '+', A_REVERSE);

  set_glyph(gent_block,           ' ', A_REVERSE);

  for (i = 0; i < FLOOR_TYPES; i++)
  {
    if (options.dotfloors)
      ext_glyph(gent_floor + i, '.', A_NORMAL, ".");
    else
      ext_glyph(gent_floor + i, ACS_CKBOARD, A_NORMAL, ".");
  }

  set_glyph(gent_door_closed,     'X', A_REVERSE);
  set_glyph(gent_door_open,       '=', A_NORMAL);

  ext_glyph(gent_wall_v,          ACS_VLINE, A_REVERSE, "\xe2\x94\x82");
  ext_glyph(gent_wall_h,          ACS_HLINE, A_REVERSE, "\xe2\x94\x80");
  ext_glyph(gent_wall_es,         ACS_ULCORNER, A_REVERSE, "\xe2\x94\x8C");
  ext_glyph(gent_wall_sw,         ACS_URCORNER, A_REVERSE, "\xe2\x94\x90");
  ext_glyph(gent_wall_ne,         ACS_LLCORNER, A_REVERSE, "\xe2\x94\x94");
  ext_glyph(gent_wall_nw,         ACS_LRCORNER, A_REVERSE, "\xe2\x94\x98");
  ext_glyph(gent_wall_nes,        ACS_LTEE, A_REVERSE, "\xe2\x94\x9C");
  ext_glyph(gent_wall_nsw,        ACS_RTEE, A_REVERSE, "\xe2\x94\xA4");
  ext_glyph(gent_wall_new,        ACS_BTEE, A_REVERSE, "\xe2\x94\xB4");
  ext_glyph(gent_wall_esw,        ACS_TTEE, A_REVERSE, "\xe2\x94\xAC");
  ext_glyph(gent_wall_cross,      ACS_PLUS, A_REVERSE, "\xe2\x94\xBC");

  set_glyph(gent_water,           '_', A_NORMAL);
  set_glyph(gent_ice,             '#', A_NORMAL);
  set_glyph(gent_lava,            '^', A_NORMAL);
  set_glyph(gent_stairs,          '%', A_NORMAL);
  set_glyph(gent_pentagram,       '*', A_NORMAL);
  set_glyph(gent_capsule,         '0', A_REVERSE);
  set_glyph(gent_terminal,        'T', A_REVERSE);
  set_glyph(gent_generator,       '$', A_REVERSE);
  set_glyph(gent_forcefield,      '&', A_NORMAL);
  set_glyph(gent_internal_trap,   '^', A_NORMAL);

  set_glyph(gent_trap_generic,    '^', A_NORMAL);
  set_glyph(gent_dart_trap,       '^', A_NORMAL);
  set_glyph(gent_booby_trap,      '^', A_NORMAL);
  set_glyph(gent_blink_trap,      '^', A_NORMAL);
  set_glyph(gent_glass_trap,      '^', A_NORMAL);
  set_glyph(gent_plate_trap,      '^', A_NORMAL);
  set_glyph(gent_flash_trap,      '^', A_NORMAL);
  set_glyph(gent_polymorph_trap,  '^', A_NORMAL);
  set_glyph(gent_web,             '^', A_NORMAL);

//  set_glyph(gent_player,          '@', A_NORMAL);
  ext_glyph(gent_player,          '@', A_NORMAL, "@" /*'@' '\0'*/);

  set_glyph(gent_floating_brain,  'b', A_NORMAL);
  set_glyph(gent_frostling,       'f', A_NORMAL);
  set_glyph(gent_sentinel,        'e', A_NORMAL);
  set_glyph(gent_graveling,       'g', A_NORMAL);
  set_glyph(gent_hellhound,       'h', A_NORMAL);
  set_glyph(gent_imp,             'i', A_NORMAL);
  set_glyph(gent_flame_spirit,    'j', A_NORMAL);

  set_glyph(gent_mimic,           'm', A_NORMAL);
  set_glyph(gent_gnoblin,         'o', A_NORMAL);
  set_glyph(gent_severed_hand_m,  'p', A_NORMAL);
  set_glyph(gent_goatman,         'p', A_NORMAL);
  set_glyph(gent_burning_skull,   'q', A_NORMAL);
  set_glyph(gent_technician,      't', A_NORMAL);
  set_glyph(gent_ratman,          'r', A_NORMAL);
  set_glyph(gent_sludge_dweller,  's', A_NORMAL);
  set_glyph(gent_crypt_vermin,    'v', A_NORMAL);
  set_glyph(gent_tentacle,        'l', A_NORMAL);
  set_glyph(gent_slime,           'x', A_NORMAL);

  set_glyph(gent_chrome_angel,    'A', A_NORMAL);
  set_glyph(gent_scarecrow,       'C', A_NORMAL);
  set_glyph(gent_dragon,          'D', A_NORMAL);
  set_glyph(gent_elder_mummylich, 'E', A_NORMAL);
  set_glyph(gent_gloom_lord,      'K', A_NORMAL);
  set_glyph(gent_gaoler,          'G', A_NORMAL);
  set_glyph(gent_nameless_horror, 'H', A_NORMAL);
  set_glyph(gent_lurker,          'L', A_NORMAL);
  set_glyph(gent_merman,          'M', A_NORMAL);
  set_glyph(gent_necromancer,     'N', A_NORMAL);
  set_glyph(gent_chainsaw_ogre,   'O', A_NORMAL);
  set_glyph(gent_f_d_g,           'P', A_NORMAL);
  set_glyph(gent_electric_snake,  'S', A_NORMAL);
  set_glyph(gent_king_of_worms,   'W', A_NORMAL);
  set_glyph(gent_giant_slimy_toad,'Y', A_NORMAL);
  set_glyph(gent_ghoul,           'Z', A_NORMAL);
  set_glyph(gent_drowned_one,     'Z', A_NORMAL);

  set_glyph(gent_caeltzan,        '?', A_NORMAL);
  set_glyph(gent_ybznek,          '?', A_NORMAL);

/*  set_glyph(gent_potion_muddy,    '!', A_NORMAL);
  set_glyph(gent_potion_blood,    '!', A_NORMAL);
  set_glyph(gent_potion_slimy,    '!', A_NORMAL);
  set_glyph(gent_potion_milky,    '!', A_NORMAL);
  set_glyph(gent_potion_clear,    '!', A_NORMAL);
  set_glyph(gent_potion_clotted,  '!', A_NORMAL);
  set_glyph(gent_potion_golden,   '!', A_NORMAL);
  set_glyph(gent_potion_silver,   '!', A_NORMAL);
  set_glyph(gent_potion_shimmering,'!', A_NORMAL);
  set_glyph(gent_potion_brown,    '!', A_NORMAL);
  set_glyph(gent_potion_sickly,   '!', A_NORMAL);
  set_glyph(gent_potion_effervescent,'!', A_NORMAL);
  set_glyph(gent_potion_bubbly,   '!', A_NORMAL);
  set_glyph(gent_potion_rusty,    '!', A_NORMAL);
  set_glyph(gent_potion_black,    '!', A_NORMAL);
  set_glyph(gent_potion_crystal_bottle, '!', A_NORMAL);
  set_glyph(gent_potion_cyan,     '!', A_NORMAL);
  set_glyph(gent_potion_tear,     '!', A_NORMAL);
  set_glyph(gent_potion_crystal_flask, '!', A_NORMAL);
  set_glyph(gent_potion_violet,   '!', A_NORMAL);*/

  set_glyph(gent_scroll,          '?', A_NORMAL);
  set_glyph(gent_book,            '?', A_REVERSE);

  set_glyph(gent_bow,             '{', A_NORMAL);
  set_glyph(gent_crossbow,        '{', A_NORMAL);
  set_glyph(gent_blowgun,         '{', A_NORMAL);
/*  set_glyph(gent_rifle,           '{', A_NORMAL);*/
  set_glyph(gent_pistol,          '{', A_NORMAL);
  set_glyph(gent_shotgun,         '{', A_NORMAL);

  set_glyph(gent_bone,            '*', A_NORMAL);
  set_glyph(gent_cranium,         '*', A_NORMAL);
  set_glyph(gent_bone_dust,       '*', A_NORMAL);
  set_glyph(gent_mushroom,        '*', A_NORMAL);
  set_glyph(gent_mandrake_root,   '*', A_NORMAL);
  set_glyph(gent_severed_hand,    '*', A_NORMAL);
  set_glyph(gent_decapitated_head,'*', A_NORMAL);
  set_glyph(gent_eyeball,         '*', A_NORMAL);

  set_glyph(gent_ogre_corpse,     '*', A_NORMAL);
  set_glyph(gent_corpse,          '*', A_NORMAL);
  set_glyph(gent_carcass,         '*', A_NORMAL);

  set_glyph(gent_bread,           '*', A_NORMAL);
  set_glyph(gent_meat,            '*', A_NORMAL);
  set_glyph(gent_fish,            '*', A_NORMAL);
  set_glyph(gent_cheese,          '*', A_NORMAL);
  set_glyph(gent_chickpeas,       '*', A_NORMAL);
  set_glyph(gent_falafel,         '*', A_NORMAL);
  set_glyph(gent_sausage,         '*', A_NORMAL);

  set_glyph(gent_mortar_pestle,   '\'', A_NORMAL);
  set_glyph(gent_hacksaw,         '\'', A_NORMAL);
  set_glyph(gent_dataprobe,       '\'', A_NORMAL);
  set_glyph(gent_key,             '\'', A_NORMAL);
  set_glyph(gent_prod,            '\'', A_NORMAL);
  set_glyph(gent_fire_extinguisher,'\'', A_NORMAL);

  set_glyph(gent_torch,           ';', A_NORMAL);
  set_glyph(gent_lantern,         ';', A_NORMAL);
  set_glyph(gent_cloak,           '\"', A_NORMAL);
  set_glyph(gent_robe,            '\"', A_NORMAL);
  set_glyph(gent_light_armor,     '[', A_NORMAL);
  set_glyph(gent_heavy_armor,     '[', A_NORMAL);
  set_glyph(gent_mummy_wrapping,  '[', A_NORMAL);
  set_glyph(gent_boots,           '[', A_NORMAL);
  set_glyph(gent_gas_mask,        '\"', A_NORMAL);
  set_glyph(gent_amulet,          '\"', A_NORMAL);
  set_glyph(gent_wand,            '~', A_NORMAL);

  set_glyph(gent_sword,           ')', A_NORMAL);
  set_glyph(gent_axe,             ')', A_NORMAL);
  set_glyph(gent_club,            ')', A_NORMAL);
  set_glyph(gent_staff,           ')', A_NORMAL);
  set_glyph(gent_spear,           ')', A_NORMAL);
  set_glyph(gent_whip,            ')', A_NORMAL);

  set_glyph(gent_book,            '?', A_NORMAL);
  set_glyph(gent_potion,          '!', A_NORMAL);

/*  set_glyph(gent_treasure,        '*', A_NORMAL);*/

/*  set_glyph(gent_crowbar,         ')', A_NORMAL);
  set_glyph(gent_steel_pipe,      ')', A_NORMAL);
  set_glyph(gent_dagger,          ')', A_NORMAL);*/

  set_glyph(gent_crown,           '-', A_NORMAL);
  set_glyph(gent_goggles,         '-', A_NORMAL);

  set_glyph(gent_shell,           ':', A_NORMAL);
  set_glyph(gent_bullet,          ':', A_NORMAL);
  set_glyph(gent_dart,            ':', A_NORMAL);
  set_glyph(gent_arrow,           ':', A_NORMAL);
  set_glyph(gent_grenade,         ':', A_NORMAL);

  set_glyph(gent_arrow_n,         '|', A_NORMAL);
  set_glyph(gent_arrow_ne,        '/', A_NORMAL);
  set_glyph(gent_arrow_e,         '-', A_NORMAL);
  set_glyph(gent_arrow_se,        '\\', A_NORMAL);
  set_glyph(gent_arrow_s,         '|', A_NORMAL);
  set_glyph(gent_arrow_sw,        '/', A_NORMAL);
  set_glyph(gent_arrow_w,         '-', A_NORMAL);
  set_glyph(gent_arrow_nw,        '\\', A_NORMAL);

  set_glyph(gent_spell_n,         '|', A_NORMAL);
  set_glyph(gent_spell_ne,        '/', A_NORMAL);
  set_glyph(gent_spell_e,         '-', A_NORMAL);
  set_glyph(gent_spell_se,        '\\', A_NORMAL);
  set_glyph(gent_spell_s,         '|', A_NORMAL);
  set_glyph(gent_spell_sw,        '/', A_NORMAL);
  set_glyph(gent_spell_w,         '-', A_NORMAL);
  set_glyph(gent_spell_nw,        '\\', A_NORMAL);

  set_glyph(gent_explosion,       '#', A_NORMAL);
  set_glyph(gent_spell_frost,     '#', A_REVERSE);
  set_glyph(gent_spell_poison,    '#', A_REVERSE);
  set_glyph(gent_spell_fire,      '#', A_NORMAL);
  set_glyph(gent_flash,           '#', A_NORMAL);

  return;
} /* init_glyph_map */



/*
  Sets the GLYPH that should be used for displaying graphical entity GENT.
*/
void set_glyph(const unsigned int gent, const chtype glyph, const int attr)
{
  glyph_map[gent] = glyph;
  glyph_attr[gent] = attr;

  return;
} /* set_glyph */



/*
  Sets the GLYPH that should be used for displaying graphical entity GENT.
*/
void ext_glyph(const unsigned int gent, const chtype glyph, const int attr, const char * convert)
{
  glyph_map[gent] = glyph;
  glyph_attr[gent] = attr;

  strcpy(glyph_convert[gent], convert);

  return;
} /* set_glyph */



#ifdef TSL_CONSOLE
/*
  Draws the glyph of graphical entity GENT at {Y, X} in window W.

  W must be wrefresh()ed for the changes to appear on-screen;
  put_glyph() doesn't do this.
*/
void put_glyph(WINDOW * w, const unsigned int y, const unsigned int x, const unsigned int gent)
{
  put_custom(w, y, x, glyph_map[gent], glyph_attr[gent]);
  
  return;
} /* put_glyph */



/*
  Prints the character C with attributes A at {Y, X} in window W.
*/
void put_custom(WINDOW * w, const unsigned int y, const unsigned int x,
		const chtype c, const unsigned int a)
{
  wmove(w, y, x);
  wattrset(w, a);
  waddch(w, c);
  wattrset(w, 0);

  return;
} /* put_custom */
#endif
