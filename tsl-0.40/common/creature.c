#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

#include "main.h"
#include "stuff.h"
#include "creature.h"
#include "fov.h"
#include "level.h"
#include "item.h"
#include "effect.h"
#include "ai.h"
#include "treasure.h"
#include "traps.h"
#include "unique.h"
#include "actions.h"
#include "game.h"
#include "player.h"
#include "altitude.h"
#include "vweapon.h"
#include "inventory.h"
#include "wounds.h"
#include "memory.h"
#include "find.h"



/*
  Allocates a new creature_t structure. The new structure will have
  all variables set to neutral values (such that they will not cause
  the game to crash if used). If insufficient memory is available,
  out_of_memory() will be called and the program will exit.
*/
creature_t * alloc_creature()
{
  creature_t * local;
  attr_index_t i;

  /* Register that we have allocated memory for this creature. */
  local = malloc(sizeof(creature_t));
  if (local == NULL) out_of_memory();
  mem_alloc.creatures++;

  local->uid = get_uid();

  set_creature_name(local, "(a) TempName", "(the) TempName", "(only) TempName");

  /* */
  local->gent = gent_undefined;

  local->death_msg = NULL;
  set_death_msg(local, "is killed!");

  /*
    These will be set to neutral values until we decide what this
    little creature should be when it grows up.
  */
  local->id = monster_undefined;
  local->corpse = treasure_undefined;
  local->parts = 0;

  local->ai_state = ai_idle;

  /* We haven't seen it, it hasn't seen us. */
  local->detected = false;

  local->humanoid = false;

  local->multiaction = MULTIACTION_NONE;

  /* Defaults */
  local->altitude = altitude_walking;
  local->retreats = true;
  local->pacifist = false;
/*  local->turn_func = &basic_ai;*/
/*  local->alignment = alignment_neutral;*/
  local->location = NULL;
/*  local->magic_weapon = NULL;*/
  local->first_effect = NULL;
  local->lifetime = 0;
  local->unarmed = virtual_fists;
  local->first_item = NULL; /* Empty inventory */

  local->attack_pos = 3;
  local->next_attack = 3;

  local->move_counter = 0;
  local->fatigue_counter = 0;
  local->poison_counter = 0;
  local->ep_counter = 0;

  /* Set default attributes */
  for (i = 0; i < ATTRIBUTES; i++)
  {
    if (attr_info[i] != NULL)
      set_attr(local, i, attr_info[i]->def);
  }
  
  return local;
} /* alloc_creature */



/*
  Sets the name of CREATURE.
*/
void set_creature_name(creature_t * creature, const char * new_one, const char * new_the, const char * new_only)
{
  if (creature == NULL)
    return;

  strncpy(creature->name_one, new_one, 30);
  strncpy(creature->name_the, new_the, 30);
  strncpy(creature->name_only, new_only, 30);
  
  return;
} /* set_creature_name */




/*
  Frees CREATURE. This also deletes any items the creature is carrying
  - if you want to save them, make sure to do so before calling this.
*/
void del_creature(creature_t * creature)
{
  if (creature == NULL)
    return;

  detach_creature(creature);

  set_death_msg(creature, NULL);

  while (creature->first_item != NULL)
    del_item(detach_item(creature->first_item));

  while (creature->first_effect != NULL)
    effect_expire(creature->first_effect);
  
  free(creature);
  mem_alloc.creatures--;
  
  return;
} /* del_creature */



/*
  Inserts CREATURE on LEVEL. Returns NULL on success; on error, the
  program will exit ("isn't that too harsh?" - no, it means we've run
  out of memory and the rest of the game won't be functioning properly
  either).
*/
creature_t * attach_creature(level_t * level, creature_t * creature)
{
  int i;

  if ((level == NULL) || (creature == NULL))
    return creature;
  
  /*
    Loop through the array of creatures, adding the new creature at
    the first free index, if any is found.
  */
  for (i = 0; i < level->creatures; i++)
  {
    if (level->creature[i] == NULL)
    {
      /* We have found an empty slot in the creature array. */
      
      /* Detach the creature from its current location. */
      detach_creature(creature);
      
      /* Insert it on this level instead. */
      level->creature[i] = creature;
      
      creature->location = level;
      
      return NULL;
    }
  }

  /* If we got here the creature array is full; we need a larger one... */
  
  /* Reserve space for one more creature */
  level->creatures++;
  level->creature = realloc(level->creature,
			    sizeof(creature_t *) * (level->creatures));
  if (level->creature == NULL) out_of_memory(); /* Verify we got a new array */
  
  /* Detach the creature from its current location. */
  detach_creature(creature);

  /*
    Place the creature in the new slot - this has to be at the end
    since all the others were full.
  */
  level->creature[level->creatures - 1] = creature;
  creature->location = level;

  return NULL;
} /* attach_creature */



/*
  Removes CREATURE from the level it's currently located on. If the
  creature isn't on any level, nothing happens. CREATURE is always
  returned.
*/
creature_t * detach_creature(creature_t * creature)
{
  int i;

  if (creature == NULL)
    return creature;

  if (creature->location == NULL)
    return creature;

  /* Find the creature in the creature list. */
  for (i = 0; i < creature->location->creatures; i++)
  {
    if (creature->location->creature[i] == creature)
    {
      /* Found it! */

      /* Mark the slot in the creature array as empty */
      creature->location->creature[i] = NULL;

      /* Mark the creature as belonging to "nowhere" */
      creature->location = NULL;
      
      return creature;
    }
  }
  
  return creature;
} /* detach_creature */



/*
  Attempts to move CREATURE MOVE_Y steps south and MOVE_X steps
  east. If MOVE_Y or MOVE_X is negative, it will instead move the
  creature west or north. If the new coordinates are blocked, false is
  returned, otherwise true is returned and CREATURE is moved. CREATURE
  will move onto water and lava tiles if it can do so safely or if
  FORCED is true. If it steps on a trap the trap will be triggered.
*/
blean_t move_creature(creature_t * creature,
		      const signed int move_y,
		      const signed int move_x,
		      const blean_t forced)
{
  signed int new_y;
  signed int new_x;
  level_t * level;
  blean_t allowed;
  tile_t new_tile;

  if (creature == NULL)
    return false;

  if (attr_current(creature, attr_p_move))
    return false;

  level = creature->location;

  if (level == NULL)
    return false;
  
  new_y = creature->y + move_y;
  new_x = creature->x + move_x;

  new_tile = get_tile(level, new_y, new_x);

  allowed = false;

  /* Is the new location occupied? */
  if (on_map(level, new_y, new_x) &&
      is_traversable(level, false, new_y, new_x) &&
      (is_door(level, new_y, new_x) == false || is_blinded(creature) == false) &&
      find_creature(level, new_y, new_x) == NULL)
  {
    if (new_tile == tile_water && attr_current(creature, attr_free_swim))
    {
      allowed = true;
    }
    else if (attr_base(creature, attr_permaswim))
    {
      /* We'd rather not get out of the water, so we won't proceed with the other ifs. */

      if (new_tile == tile_water)
	allowed = true;
    }
    else if (is_walkable(level, false, new_y, new_x))
    {
      allowed = true;
    }
    else if ((is_floating(creature) || is_blinded(creature) || forced) &&
	     (new_tile == tile_water || new_tile == tile_lava))
    {
      allowed = true;
    }
    else if ((is_blinded(creature) || forced) && new_tile == tile_forcefield)
    {
      allowed = true;
    }
  } /* noone there */

  /* We've exhausted all safe possibilities to move to that tile. */
  if (allowed == false)
    return false;

  set_creature_coordinates(creature, new_y, new_x);
  
  change_altitude(creature);

/*  if (level->map[new_y][new_x] != tile_water &&
      is_floating(creature) == false)
  {
    creature->altitude = altitude_walking;
    }*/
  
  /* Deal damage from open wounds. */
  wound_damage(creature);

  activate_trap(find_trap(level, new_y, new_x));

  return true;
} /* move_creature */



/*
  Sets the location of CREATURE to {Y, X}. No check is performed
  whether the specified coordinates are traversable, this is left to
  the caller. However, CREATURE can not be moved off the edge of the
  map. The FOV of CREATURE will be rebuilt.
*/
void set_creature_coordinates(creature_t * creature,
			      const unsigned int y, const unsigned int x)
{
  level_t * level;

  if (creature == NULL)
    return;

  level = creature->location;

  if (on_map(level, y, x) == false)
    return;
  
  creature->y = y;
  creature->x = x;

  build_fov(creature);

  /* RFE: This doesn't really belong in creature. */
  if (is_player(creature))
  {
    update_level_memory(creature->location);
  }

  return;
} /* set_creature_coordinates */



/*
  Moves CREATURE to a random walkable spot on the map it's attached to.
*/
void find_random_free_spot(creature_t * creature)
{
  int y;
  int x;

  if ((creature == NULL) || (creature->location == NULL))
    return;

  find_spot(creature->location, &y, &x, find_unoccupied);
  set_creature_coordinates(creature, y, x);

  return;
} /* find_random_free_spot */



/*
  Removes at most AMOUNT points of fatigue from CREATURE.
  
  Returns: the number of hit points restored.
*/
unsigned int heal(creature_t * creature, const unsigned int amount)
{
  signed int old_health;

  if (creature == NULL)
    return 0;

/*
  signed int new_fatigue;
  old_fatigue = attr_base(creature, attr_fatigue);
  new_fatigue = MAX(0, old_fatigue - amount);
  
*/
 /* old_fatigue - new_fatigue;*/

  old_health = attr_base(creature, attr_health);
  
  set_attr(creature, attr_health, old_health + amount);

  return amount;
} /* heal */



/*
  Restores AMOUNT of CREATUREs energy points.
*/
unsigned int regain_ep(creature_t * creature, const unsigned int amount)
{
  unsigned int new_ep;
  signed int old_ep;

  if (creature == NULL)
    return 0;

  old_ep = attr_base(creature, attr_ep_current);

  new_ep = old_ep + amount;
  set_attr(creature, attr_ep_current, new_ep);
  
  return (attr_base(creature, attr_ep_current) - old_ep);
} /* regain_ep */



/*
  Removes AMOUNT from CREATUREs Health.  
  Returns: the amount of fatigue added.
*/
signed int damage(creature_t * creature,
		  const unsigned int amount)
{
  attribute_t old_health;
  attribute_t new_health;
/*  unsigned int res;
    signed int change;*/
  
  if (creature == NULL)
    return 0;

/*  unsigned int limit;
    unsigned int new_fatigue;
  limit = attr_current(creature, attr_fatigue_limit);
*/

  old_health = attr_base(creature, attr_health);
  new_health = old_health - amount;
  set_attr(creature, attr_health, new_health);

  if (new_health <= 0)
    set_attr(creature, attr_killed, 1);

  return amount;
} /* damage */



/*
  Kills CREATURE. This will leave a corpse item (if applicable; some
  creatures don't leave corpses - see corpse in creature_t) and any
  items the creature was carrying in a pile. If BLEED is true, blood
  might be sprayed on the adjacent squares (again, if applicable -
  some creatures don't bleed).
*/
void kill_creature(creature_t * creature, const blean_t bleed)
{
  struct item_t * temp;

  if (creature == NULL)
    return;

  /*
    Leave a corpse at the mobs feet. Some creatures don't leave corpses.
  */

  temp = NULL;

  temp = build_item(creature->corpse);

  if (temp != NULL)
  {
    /*
      Set coordinates, try to attach the corpse, apply floor
      effect. If it doesn't attach, delete it.
    */
    place_item(temp, creature->y, creature->x);

    if (attach_item_to_level(creature->location, temp))
      del_item(temp);
    else
      floor_effect(temp);
  }
  
  /*
    Drop all items on the floor. If the creature isn't on a level,
    drop_item() will just delete the items instead.
  */
  while (creature->first_item != NULL)
  {
    drop_item(creature, creature->first_item, true);
  }

  /* Finally, remove the dead creature from the map and delete it. */
  detach_creature(creature);
  del_creature(creature);

  return;
} /* kill_creature */



/*
  Returns a pointer to a clone of CREATURE. The clone will be (mostly)
  a deep copy; string pointers will be duplicated, but it will have no
  effects, magic weapon, items or location.
*/
creature_t * clone_creature(const creature_t * creature)
{
  creature_t * clone;

  if (creature == NULL)
    return NULL;

  clone = alloc_creature();

  /* We will replace this so remove whatever is in there. */
  set_death_msg(clone, NULL);

  memcpy(clone, creature, sizeof(creature_t));

  clone->first_item = NULL;
  clone->location = NULL;
  clone->first_effect = NULL;
/*  clone->magic_weapon = NULL;*/

  clone->unarmed = creature->unarmed;

  /*
    Clones are permanent. If anything requires a temporary clone, set
    it manually.
  */
  clone->lifetime = 0;

  /*
    Reset these so that all clones don't attack at exactly the same
    time, etc.
  */
  clone->move_counter = 0;
  clone->fatigue_counter = 0;
  clone->poison_counter = 0;
  clone->ep_counter = 0;

/*  set_creature_name(clone, creature->name);*/

  clone->death_msg = NULL;
  set_death_msg(clone, creature->death_msg);

  return clone;
} /* clone_creature */



/*
  Removes AMOUNT of CREATUREs EP.
*/
void spend_ep(creature_t * creature, const unsigned int amount)
{
  if (creature == NULL)
    return;
  
  set_attr(creature, attr_ep_current,
	   attr_base(creature, attr_ep_current) - amount);
  
  return;
} /* spend_ep */



/*
  Moves CREATURE to the first available free spot on or near
  {DESIRED_Y, DESIRED_X}.
*/
void find_nearest_free_spot(creature_t * creature,
			    const unsigned int desired_y,
			    const unsigned int desired_x)
{
  level_t * level;
  unsigned int radius;
  signed int y;
  signed int x;
  unsigned int coord[25][2];
  unsigned int found;

  if (creature == NULL)
    return;

  level = creature->location;

  found = 0;

  /*
    Scan a rectangle around the desired coordinates. First pass will
    only include one tile, second and third will be 3x3 and 5x5. Save
    all valid coordinates found - if any, pick one at random.
  */
  for (radius = 0; radius <= 2; radius++)
  {
    for (y = desired_y - radius; y <= desired_y + radius; y++)
    {
      for (x = desired_x - radius; x <= desired_x + radius; x++)
      {
	if (on_map(level, y, x) == false)
	  continue;

	if ((is_swimming(creature) && get_tile(level, y, x) == tile_water && find_creature(level, y, x) == NULL) ||
	    (is_swimming(creature) == false && is_walkable(level, false, y, x)))
	{
	  coord[found][0] = y;
	  coord[found][1] = x;
	  found++;
 	}
      }
    }

    if (found > 0)
    {
      found = tslrnd() % found;
      set_creature_coordinates(creature, coord[found][0], coord[found][1]);
      return;
    }
  } /* radius */

  /* Give up, just put in anywhere. */
  find_random_free_spot(creature);

  return;
} /* find_nearest_free_spot */



/*
  Sets the temporary weapon of CREATURE to virtual weapon index
  VWEAPON. It will last TTL turns. Pass -1 for index to clear.
*/
void set_temp_weapon(creature_t * creature,
		     const signed int vweapon,
		     const unsigned int ttl)
{
  effect_t * temp_weapon;

  if (creature == NULL)
    return;

  effect_expire(get_effect_by_id(creature, effect_temp_weapon));

  if (vweapon >= 0)
  {
    temp_weapon = std_effect(effect_temp_weapon);
    temp_weapon->param[EFFECT_VWEAPON_INDEX] = vweapon;
    temp_weapon->ttl = ttl;

    add_effect(creature, temp_weapon);
  }

  return;
} /* set_temp_weapon */



/*
  Sets the death message for CREATURE to S. If the creature already
  has a death message set, it will be freed. If S is null, any
  previous message will be freed but no new will be set.
*/
void set_death_msg(creature_t * creature, const char * s)
{
  if (creature == NULL)
    return;

  if (creature->death_msg != NULL)
  {
    mem_alloc.chars -= strlen(creature->death_msg) + 1;
    free(creature->death_msg);
  }

  if (s != NULL)
  {
    creature->death_msg = mydup(s);
    mem_alloc.chars += strlen(creature->death_msg) + 1;
  }
  else
    creature->death_msg = NULL;
  
  return;
} /* set_death_msg */



/*
  Returns the virtual weapon CREATURE uses when nothing else is equipped.
*/
item_t * get_unarmed_weapon(const creature_t * creature)
{
  if (creature == NULL)
    return NULL;

  return virtual_weapon[creature->unarmed];
} /* get_unarmed_weapon */



/*
  Returns true if CREATURE has been killed.
*/
blean_t killed(const creature_t * creature)
{
  if (creature == NULL)
    return false;

  if (attr_base(creature, attr_killed))
    return true;
  else
    return false;
} /* killed */
