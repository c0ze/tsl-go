/*
  memory.h - level memory, automap. This was previously known as "seen".
*/

#ifndef _MEMORY_H_
#define _MEMORY_H_

#include "main.h"
#include "level.h"
#include "creature.h"
#include <stdint.h>

void update_level_memory(level_t * level);

blean_t memory_should_connect(const level_t * level,
			      const unsigned int y,
			      const unsigned int x);

blean_t believe_should_connect(const level_t * level,
			       const unsigned int y,
			       const unsigned int x);

/*blean_t memory_is_traversable(const level_t * level,
			      const unsigned int y,
			      const unsigned int x);*/

/*unsigned int count_memory_traversables(const level_t * level,
				       const unsigned int y,
				       const unsigned int x);*/

#endif
