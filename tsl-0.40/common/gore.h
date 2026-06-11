/*
  gore.h

  Everything messy.
*/

#ifndef _GORE_H_
#define _GORE_H_

#include "creature.h"

void creature_death(creature_t * vic,
		    creature_t * killer,
		    item_t * weapon);

#endif
