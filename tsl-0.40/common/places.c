#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <ctype.h>

#include "main.h"
#include "stuff.h"
#include "places.h"
#include "creature.h"
#include "traps.h"
#include "content.h"
#include "game.h"
#include "dungeon.h"
#include "modbuild.h"



/*
  Sets up the list with definitions for all in-game locations.
*/
void init_levels(void)
{
  level_t * temp;
  unsigned int i;

  for (i = 0; i < LEVELS; i++)
  {
    game->level_list[i] = NULL;
  }

  temp = game->level_list[LEVEL_TEST] = alloc_level(LEVEL_TEST, "the Test Map");
  temp->size_y = 20;
  temp->size_x = 40;
  temp->starting_module = CHAMBER_TEST;
  temp->wanted_modules = 0;
  temp->wanted_challenge = 100;

  /*temp->link[0] = LEVEL_DUNGEON;*/

  encounter_table(temp, encounter_mimic, 1);



  temp = game->level_list[LEVEL_DUNGEON] = alloc_level(LEVEL_DUNGEON, "the Dungeon");

  temp->link[0] = LEVEL_CATACOMBS;
  temp->link[1] = LEVEL_OMINOUS_CAVE;

  temp->size_y = 30;
  temp->size_x = 50;
  temp->starting_module = CHAMBER_START;
  temp->wanted_modules = MODULE_ROOM | MODULE_NARROW | MODULE_CELLS;

  temp->water = tslrnd() % 2;

  temp->std_enemy[0] = monster_gnoblin;
  temp->std_enemy[1] = monster_ratman;
  temp->std_enemy[2] = monster_graveling;
  temp->std_enemy[3] = monster_electric_snake;
  temp->std_enemy[4] = monster_electric_snake;
  temp->std_enemy[5] = monster_sludge_dweller;
  temp->monsters = intrnd(12, 16);

  temp->supplies = 3 + tslrnd() % 4;

  temp->wanted_challenge = 5;

  encounter_table(temp, encounter_mimic, 1);
  encounter_table(temp, encounter_terminal, 10);
  encounter_table(temp, encounter_gaoler, 1);
  encounter_table(temp, encounter_glass_trap, 10);
  encounter_table(temp, encounter_flash_trap, 12);
  encounter_table(temp, encounter_blink_trap, 2);
  encounter_table(temp, encounter_dart_trap, 2);
  encounter_table(temp, encounter_polymorph_trap, 1);
  encounter_table(temp, encounter_web_trap, 10);



  temp = game->level_list[LEVEL_OMINOUS_CAVE] = alloc_level(LEVEL_OMINOUS_CAVE, "the Ominous Cave");

//  set_level_message(temp, "You are in a damp underground passage.");

  temp->link[0] = LEVEL_DUNGEON;
  temp->link[1] = LEVEL_COMM_HUB;
  temp->link[2] = LEVEL_UNDERPASS;

  temp->size_y = 40;
  temp->size_x = 60;
  temp->starting_module = CHAMBER_KING;
  temp->wanted_modules = MODULE_CAVE;

  temp->boss = encounter_king_of_worms;

  temp->water = 3 + tslrnd() % 3;

  temp->std_enemy[0] = monster_chainsaw_ogre;
  temp->std_enemy[1] = monster_giant_slimy_toad;
  temp->std_enemy[2] = monster_sludge_dweller;
  temp->std_enemy[3] = monster_slime;
  temp->std_enemy[4] = monster_scarecrow;
  temp->std_enemy[5] = monster_electric_snake;
  temp->monsters = intrnd(12, 20);

  temp->supplies = 5 + tslrnd() % 5;
  temp->wanted_challenge = 7;

  encounter_table(temp, encounter_mimic, 1);
  encounter_table(temp, encounter_terminal, 1);
  encounter_table(temp, encounter_mushroom, 5);
  encounter_table(temp, encounter_web_trap, 1);
  encounter_table(temp, encounter_blink_trap, 1);
  encounter_table(temp, encounter_polymorph_trap, 2);
  encounter_table(temp, encounter_dart_trap, 10);



  temp = game->level_list[LEVEL_CATACOMBS] = alloc_level(LEVEL_CATACOMBS, "the Catacombs");

  set_level_message(temp, "The stench of death lies thick here.");

  temp->link[0] = LEVEL_DUNGEON;
  temp->link[1] = LEVEL_LABORATORY;

  temp->size_y = 40;
  temp->size_x = 60;
  temp->wanted_modules = MODULE_WIDE | MODULE_NARROW | MODULE_CAVE;

  temp->starting_module = CHAMBER_NECROMANCER;
  temp->boss = encounter_necromancer;

  temp->std_enemy[0] = monster_ghoul;
  temp->std_enemy[1] = monster_graveling;
  temp->std_enemy[2] = monster_severed_hand;
  temp->std_enemy[3] = monster_burning_skull;
  temp->std_enemy[4] = monster_drowned_one;
  temp->std_enemy[5] = monster_crypt_vermin;
  temp->monsters = intrnd(7, 14);

  temp->wanted_challenge = 5;
  temp->supplies = 5 + tslrnd() % 5;

  encounter_table(temp, encounter_mimic, 1);

  encounter_table(temp, encounter_caeltzan, 1);
  encounter_table(temp, encounter_gaoler, 1);
  encounter_table(temp, encounter_lognac, 1);

  encounter_table(temp, encounter_terminal, 1);

  encounter_table(temp, encounter_web_trap, 10);
  encounter_table(temp, encounter_glass_trap, 20);
  encounter_table(temp, encounter_blink_trap, 2);
  encounter_table(temp, encounter_dart_trap, 10);
  encounter_table(temp, encounter_polymorph_trap, 1);



  temp = game->level_list[LEVEL_LABORATORY] = alloc_level(LEVEL_LABORATORY, "the Laboratory");

  temp->link[0] = LEVEL_CATACOMBS;
  temp->link[1] = LEVEL_DROWNED_CITY;

  temp->size_y = 40;
  temp->size_x = 60;
  temp->starting_module = CHAMBER_LAB;
  temp->wanted_modules = MODULE_WIDE | MODULE_WIDE_ROOM;

//  temp->boss = encounter_lurker;

//  temp->water = 5 + tslrnd() % 5;

  temp->std_enemy[0] = monster_technician;
  temp->std_enemy[1] = monster_ratman;
  temp->std_enemy[2] = monster_slime;
  temp->std_enemy[3] = monster_giant_slimy_toad;
  temp->std_enemy[4] = monster_sludge_dweller;
  temp->std_enemy[5] = monster_drowned_one;
  temp->monsters = intrnd(10, 18);

  temp->supplies = 5 + tslrnd() % 5;

  temp->wanted_challenge = 10;

  encounter_table(temp, encounter_mimic, 1);
  encounter_table(temp, encounter_terminal, 5);
  encounter_table(temp, encounter_blink_trap, 1);
  encounter_table(temp, encounter_polymorph_trap, 1);
  encounter_table(temp, encounter_dart_trap, 1);



  temp = game->level_list[LEVEL_COMM_HUB] = alloc_level(LEVEL_COMM_HUB, "the Communications Hub");

  temp->link[0] = LEVEL_OMINOUS_CAVE;
  temp->link[1] = LEVEL_FROZEN_VAULT;

  temp->size_y = 40;
  temp->size_x = 60;
  temp->starting_module = CHAMBER_LAB;
  temp->wanted_modules = MODULE_WIDE | MODULE_WIDE_F | MODULE_WIDE_ROOM;

//  temp->boss = encounter_lurker;

//  temp->water = 5 + tslrnd() % 5;

  temp->std_enemy[0] = monster_technician;
  temp->std_enemy[1] = monster_ratman;
  temp->std_enemy[2] = monster_slime;
  temp->std_enemy[3] = monster_giant_slimy_toad;
  temp->std_enemy[4] = monster_sludge_dweller;
  temp->std_enemy[5] = monster_drowned_one;
  temp->monsters = intrnd(10, 18);

  temp->supplies = 5 + tslrnd() % 5;

  temp->wanted_challenge = 10;

  encounter_table(temp, encounter_mimic, 1);

  encounter_table(temp, encounter_terminal, 5);

  encounter_table(temp, encounter_blink_trap, 1);
  encounter_table(temp, encounter_polymorph_trap, 1);
  encounter_table(temp, encounter_dart_trap, 1);



  temp = game->level_list[LEVEL_UNDERPASS] = alloc_level(LEVEL_UNDERPASS, "the Underpass");

  set_level_message(temp, "You are in a damp underground passage.");

  temp->link[0] = LEVEL_OMINOUS_CAVE;
  temp->link[1] = LEVEL_DROWNED_CITY;

  temp->size_y = 40;
  temp->size_x = 60;
  temp->starting_module = CHAMBER_KING;
  temp->wanted_modules = MODULE_CAVE;

  temp->boss = encounter_king_of_worms;

  temp->water = 3 + tslrnd() % 3;

  temp->std_enemy[0] = monster_chainsaw_ogre;
  temp->std_enemy[1] = monster_giant_slimy_toad;
  temp->std_enemy[2] = monster_slime;
  temp->std_enemy[3] = monster_scarecrow;
  temp->std_enemy[4] = monster_sludge_dweller;
  temp->std_enemy[5] = monster_electric_snake;
  temp->monsters = 6 + tslrnd() % 6;

  temp->supplies = 5 + tslrnd() % 5;
  temp->wanted_challenge = 7;

  encounter_table(temp, encounter_mimic, 1);

  encounter_table(temp, encounter_terminal, 1);

  encounter_table(temp, encounter_mushroom, 5);
  encounter_table(temp, encounter_web_trap, 1);

  encounter_table(temp, encounter_blink_trap, 1);
  encounter_table(temp, encounter_polymorph_trap, 2);
  encounter_table(temp, encounter_dart_trap, 10);



  temp = game->level_list[LEVEL_DROWNED_CITY] = alloc_level(LEVEL_DROWNED_CITY, "the Drowned City");

  temp->link[0] = LEVEL_LABORATORY;
  temp->link[1] = LEVEL_DRAGONS_LAIR;
  temp->link[2] = LEVEL_UNDERPASS;

  temp->size_y = 40;
  temp->size_x = 60;
  temp->starting_module = CHAMBER_LURKER;
  temp->wanted_modules = MODULE_NARROW | MODULE_ROOM | MODULE_CITY;

  temp->boss = encounter_lurker;

  temp->water = 5 + tslrnd() % 5;

  temp->std_enemy[0] = monster_merman;
  temp->std_enemy[1] = monster_sludge_dweller;
  temp->std_enemy[2] = monster_slime;
  temp->std_enemy[3] = monster_giant_slimy_toad;
  temp->std_enemy[4] = monster_ratman;
  temp->std_enemy[5] = monster_drowned_one;
  temp->monsters = 10 + tslrnd() % 8;

  temp->supplies = 5 + tslrnd() % 5;
  temp->wanted_challenge = 10;

  encounter_table(temp, encounter_mimic, 1);

  encounter_table(temp, encounter_terminal, 1);

  encounter_table(temp, encounter_blink_trap, 1);
  encounter_table(temp, encounter_polymorph_trap, 1);
  encounter_table(temp, encounter_dart_trap, 1);



  temp = game->level_list[LEVEL_FROZEN_VAULT] = alloc_level(LEVEL_FROZEN_VAULT, "the Vault of the Frozen Saint");

  set_level_message(temp, "It is very cold here.");

  temp->link[0] = LEVEL_COMM_HUB;
  temp->link[1] = LEVEL_DRAGONS_LAIR;

  temp->size_y = 40;
  temp->size_x = 60;
  temp->starting_module = CHAMBER_VAULT;
  temp->wanted_modules = MODULE_WIDE;

  temp->std_enemy[0] = monster_frostling;
  temp->std_enemy[1] = monster_floating_brain;
  temp->std_enemy[2] = monster_sentinel;
  temp->std_enemy[3] = monster_technician;
  temp->std_enemy[4] = monster_frostling;
  temp->std_enemy[5] = monster_frostling;
  temp->monsters = 10 + tslrnd() % 5;

  temp->supplies = 2 + tslrnd() % 3;
  temp->wanted_challenge = 10;

  encounter_table(temp, encounter_terminal, 1);
  encounter_table(temp, encounter_plate_trap, 10);
  encounter_table(temp, encounter_blink_trap, 1);
  encounter_table(temp, encounter_polymorph_trap, 1);
  encounter_table(temp, encounter_dart_trap, 5);



  temp = game->level_list[LEVEL_DRAGONS_LAIR] = alloc_level(LEVEL_DRAGONS_LAIR, "the Dragons Lair");

  set_level_message(temp, "It is very hot here.");

  /* This needs to be first so it becomes the stair behind the Dragon */
  temp->link[0] = LEVEL_CHAPEL;
  temp->link[1] = LEVEL_DROWNED_CITY;
  temp->link[2] = LEVEL_FROZEN_VAULT;

  temp->size_y = 40;
  temp->size_x = 60;
  temp->wanted_modules = MODULE_CAVE;

  temp->starting_module = CHAMBER_DRAGON;
  temp->boss = encounter_dragon;

  temp->lava = 10 + tslrnd() % 10;
  temp->supplies = 5 + tslrnd() % 5;

  temp->std_enemy[0] = monster_hellhound;
  temp->std_enemy[1] = monster_flame_spirit;
  temp->std_enemy[2] = monster_burning_skull;
  temp->std_enemy[3] = monster_hellhound;
  temp->std_enemy[4] = monster_hellhound;
  temp->std_enemy[5] = monster_hellhound;
  temp->monsters = 10 + tslrnd() % 8;

  temp->wanted_challenge = 5;
  encounter_table(temp, encounter_mimic, 1);



  temp = game->level_list[LEVEL_CHAPEL] = alloc_level(LEVEL_CHAPEL, "the Chapel of Fallen Stars");

  temp->link[0] = LEVEL_DRAGONS_LAIR;

  temp->size_y = 19;
  temp->size_x = 160;

  temp->starting_module = CHAMBER_CHAPEL;
  temp->boss = encounter_win;
  temp->wanted_modules = MODULE_CHAPEL;

  temp->supplies = 10;

  temp->std_enemy[0] = monster_gloom_lord;
  temp->std_enemy[1] = monster_chrome_angel;
  temp->std_enemy[2] = monster_floating_demon_genitalia;
  temp->std_enemy[3] = monster_elder_mummylich;
  temp->std_enemy[4] = monster_nameless_horror;
  temp->std_enemy[5] = monster_chrome_angel;
  temp->monsters = 15 + tslrnd() % 10;

  temp->wanted_challenge = 20;
  encounter_table(temp, encounter_mimic, 1);
  encounter_table(temp, encounter_terminal, 1);

  encounter_table(temp, encounter_web_trap, 2);
  encounter_table(temp, encounter_plate_trap, 10);
  encounter_table(temp, encounter_polymorph_trap, 10);
  encounter_table(temp, encounter_dart_trap, 10);



  for (i = 0; i < LEVELS; i++)
  {
    if (game->level_list[i] != NULL)
    {
      game->level_list[i]->floor_type = tslrnd() % FLOOR_TYPES;
    }
  }

  return;
} /* init_levels */


/*
  Adds SLOTS slots of NEW_ENCOUNTER to LEVELs encounter table. SLOTS
  must be non-zero. Note that SLOTS isn't a 0-100%
  probability. Example: 5 slots of one encounter + 15 slots of another
  gives 20 possibilities, and 25%/75% chance for each.
 */
void encounter_table(level_t * level,
		     const encounter_t new_encounter,
		     const unsigned int slots)
{
  unsigned int start_index;
  unsigned int i;

  if (level == NULL)
    return;

  if (slots == 0)
    return;

  start_index = level->encounters;
  level->encounters += slots;

  level->encounter = realloc(level->encounter,
			     level->encounters * sizeof(encounter_t));

  if (level->encounter == NULL)
    out_of_memory();

  for (i = start_index; i < start_index + slots; i++)
    level->encounter[i] = new_encounter;

  return;
} /* encounter_table */



/*
  Returns a random encounter from LEVELs encounter table.
*/
encounter_t pick_random_encounter(level_t * level)
{
  if ((level == NULL) ||
      (level->encounters == 0) ||
      (level->encounter == NULL))
  {
    return encounter_undefined;
  }

  return level->encounter[tslrnd() % MAX(1, level->encounters)];
} /* pick_random_encounter */
