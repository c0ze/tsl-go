/*
  poison.h

  Poison resistance slows the effects of poison. It has a peak value
  (POISON_IMMUNITY in rules.h) where it turns into resistance and
  completely prevents a creature from being poisoned.

  See also: increment_counters in game.h
*/

#ifndef _POISON_H_
#define _POISON_H_

#include "poison.h"

unsigned int poison_creature(creature_t * creature, const unsigned int amount);
void poison_damage(creature_t * creature);
blean_t poison_immunity(creature_t * creature);
blean_t cure_poison(creature_t * caster, item_t * source, signed int param);

#endif
