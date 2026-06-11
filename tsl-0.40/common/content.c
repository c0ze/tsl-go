#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

#include "main.h"
#include "stuff.h"
#include "content.h"
#include "treasure.h"
#include "traps.h"
#include "monster.h"
#include "actions.h"
#include "game.h"
#include "area.h"
#include "find.h"



/*
  Adds default content to LEVEL. This is what most levels should
  contain. The content added will be based on the world metadata for
  the given level.

  You can override this by repointing add_content for LEVELs world to
  another function of your choice.
*/
void add_content(level_t * level)
{
  unsigned int t;
  
  if (level == NULL)
    return;
  
  /* Add capsules */
  t = 1 + roll(1, 2);
  while (t--)
    add_encounter(level, 0, 0, encounter_capsule);
  
  /* Add medkits */
  t = roll(2, 4);
  while (t--)
    add_encounter(level, 0, 0, encounter_medkit);
  
  /* Add general supply items */
  for (t = 0; t < level->supplies; t++)
    add_encounter(level, 0, 0, encounter_supply);
  
  /* Add random treasure */
  t = intrnd(5, 15);
  while (t--)
    add_encounter(level, 0, 0, encounter_treasure);
  
  /* Add a number of random monsters */
  for (t = 0; t < level->monsters; t++)
    add_encounter(level, 0, 0, encounter_std_enemy);
  
  if (maybe())
    add_encounter(level, 0, 0, encounter_inscription);
  
  /* Add some other random things */
  add_random_encounters(level);
    
  return;
} /* add_content */



/*
  Adds an encounter of type ENCOUNTER at {Y, X} on LEVEL.  Y and X
  will only be treated as a *suggested* location for the encounter;
  some encounter types might impose additional conditions on the
  location and might pick an entirely different place. For this
  reason, Y and X aren't defined as const.
*/
void add_encounter(level_t * level,
		   int y, int x,
		   encounter_t encounter)
{
  creature_t * creature;
  trap_t * trap;
  int i;
/*  unsigned int new_y;
    unsigned int new_x;*/
  int number;
  int temp;
  item_t * item;
  area_t * area;
  area_t * temp_area;

  if (on_map(level, y, x) == false)
    return;

  level->challenge++;

  if (y == 0 && x == 0)
    find_spot(level, &y, &x, find_unoccupied);

  switch (encounter)
  {
    case encounter_frostling:      put_or_destroy(level, build_monster(monster_frostling), y, x);        break;
    case encounter_flame_spirit:   put_or_destroy(level, build_monster(monster_flame_spirit), y, x);     break;
    case encounter_slime:          put_or_destroy(level, build_monster(monster_slime), y, x);            break;
    case encounter_gnoblin:        put_or_destroy(level, build_monster(monster_gnoblin), y, x);          break;
    case encounter_drowned_one:    put_or_destroy(level, build_monster(monster_drowned_one), y, x);      break;
    case encounter_electric_snake: put_or_destroy(level, build_monster(monster_electric_snake), y, x);   break;
    case encounter_goatman:        put_or_destroy(level, build_monster(monster_goatman), y, x);          break;
    case encounter_technician:     put_or_destroy(level, build_monster(monster_technician), y, x);       break;
    case encounter_gloom_lord:     put_or_destroy(level, build_monster(monster_gloom_lord), y, x);       break;
    case encounter_ghoul:          put_or_destroy(level, build_monster(monster_ghoul), y, x);            break;
    case encounter_crypt_vermin:   put_or_destroy(level, build_monster(monster_crypt_vermin), y, x);     break;
    case encounter_chainsaw_ogre:  put_or_destroy(level, build_monster(monster_chainsaw_ogre), y, x);    break;
    case encounter_burning_skull:  put_or_destroy(level, build_monster(monster_burning_skull), y, x);    break;
    case encounter_severed_hand:   put_or_destroy(level, build_monster(monster_severed_hand), y, x);     break;
    case encounter_mimic:          put_or_destroy(level, build_monster(monster_mimic), y, x);            break;
    case encounter_sludge_dweller: put_or_destroy(level, build_monster(monster_sludge_dweller), y, x);   break;
    case encounter_toad:           put_or_destroy(level, build_monster(monster_giant_slimy_toad), y, x); break;
    case encounter_hellhound:      put_or_destroy(level, build_monster(monster_hellhound), y, x);        break;
    case encounter_merman:         put_or_destroy(level, build_monster(monster_merman), y, x);           break;
    case encounter_ratman:         put_or_destroy(level, build_monster(monster_ratman), y, x);           break;
    case encounter_graveling:      put_or_destroy(level, build_monster(monster_graveling), y, x);        break;
    case encounter_scarecrow:      put_or_destroy(level, build_monster(monster_scarecrow), y, x);        break;

    case encounter_std_enemy:
      number = tslrnd() % 100;
      
      if (number < 38)
	temp = level->std_enemy[0];
      else if (number < 63)
	temp = level->std_enemy[1];
      else if (number < 81)
	temp = level->std_enemy[2];
      else if (number < 92)
	temp = level->std_enemy[3];
      else if (number < 98)
	temp = level->std_enemy[4];
      else
	temp = level->std_enemy[5];

      if (temp != monster_undefined)
      {
	put_or_destroy(level, build_monster(temp), y, x);
      }
      break;

    case encounter_ybznek:
      break;

    case encounter_gaoler:
      put_or_destroy(level, build_unique(unique_gaoler), y, x);
      break;

    case encounter_lognac:
      break;

    case encounter_caeltzan:
      break;

    case encounter_lurker:
      /* Reset the lurkers pool */
      set_tile(level, y, x, tile_water);
      
      creature = build_unique(unique_lurker);
      attach_creature(level, creature);
      set_creature_coordinates(creature, y, x);
      
      for (i = 0; i < 8; i++)
      {
	creature = build_monster(monster_tentacle);
	attach_creature(level, creature);
	find_nearest_free_spot(creature, y, x);
      }
      break;

    case encounter_teleporter:
    case encounter_win:

      if (encounter == encounter_teleporter)
	trap = alloc_trap(trap_teleporter);
      else if (encounter == encounter_win)
	trap = alloc_trap(trap_win);

      if (attach_trap(level, trap) == trap)
      {
	del_trap(trap);
	trap = NULL;
      }
      else
      {
	place_trap(trap, y, x);
	set_tile(level, y, x, tile_floor);
      }
      break;

    case encounter_necromancer:
      creature = build_unique(unique_necromancer);
      attach_creature(level, creature);
      set_creature_coordinates(creature, y, x);

      number = 10;

      for (i = 0; i < number; i++)
      {
	item = build_item(treasure_corpse);
	put_item_or_destroy(level, item,
			    round(y + 4 * sin(6.28 * (i / (float)number))),
			    round(x + 4 * cos(6.28 * (i / (float)number)))) ;
      }
      break;

    case encounter_dragon:
      creature = build_unique(unique_dragon);
      attach_creature(level, creature);
      set_creature_coordinates(creature, y, x);
      break;

    case encounter_king_of_worms:
      creature = build_unique(unique_king_of_worms);
      attach_creature(level, creature);
      set_creature_coordinates(creature, y, x);
      break;

    case encounter_pentagram:
      if (find_spot(level, &y, &x, find_sweet))
      {
	set_tile(level, y, x, tile_pentagram);
      }
      break;

    case encounter_block:
      if (find_spot(level, &y, &x, find_3x3))
      {
	set_tile(level, y, x, tile_block);
      }
      break;

    case encounter_capsule:
      if (find_spot(level, &y, &x, find_sweet))
      {
	set_tile(level, y, x, tile_capsule);
      }
      break;

    case encounter_terminal:
      if (find_spot(level, &y, &x, find_sweet))
      {
	set_tile(level, y, x, tile_terminal);
      }
      break;

    case encounter_water:
    case encounter_lava:
      if (find_spot(level, &y, &x, find_3x3))
      {
	area = new_area(y, x);

	if (encounter == encounter_water)
	  area_cloud(area, level, 5 + tslrnd() % 20);
	else
	  area_cloud(area, level, 5 + tslrnd() % 9);
	
	for (temp_area = area; temp_area != NULL; temp_area = temp_area->next)
	{
	  if (encounter == encounter_water)
	    set_tile(level, temp_area->y, temp_area->x, tile_water);
	  else
	    set_tile(level, temp_area->y, temp_area->x, tile_lava);
	}
	
	del_area(area);
      }
      break;

    case encounter_booby_trap:
    case encounter_web_trap:
    case encounter_dart_trap:
    case encounter_polymorph_trap:
    case encounter_glass_trap:
    case encounter_plate_trap:
    case encounter_blink_trap:
    case encounter_flash_trap:
    case encounter_medkit:
    case encounter_inscription:

      if (encounter == encounter_booby_trap)
	trap = alloc_trap(trap_booby);
      else if (encounter == encounter_web_trap)
	trap = alloc_trap(trap_web);
      else if (encounter == encounter_dart_trap)
	trap = alloc_trap(trap_dart);
      else if (encounter == encounter_polymorph_trap)
	trap = alloc_trap(trap_polymorph);
      else if (encounter == encounter_glass_trap)
	trap = alloc_trap(trap_glass);
      else if (encounter == encounter_plate_trap)
	trap = alloc_trap(trap_plate);
      else if (encounter == encounter_blink_trap)
	trap = alloc_trap(trap_blink);
      else if (encounter == encounter_flash_trap)
	trap = alloc_trap(trap_flash);
      else if (encounter == encounter_medkit)
	trap = alloc_trap(trap_medkit);
      else if (encounter == encounter_inscription)
	trap = alloc_trap(trap_inscription);

      /* Try to attach it to the level */
      if (attach_trap(level, trap) == trap)
      {
	/* Couldn't attach; delete trap and abort. */
	del_trap(trap);
	trap = NULL;
      }
      else
      {
	find_spot(level, &y, &x, find_unoccupied);

	place_trap(trap, y, x);
      }
      break;

    case encounter_gore:
      number = roll(1, 4);

      for (i = 0; i < number; i++)
      {
	if (roll(1, 6) == 1)
	{
	  item = random_treasure(item_table_bodyparts);
	  put_item_or_destroy(level, item,
			      y - 1 + tslrnd() % 3,
			      x - 1 + tslrnd() % 3);
	}
      }
      break;

    case encounter_mushroom:
      item = build_item(treasure_r_mushroom);
      put_item_or_destroy(level, item, y, x);
      break;

    case encounter_bone_pile:
      number = roll(1, 3);
      for (i = 0; i < number; i++)
      {
	item = random_treasure(item_table_bones);
	put_item_or_destroy(level, item,
			    y - 2 + tslrnd() % 5,
			    x - 2 + tslrnd() % 5);
      }
      break;

    case encounter_treasure:
      item = item_for_level(level);
      put_item_or_destroy(level, item, y, x);
      break;

    case encounter_supply:
      item = random_treasure(item_table_supply);
      put_item_or_destroy(level, item, y, x);
      break;

    default:
      break;
  }
  
  return;
} /* add_encounter */



/*
  Adds encounter from LEVELs encounter table to LEVEL until the
  desired challenge level of the level is reached.
*/
void add_random_encounters(level_t * level)
{
  int y;
  int x;

  if (level == NULL)
    return;

  level->challenge = 0;
  
  /* Only do this for levels that have an encounter table defined. */
  if (level->encounter != NULL)
  {
    while (level->challenge < level->wanted_challenge)
    {
      find_spot(level, &y, &x, find_unoccupied);
      add_encounter(level, y, x, pick_random_encounter(level));
    }
  }
    
  return;
} /* add_random_encounters */
