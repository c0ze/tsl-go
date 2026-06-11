/*
  content.h - This file contains functions for adding content
  (treasure, NPCs, traps, etc) to levels.
*/

#ifndef _CONTENT_H_
#define _CONTENT_H_

enum encounter_t
{
  encounter_undefined,

  encounter_std_enemy,

  encounter_inscription,

  encounter_water,
  encounter_lava,

  encounter_win,
  encounter_teleporter,

  encounter_block,

  encounter_medkit,

  encounter_web_trap,
  encounter_booby_trap,
  encounter_snake_trap,
  encounter_blink_trap,
  encounter_dart_trap,
  encounter_glass_trap,
  encounter_plate_trap,
  encounter_flash_trap,
  encounter_polymorph_trap,

  encounter_pentagram,
  encounter_mushroom,

  encounter_burning_skull,
  encounter_severed_hand,
  encounter_slime,
  
  encounter_sludge_dweller,
  encounter_ratman,
  encounter_graveling,
  encounter_gnoblin,
  encounter_drowned_one,
  encounter_merman,
  encounter_toad,
  encounter_hellhound,
  encounter_electric_snake,
  encounter_technician,

  encounter_scarecrow,
  encounter_frostling,
  encounter_chainsaw_ogre,
  encounter_flame_spirit,

  encounter_crypt_vermin,
  encounter_ghoul,
  encounter_chrome_angel,

  encounter_caeltzan,
  encounter_lognac,
  encounter_gaoler,
  encounter_ybznek,

  /* Bosses */
  encounter_king_of_worms,
  encounter_necromancer,
  encounter_lurker,
  encounter_dragon,

  encounter_capsule,
  encounter_terminal,

  encounter_treasure,
  encounter_supply,

  encounter_mimic,
  encounter_wraith,
  encounter_bone_pile,
  encounter_gore,
  encounter_goatman,
  encounter_gloom_lord,
  encounter_dusk_raider,
  encounter_black_knight,
  encounter_
};
typedef enum encounter_t encounter_t;

#include "main.h"
#include "rules.h"
#include "level.h"

void add_content(level_t * level);
void add_encounter(level_t * level,
		   int y, int x,
		   encounter_t encounter);
void add_random_encounters(level_t * level);

#endif
