#include <stdlib.h>
#include <stdio.h>

#include "potions.h"
#include "stuff.h"
#include "poison.h"
#include "effect.h"
#include "message.h"
#include "player.h"
#include "sleep.h"
#include "fov.h"
#include "shapeshf.h"
#include "losegame.h"
#include "game.h"
#include "actions.h"
#include "gore.h"
#include "altitude.h"
#include "ui.h"
#include "stacks.h"
#include "burdened.h"



/*
  Makes CREATURE drink ITEM.
*/
blean_t drink_potion(creature_t * creature, item_t * item)
{
  char line[100];
  item_t * temp;
  char * temp_str;

  if (creature == NULL || item == NULL || attr_current(creature, attr_p_drink))
    return false;
  
  temp = detach_item(get_item_from_stack(item));

  if ((is_player(creature) == false) &&
      can_see_creature(game->player, creature))
  {
    temp_str = get_item_name(item);
    sprintf(line, "drinks %s!", temp_str);
    free(temp_str);

    msg_one(creature, line);
  }
  
  apply_potion(creature, temp, NULL);
  unburdened(creature, temp->weight);
  del_item(temp);

  return true;
} /* drink_potion */



/*
  Applies ITEM to CREATURE. ATTACKER is the creature that threw the
  potion, if any. Return true if CREATURE was killed, otherwise false.
*/
blean_t apply_potion(creature_t * creature, item_t * item, creature_t * attacker)
{
  char line[80];
  int temp;
  char * name;
  
  if (creature == NULL || item == NULL)
    return false;

  switch (item->item_number)
  {
    case treasure_elixir:
      /* Removes all status effects, even good ones */
      effect_expire(get_effect_by_id(creature, effect_poison));
      effect_expire(get_effect_by_id(creature, effect_haste));
      effect_expire(get_effect_by_id(creature, effect_slow));
      effect_expire(get_effect_by_id(creature, effect_healing));
      effect_expire(get_effect_by_id(creature, effect_stun));
      effect_expire(get_effect_by_id(creature, effect_wound));
      effect_expire(get_effect_by_id(creature, effect_blindness));
      set_attr(creature, attr_levitate, 0);

      if (is_player(creature))
      {
	queue_msg("You feel perfectly normal.");
	identify(item);
      }

      change_altitude(creature);
      
      break;

    case treasure_p_speed:
      /* Cancels slow and speeds up creature */
      effect_expire(get_effect_by_id(creature, effect_slow));
      prolong_effect(creature, effect_haste, HASTE_LENGTH);
      
      if (is_player(creature))
      {
	queue_msg("You move faster!");
	identify(item);
      }
      else if (can_see(game->player, creature->y, creature->x))
      {
	msg_the(creature, "appears to be moving faster!");
	identify(item);
      }
      break;
  
    case treasure_p_poison:
      if (poison_creature(creature, DEFAULT_POISON_TIME) > 0)
      {
	if (is_player(creature))
	{
	  queue_msg("You feel sick!");
	  identify(item);
	}
	else if (can_see(game->player, creature->y, creature->x))
	{
	  msg_one(creature, "appears sick!");
	  identify(item);
	}
      }
      break;
  
    case treasure_p_sleep:
      if (attr_current(creature, attr_p_sleep) == 0)
      {
	tranquilize(creature, item);

	/* If the player falls asleep, it always identifies. */
	/* RFE: What if the player is blinded and hit by a throw potion of sleep? */
	if (is_player(creature))
	  identify(item);
      }
      break;
  
    case treasure_p_blindness:
      if (is_blinded(creature) == false)
      {
	if (is_player(creature))
	{
	  identify(item);
	  queue_msg("You can't see!");
	}
	else if (can_see_creature(game->player, creature))
	{
	  identify(item);
	  msg_the(creature, "is blinded!");

	  creature->ai_state = ai_idle;
	  set_attr(creature, attr_perception, roll(1, 3));
	}
      }

      prolong_effect(creature, effect_blindness, DEFAULT_BLIND_TIME);
      build_fov(creature);
      
      if (is_player(creature))
	draw_level();

      break;
  
    case treasure_p_polymorph:
      if (is_player(creature))
	identify(item);
      else if (can_see_creature(game->player, creature))
	identify(item);
      
      shapeshift_random(creature, item, 0);
      break;
  
    case treasure_p_slowing:
      /* Cancels haste */
      effect_expire(get_effect_by_id(creature, effect_haste));

      /* Slows creature down */
      prolong_effect(creature, effect_slow, SLOW_LENGTH);
      
      if (is_player(creature))
      {
	queue_msg("You feel very sluggish.");
	identify(item);
      }
      else if (can_see(game->player, creature->y, creature->x))
      {
	msg_one(creature, "appears to be moving slower!");
	identify(item);
      }
      break;
  
    case treasure_p_healing:
      prolong_effect(creature, effect_healing, POTION_OF_HEALING);
      
      if (is_player(creature))
      {
	queue_msg("A warm feeling spreads throughout your body.");
	identify(item);
      }
      else if (can_see(game->player, creature->y, creature->x))
      {
	msg_one(creature, "appears healthier!");
	identify(item);
      }
      break;

    case treasure_p_levitation:
      if (attr_current(creature, attr_levitate) == 0)
      {
	if (is_player(creature) ||
	    can_see_creature(game->player, creature))
	{
	  identify(item);
	}
      }

      levitate(creature);
      break;

    case treasure_p_instant_healing:
      temp = heal(creature, 10 + tslrnd() % 15);
      
      /*
	!oIH will only become known if they heal anything and it was
	either the player that used it or a visible NPC.
      */
      if (temp)
      {
	if (is_player(creature))
	{
	  queue_msg("You feel better!");
	  identify(item);
	}
	else if (can_see(game->player, creature->y, creature->x))
	{
	  msg_one(creature, "appears much healthier!");
	  identify(item);
	}
      }
      else
      {
	if (is_player(creature))
	  queue_msg("Nothing happens.");
      }
      break;

    case treasure_p_energy:
      temp = regain_ep(creature, POTION_OF_ENERGY);
      
      /* !oE will only become known if this one restored anything. */
      if (is_player(creature))
      {
	queue_msg("You feel energized!");
	identify(item);
      }

      break;

    case treasure_p_yuck:
      if (is_player(creature))
      {
	switch (roll(1, 4))
	{
	  case 1:
	    queue_msg("This tastes like ratman urine.");
	    break;

	  case 2:
	  case 3:
	    queue_msg("This tastes like gnoblin blood.");
	    break;

	  case 4:
	    queue_msg("This tastes like liquified maggots.");
	    break;
	}
	
	identify(item);

	if (creature->id == monster_gnoblin)
	  queue_msg("Yum!");
	else
	  queue_msg("You feel slightly nauseous.");
      }
      break;

    case treasure_p_pain:
      /* Damages creature */
      if (is_player(creature))
      {
	queue_msg("It burns!");
	identify(item);
      }
      else if (can_see(game->player, creature->y, creature->x))
      {
	msg_one(creature, "looks hurt!");
	identify(item);
      }
      
      damage(creature, roll(2, 4));
      
      if (is_player(creature))
      {
	/* Did we get killed? */
	name = get_item_name(item);
	sprintf(line, "were killed by %s", name);
	free(name);
	
	if (killed(creature))
	{
	  queue_msg("You die...");

	  /*
	    Delete the potion, since it's "in the air" and we're not
	    coming back.
	  */
	  del_item(item);
	
	  check_for_player_death(line);
	}
      }
      
      if (killed(creature))
      {
	/* We do this before we kill the creature so any items the
	   creature drops will be properly identified. */
	if (can_see(game->player, creature->y, creature->x))
	  identify(item);

	creature_death(creature, attacker, item);
      }
      return true;
      
    default:
      break;
  }

  return false;
} /* apply_potion */
