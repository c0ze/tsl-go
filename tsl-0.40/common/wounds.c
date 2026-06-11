#include <stdlib.h>
#include <stdio.h>

#include "stuff.h"
#include "wounds.h"
#include "effect.h"
#include "message.h"
#include "game.h"
#include "losegame.h"
#include "ui.h"
#include "fov.h"
#include "player.h"



/*
  Increases CREATUREs "wounded" time by AMOUNT.

  Returns how many turns of bleeding were added.
*/
unsigned int wound_creature(creature_t * creature, const unsigned int amount)
{
  if ((creature == NULL) || (amount == 0))
    return 0;

  if (attr_current(creature, attr_wound_immunity) == 0)
  {
    prolong_effect(creature, effect_wound, amount);
    return amount;
  }
  else
  {
    return 0;
  }
} /* wound_creature */



/*
  Deals damage to CREATURE if it is wounded. This is intended after a
  creature has moved. Returns true if the creature was killed.
*/
blean_t wound_damage(creature_t * creature)
{
  if (creature == NULL)
    return false;

  if (attr_current(creature, attr_wounded) == 0)
    return false;

  damage(creature, 1);
  add_anim(anim_type_damage, creature->uid, 1, 0, 0);
    
  if (is_player(creature))
    queue_msg("You are bleeding!");

  if (killed(creature))
  {
    if (is_player(creature))
    {
      draw_level(); /* Since we just moved, we want to update the may before we die. */
      queue_msg("You have lost too much blood.");
      queue_msg("You die...");
      msgflush_wait();
      check_for_player_death("bled to death");
    }
    else
    {
      if (can_see_creature(game->player, creature))
	msg_one(creature, "collapses!");
      
      kill_creature(creature, false);
    }

    return true;
  }
  
  return false;
} /* wound_damage */
