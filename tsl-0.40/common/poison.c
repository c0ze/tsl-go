#include <stdlib.h>
#include <stdio.h>

#include "main.h"
#include "stuff.h"
#include "poison.h"
#include "creature.h"
#include "effect.h"
#include "ui.h"
#include "message.h"
#include "fov.h"
#include "losegame.h"
#include "game.h"
#include "player.h"



/*
  Increases CREATUREs poison time by AMOUNT.

  Returns how many ticks of poison were added.
*/
unsigned int poison_creature(creature_t * creature, const unsigned int amount)
{
  if ((creature == NULL) || (amount == 0))
    return 0;

  if (poison_immunity(creature) == false)
  {
    prolong_effect(creature, effect_poison, amount);
    return amount;
  }
  else
  {
    return 0;
  }
} /* poison_creature */



/*
  Deals a point of poison damage to CREATURE
*/
void poison_damage(creature_t * creature)
{
  if (creature == NULL)
    return;

  damage(creature, 1);
  add_anim(anim_type_damage, creature->uid, 1, 0, 0);
    
  if (killed(creature))
  {
    if (is_player(creature))
    {
      draw_level(); /* Since we just moved, we want to update the may before we die. */
      queue_msg("The poison has reached your heart.");
      queue_msg("You die...");
      msgflush_wait();
      check_for_player_death("succumbed to poison");
    }
    else
    {
      if (can_see_creature(game->player, creature))
	msg_one(creature, "collapses!");
      
      kill_creature(creature, false);
      return;
    }
  }
  
  return;
} /* poison_damage */



/*
  Returns TRUE if CREATURE is immune to poison, otherwise FALSE.
*/
blean_t poison_immunity(creature_t * creature)
{
  if (attr_current(creature, attr_poison_resistance) >= POISON_IMMUNITY)
    return true;
  else
    return false;
} /* poison_immunity */



/*
  Cures poison for CASTER.
*/
blean_t cure_poison(creature_t * caster, item_t * source, signed int param)
{
  effect_t * effect;

  if (caster  == NULL)
    return false;

  effect = get_effect_by_id(caster, effect_poison);

  if (effect == NULL)
  {
    if (is_player(caster))
      queue_msg("Nothing happens.");
  }
  else
  {
    if (is_player(caster))
    {
      msg_expire(effect);
      effect_expire(effect);
      identify(source);
    }
    else if (can_see_creature(game->player, caster))
    {
      /* TODO: Should be something like "doesn't look as sick anymore" */
      msg_one(caster, "looks healthier!");
    }
  }

  return true;
} /* cure_poison */
