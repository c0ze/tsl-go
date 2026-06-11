#include <stdlib.h>

#include "sleep.h"
#include "stuff.h"
#include "effect.h"
#include "message.h"
#include "fov.h"
#include "player.h"
#include "game.h"
#include "ui.h"



/*
  Makes CREATURE fall asleep (if TIRED) or wake up (not TIRED).
*/
void creature_sleep(creature_t * creature, const blean_t tired)
{
  effect_t * eff;
  
  if (creature == NULL)
    return;

  eff = get_effect_by_id(creature, effect_sleep);

  if (attr_current(creature, attr_p_sleep))
  {
    effect_expire(eff);
    return;
  }

  /*
    A message is displayed only if the creature has been detected and
    is within sight of the player.
  */
  
  /* But I am LE TIRED */
  if (tired)
  {
    prolong_effect(creature, effect_sleep, SLEEP_DURATION);

    /* If there was no previous effect... */
    if (eff == NULL)
    {
      if (is_player(creature))
      {
	queue_msg("You fall asleep!");
	build_fov(creature);
	draw_level();
	display_stats(creature);
	msgflush_wait();
	clear_msgbar();
      }
      else if (can_see(game->player, creature->y, creature->x)
	       && creature->detected)
      {
	/*draw_level();*/
	msg_one(creature, "falls asleep.");
      }
    }
  }
  else if (eff)
  {
    effect_expire(eff);    

    if (is_player(creature))
    {
      queue_msg("You wake up!");
      build_fov(creature);
      draw_level();
      display_stats(creature);
      msgflush_wait();
      clear_msgbar();
    }
    else if (can_see(game->player, creature->y, creature->x)
	     && creature->detected)
    {
      /*draw_level();*/
      msg_one(creature, "wakes up.");
    }
  }

  return;
} /* creature_sleep */



/*
  Forces CREATURE to sleep. SOURCE will be identified if it was clear
  it was the cause.
*/
void tranquilize(creature_t * creature, item_t * source)
{
  blean_t was_awake;
  
  if (creature == NULL)
    return;

  if (attr_current(creature, attr_p_sleep))
    return;

  was_awake = is_awake(creature);
  
  creature_sleep(creature, true);
  
  /* If it was awake, but it isn't now and we could see what happened. */
  if (was_awake &&
      is_awake(creature) == false &&
      can_see_creature(game->player, creature))
  {
    identify(source);
  }
  
  return;
} /* tranquilize */
