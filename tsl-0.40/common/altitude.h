/*
  altitude.h

  Floating is synonymous to levitating, not floating in water.
*/

#ifndef _ALTITUDE_H_
#define _ALTITUDE_H_

#include "level.h"
#include "creature.h"

blean_t change_altitude(creature_t * creature);
blean_t is_floating(const creature_t * creature);
blean_t is_walking(const creature_t * creature);
blean_t is_swimming(const creature_t * creature);

#endif
