#include <stdlib.h>

#include "swimming.h"
#include "message.h"
#include "losegame.h"
#include "actions.h"
#include "fov.h"
#include "game.h"
#include "player.h"
#include "altitude.h"



/*
  Returns true if the creature drowned.
*/
blean_t swim(creature_t * creature)
{
  level_t * level;
  unsigned int swim_fatigue;
  unsigned int swimming;

  if (creature == NULL || 
      (level = creature->location) == NULL ||
      on_map(level, creature->y, creature->x) == false)
  {
    return false;
  }

  if (attr_current(creature, attr_nonbreathing) ||
      attr_current(creature, attr_free_swim) ||
      is_floating(creature))
  {
    return false;
  }

  swim_fatigue = attr_base(creature, attr_swim_fatigue);
  swimming = attr_current(creature, attr_swimming);
  
  if (level->map[creature->y][creature->x] == tile_water)
  {
    swim_fatigue++;

    set_attr(creature, attr_swim_fatigue, swim_fatigue);

    if (swim_fatigue > swimming)
    {
      damage(creature, 1);
      /* queue_msg("Gulp!"); */
    }

    if (killed(creature))
    {
      if (is_player(creature))
      {
	queue_msg("You drown...");
	msgflush_wait();
	check_for_player_death("drowned");
      }
      else
      {
	if (can_see_creature(game->player, creature))
	  msg_one(creature, "drowns!");

	/*
	  We need to kill creatures in a special way. They cannot
	  leave any items or corpses.
	*/
	creature->corpse = treasure_undefined;

	while (creature->first_item)
	  del_item(creature->first_item);

	kill_creature(creature, false);
      }

      return true;
    }
  }
  else
  {
    set_attr(creature, attr_swim_fatigue, 0);
  }
  
  return false;
} /* swim */
