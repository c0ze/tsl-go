/*
  missile.h - ranged combat.
*/

#ifndef _MISSILE_H_
#define _MISSILE_H_

#include "main.h"
#include "creature.h"

unsigned int get_spell_range(creature_t * caster, const attr_index_t spell);

unsigned int get_missile_damage(const creature_t * attacker,
				const item_t * weapon,
				const item_t * missile);

void fire_missile(creature_t * creature, dir_t dir,
		  item_t * weapon, item_t * missile,
		  blean_t destroy_missile, unsigned int range);

blean_t missile_hit(level_t * level, creature_t * creature,
		    item_t * weapon, item_t * missile,
		    const unsigned int y, const unsigned int x,
		    const signed int move_y, const signed int move_x,
		    const blean_t destroy_missile,
		    const creature_t * watch, blean_t * casualty);

#endif
