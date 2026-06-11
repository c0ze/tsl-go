/*
  pushing.h
*/

#ifndef _PUSHING_H_
#define _PUSHING_H_

#include "main.h"
#include "creature.h"

blean_t push_internal(creature_t * creature, dir_t dir, unsigned int dist);
blean_t push_tile(level_t * level, const unsigned int y, const unsigned int x, const dir_t dir, const item_t * source);
blean_t push(creature_t * caster, item_t * source, signed int param);
blean_t is_pushable(level_t * level, const unsigned int y, const unsigned int x);

#endif
