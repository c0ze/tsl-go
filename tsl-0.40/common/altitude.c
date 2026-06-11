#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "main.h"
#include "stuff.h"
#include "altitude.h"
#include "ui.h"
#include "fov.h"
#include "elements.h"
#include "traps.h"
#include "game.h"
#include "ai.h"
#include "player.h"



/*
  Updates the altitude of CREATURE. If it isn't floating but has a
  source of levitation it will start to float. If it's floating but
  has no source of lev. it will fall to the ground. If it's falling or
  walking on a water tile, it will be set to swimming. If it lands on
  lava or a trap, these will take effect. If the player or a creature
  visible to the player changes altitude, a message will be printed.

  Returns true if creature was killed or moved away by changing
  altitude (e.g. landing in lava or on a trap), false if it is alive
  and located on the floor/in the water.
*/
blean_t change_altitude(creature_t * creature)
{
  level_t * level;
  char line[80];
  
  strcpy(line, "");

  if (creature == NULL)
    return false;

  level = creature->location;

  if (level == NULL)
    return false;

  if (on_map(level, creature->y, creature->x) == false)
    return false;

  if (attr_base(creature, attr_permafloat) > 0)
    return false;

  /* If the creature has recently begun to float */
  if (is_floating(creature) &&
      creature->altitude != altitude_floating)
  {
    creature->altitude = altitude_floating;
    
    if (is_player(creature))
      queue_msg("You soar into the air!");
    else if (can_see_creature(game->player, creature))
      msg_one(creature, "begins to float in the air!");
    
    return false;
  }

  /* If the creature has recently ceased floating */
  if (creature->altitude == altitude_floating &&
      is_floating(creature) == false)
  {
    creature->altitude = altitude_walking;

    if (level->map[creature->y][creature->x] == tile_lava)
    {
      if (is_player(creature))
	queue_msg("You land in lava!");
      else if (can_see_creature(game->player, creature))
	msg_one(creature, "lands in lava!");
      
      return lava_bath(creature);
    }
    else if (level->map[creature->y][creature->x] != tile_capsule)
    {
      /*
	If we land on the ground, we'll just put the message in
	line. If we find out later that we're landing in water, we can
	just overwrite the message.
      */
      if (is_player(creature))
	strcpy(line, "You land on the ground.");
      else if (can_see_creature(game->player, creature))
      {
	sprintf(line, "%s lands on the ground!", creature->name_one);
	upperfirst(line);
      }

      if (find_trap(level, creature->y, creature->x) != NULL)
      {
	/*
	  Traps should only appear on floor tiles, so we will display
	  the message here and return after the trap has been
	  sprung.
	*/
	queue_msg(line);
	
	/* Spring any traps we might have landed on... */
	return activate_trap(find_trap(level, creature->y, creature->x));
      }
    }
  }

  if (level->map[creature->y][creature->x] == tile_water)
  {
    if (is_walking(creature))
    {
      /* Get into the water. */
      creature->altitude = altitude_swimming;

      /* TODO: Print a message like "you hear a splash" for NPCs that
	 are out of sight? */

      draw_attention(creature->location, creature->y, creature->x, noise_faint);

      if (is_player(creature))
	strcpy(line, "You plunge into water!");
      else if (can_see_creature(game->player, creature))
      {
	sprintf(line, "%s plunges into water!", creature->name_one);
	upperfirst(line);
      }
    }
  }
  else /* it's not a water tile */
  {
    if (is_swimming(creature))
    {
      /* Get out of the water. */
      creature->altitude = altitude_walking;
    }
  }
  
  queue_msg(line);
  
  return false;
} /* change_altitude */


/*
  Returns true if CREATURE is floating.
  See also: is_swimming(), is_walking()
*/
blean_t is_floating(const creature_t * creature)
{
  if (creature == NULL)
    return false;

  if (attr_current(creature, attr_levitate) > 0 ||
      attr_base(creature, attr_permafloat) > 0)
  {
    return true;
  }

  return false;
} /* is_floating */



/*
  Returns true if CREATURE is swimming in water.
  See also: is_walking(), is_floating()
*/
blean_t is_swimming(const creature_t * creature)
{
  if (creature == NULL)
    return false;

  if (is_floating(creature))
    return false;

  if (creature->altitude == altitude_swimming)
    return true;

  return false;
} /* is_swimming */



/*
  Returns true if CREATURE is walking on the ground.
  See also: is_swimming(), is_floating()
*/
blean_t is_walking(const creature_t * creature)
{
  if (creature == NULL)
    return false;
  
  if (creature->altitude == altitude_walking)
    return true;

  return false;
} /* is_walking */
