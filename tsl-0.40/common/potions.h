/*
  potions.h
*/

#ifndef _POTIONS_H_
#define _POTIONS_H_

#include "monster.h"
#include "main.h"

blean_t drink_potion(creature_t * creature, item_t * item);
blean_t apply_potion(creature_t * creature, item_t * item, creature_t * attacker);

#endif
