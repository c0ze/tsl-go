/*
  elements.h
*/

#ifndef _ELEMENTS_H_
#define _ELEMENTS_H_

#include "main.h"
#include "actions.h"
#include "item.h"
#include "level.h"
#include "creature.h"

blean_t expose_item_to_fire(item_t * item, blean_t force_burn);
blean_t expose_tile_to_fire(level_t * level,
			    const unsigned int y,
			    const unsigned int x);
blean_t expose_tile_to_cold(level_t * level,
			    const unsigned int y,
			    const unsigned int x);
blean_t expose_creature_to_fire(creature_t * creature);
blean_t lava_bath(creature_t * creature);
blean_t forcefield(creature_t * creature);
blean_t hazard(creature_t * creature);

#endif
