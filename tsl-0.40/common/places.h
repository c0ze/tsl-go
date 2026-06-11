/*
  places.h
  
  This is where we set up the level skeletons with parameters for the dungeon generator(s).
*/

#ifndef _PLACES_H_
#define _PLACES_H_

#include <stdint.h>

#include "main.h"
#include "creature.h"
#include "level.h"
#include "unique.h"
#include "content.h"
#include "gent.h"

#define LEVEL_DUNGEON          1
#define LEVEL_OMINOUS_CAVE     2
#define LEVEL_DROWNED_CITY     3
#define LEVEL_CATACOMBS        4
#define LEVEL_DRAGONS_LAIR     5
#define LEVEL_FROZEN_VAULT     6
#define LEVEL_CHAPEL           7
#define LEVEL_LABORATORY       8
#define LEVEL_COMM_HUB         9
#define LEVEL_UNDERPASS        10
#define LEVEL_TEST             14
#define LEVELS                 15

#define LEVEL_START LEVEL_DUNGEON

void init_levels(void);
encounter_t pick_random_encounter(level_t * level);
void encounter_table(level_t * level,
		     const encounter_t new_encounter,
		     const unsigned int slots);

#endif
