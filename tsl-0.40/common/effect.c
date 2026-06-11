#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "main.h"
#include "stuff.h"
#include "effect.h"
#include "rules.h"
#include "message.h"



/*
  Allocates an effect_t structure.
*/
effect_t * alloc_effect()
{
  unsigned int i;

  effect_t * local;

  local = malloc(sizeof(effect_t));
  if (local == NULL) out_of_memory();
  mem_alloc.effects++;

  local->next_effect = NULL;
/*  local->expire = NULL;*/
  local->affecting = NULL;
  local->countdown = true;
  local->ttl = 0;

  for (i = 0; i < ATTRIBUTES; i++)
  {
    local->attr_mod[i] = 0;
  }
  
  return local;
} /* alloc_effect */



/*
  Frees EFFECT. Make sure to rearrange the nodes before and after
  EFFECT, or any effects it is linking to will disappear.
*/
void del_effect(effect_t * effect)
{
  if (effect == NULL)
    return;

/*  if (effect->expire != NULL)
  {
    mem_alloc.chars -= strlen(effect->expire) + 1;
    free(effect->expire);
    }*/

  free(effect);
  mem_alloc.effects--;

  return;
} /* del_effect */



/*
  Adds EFFECT to CREATUREs list of effects.
 */
void add_effect(creature_t * creature, effect_t * effect)
{
  if ((creature == NULL) || (effect == NULL))
    return;

  effect->next_effect = creature->first_effect;
  creature->first_effect = effect;
  effect->affecting = creature;

  return;
} /* add_effect */



/*
  Removes EFFECT from any creature it is attached to.
*/
void effect_expire(effect_t * effect)
{
  effect_t * temp;
  creature_t * creature;

  if ((effect == NULL) || (effect->affecting == NULL))
    return;

  creature = effect->affecting;

  if (effect == creature->first_effect)
  {
    creature->first_effect = effect->next_effect;
    effect->affecting = NULL;
    del_effect(effect);
    return;
  }

  for (temp = creature->first_effect; temp != NULL; temp = temp->next_effect)
  {
    if (temp->next_effect == effect)
    {
      temp->next_effect = effect->next_effect;
      effect->affecting = NULL;
      del_effect(effect);
      return;
    }
  }

  return;
} /* effect_expire */



/*
  Builds a "standard effect". If sufficient memory couldn't be
  allocated, out_of_memory() will be called and the game will exit.
 */
effect_t * std_effect(effect_id_t id)
{
  effect_t * local;

  local = alloc_effect();
  if (local == NULL) out_of_memory();

  local->id = id;
    
  switch (id)
  {
    case effect_slow:
      local->attr_mod[attr_speed] = -1;
      break;

    case effect_rage:
      break;

    case effect_haste:
      local->attr_mod[attr_speed] = HASTE_AMOUNT;
      break;

    case effect_healing:
      local->attr_mod[attr_recovery] = +20;
      break;

/*    case effect_light:
      local->attr_mod[attr_light_radius] = +5;
      break;*/

    case effect_poison:
      local->attr_mod[attr_poisoned] = 1;
      break;

    case effect_stun:
      local->attr_mod[attr_p_move] = 1;
      break;

    case effect_web:
      local->attr_mod[attr_p_move] = 1;
      break;

    case effect_wound:
      local->attr_mod[attr_wounded] = 1;
      break;

    case effect_hide:
      local->attr_mod[attr_stealth] = +5;
      break;

    case effect_blindness:
      local->attr_mod[attr_blindness] = 1;
      break;

    case effect_illiteracy:
      local->attr_mod[attr_p_read] = 1;
      break;

    case effect_sleep:
      /*
	Make pass_time_on_effects() ignore this one. We handle it
	manually in play().
      */
      local->countdown = false;
      break;

    case effect_confusion:
      /* Do nothing */
      break;

    case effect_temp_weapon:
      break;

    case effect_light:
      local->attr_mod[attr_vision] = 3;
      break;

    default:
      break;
  }

  return local;
} /* std_effect */



/*
  Returns any effect affecting CREATURE that is of type ID.
*/
effect_t * get_effect_by_id(const creature_t * creature, const effect_id_t id)
{
  effect_t * temp;

  if (creature == NULL)
    return NULL;

  for (temp = creature->first_effect; temp != NULL; temp = temp->next_effect)
  {
    if (temp->id == id)
      return temp;
  }

  return NULL;
} /* get_effect_by_id */



/*
  If CREATURE has an effect of type ID, it adds TIME to its TTL. If no
  such effect is already present, a new is added with the specified
  time as TTL.
*/
void prolong_effect(creature_t * creature, const effect_id_t id, const unsigned int time)
{
  effect_t * temp;

  if (creature == NULL)
    return;

  temp = get_effect_by_id(creature, id);

  if (temp == NULL)
  {
    temp = std_effect(id);
    add_effect(creature, temp);
  }

  temp->ttl += time;

  return;
} /* prolong_effect */



void clear_level_effects(creature_t * creature)
{
  effect_t * temp;

  if (creature == NULL)
    return;

  while ((temp = get_effect_by_id(creature, effect_disable)))
  {
    effect_expire(temp);
  }

  return;
} /* clear_level_effects */
