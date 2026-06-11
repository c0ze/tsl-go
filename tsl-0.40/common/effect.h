/*
  effect.h
  
  Effects are temporary states that affect a single creature. Their
  primary purpose is as attribute modifiers (magical effects, etc) but
  they could be used to keep track of anything that requires a
  time-limited binary state.
*/

#ifndef _EFFECT_H_
#define _EFFECT_H_

#include "main.h"
#include "creature.h"



enum effect_id_t
{
  effect_undefined,
  effect_haste,
  effect_slow,
  effect_healing,
  effect_web,
  effect_rage,
  effect_light,
  effect_stun,
  effect_wound,
  effect_blindness,
  effect_sleep,
  effect_confusion,
  effect_poison,
  effect_hide,
  effect_temp_weapon,
  effect_illiteracy,
  effect_disable /* see ffield.h */
};
typedef enum effect_id_t effect_id_t;


#define EFFECT_PARAMS 4
#define EFFECT_Y 0
#define EFFECT_X 1

#define EFFECT_VWEAPON_INDEX 0



struct effect_t
{
  /*
    Should be one of effect_id_t. This is so we know what kind of
    effect this is; for example, slowing will cancel haste.
  */
  effect_id_t id;

  /* How many ticks this effect will remain in play. */
  unsigned int ttl;

  /*
    true if this effect should count down each turn (and eventually
    expire) in the main loop, false if we want to do this manually
    somewhere else.
  */
  blean_t countdown;

  /*
    How much it modifies each attribute. Note that it is perfectly
    legal to have an effect without attribute modifiers.
  */
  signed int attr_mod[ATTRIBUTES];

  /*
    Whom it affects. This is mostly so we can easily find where this
    effect belongs when we want to remove it.
  */
  creature_t * affecting;

  /* Next node in the linked list of effects for this creature. */
  effect_t * next_effect;

  /* What message to print when this effect expires. */
  /*expire_t expire;*/

  /* General-purpose parameters */
  unsigned int param[EFFECT_PARAMS];
};



effect_t * alloc_effect(void);
void del_effect(effect_t * effect);
void add_effect(creature_t * creature, effect_t * effect);
void effect_expire(effect_t * effect);
effect_t * std_effect(effect_id_t id);
effect_t * get_effect_by_id(const creature_t * creature,
			    const effect_id_t id);
void prolong_effect(creature_t * creature,
		    const effect_id_t id,
		    const unsigned int time);

void clear_level_effects(creature_t * creature);


#endif
