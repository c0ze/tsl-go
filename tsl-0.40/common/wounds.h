/*
  wounds.h
*/

#ifndef _WOUNDS_H_
#define _WOUNDS_H_

#include "creature.h"

unsigned int wound_creature(creature_t * creature, const unsigned int amount);
blean_t wound_damage(creature_t * creature);

#endif
