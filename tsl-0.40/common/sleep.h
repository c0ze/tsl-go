/*
  sleep.h
*/

#ifndef _SLEEP_H_
#define _SLEEP_H_

#include "creature.h"

void creature_sleep(creature_t * creature, const blean_t tired);
void tranquilize(creature_t * creature, item_t * source);

#endif
