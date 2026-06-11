/*
  ai.h - NPC behaviour, targetting, etc.
*/

#ifndef _AI_H_
#define _AI_H_

#include "main.h"
#include "creature.h"

void basic_ai(creature_t * creature);

blean_t wanna_panic(creature_t * creature);
blean_t stumble(creature_t * creature);

blean_t target_player(const creature_t * creature,
		      signed int * move_y, signed int * move_x,
		      const unsigned int range);

void find_target(creature_t * creature);

dir_t target_best_direction(const creature_t * creature, unsigned int range);

dir_t cone_direction(const creature_t * creature,
		     unsigned int range,
		     const blean_t ray);

dir_t find_path(level_t * level,
		const unsigned int start_y,
		const unsigned int start_x,
		const unsigned int end_y,
		const unsigned int end_x,
		unsigned int limit,
		const blean_t ignore_creatures,
		const blean_t traverse_water);

blean_t pursue(creature_t * stalker,
	       const unsigned int target_y,
	       const unsigned int target_x);

blean_t retreat(creature_t * creature,
		const unsigned int flee_y,
		const unsigned int flee_x);

blean_t offensive(creature_t * creature);

item_t * get_throw_item(creature_t * creature);

blean_t try_to_heal_self(creature_t * creature);
blean_t try_to_teleport(creature_t * creature);
blean_t use_beneficial_item(creature_t * creature);

item_t * get_specific_item(creature_t * creature, const unsigned int number);

#endif
