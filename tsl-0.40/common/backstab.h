/*
  backstab.h
*/

#ifndef _BACKSTAB_H_
#define _BACKSTAB_H_

#include "creature.h"

blean_t backstab(creature_t * attacker, creature_t * defender);

unsigned int get_backstab_weapon(creature_t * creature);

#endif
