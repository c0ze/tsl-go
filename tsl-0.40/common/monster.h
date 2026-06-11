/*
  monster.h
*/

#include "main.h"

#ifndef _MONSTER_H_
#define _MONSTER_H_

#include "main.h"
#include "creature.h"
#include "level.h"

creature_t * build_monster(const monster_t monster);
blean_t put_or_destroy(level_t * level, creature_t * creature,
		       const unsigned int y, const unsigned int x);

#endif
