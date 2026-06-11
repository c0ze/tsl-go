/*
  equip.h
*/

#ifndef _EQUIP_H_
#define _EQUIP_H_

#include "main.h"
#include "creature.h"

blean_t equip_item(item_t * item);
void equip_msg(creature_t * creature, item_t * item);
blean_t unequip_item(item_t * item, blean_t message);
void unequip_msg(creature_t * owner, item_t * item);
void quiver_cycle(creature_t * creature);
item_t * get_equipped(const creature_t * owner,
		      const item_type_t item_type,
		      const unsigned int n);
unsigned int count_item_type(const creature_t * owner,
			     const item_type_t item_type,
			     const blean_t only_equipped);
item_t * equipped_ranged(const creature_t * creature);
blean_t try_to_equip(item_t * item);
blean_t try_to_unequip(item_t * item, blean_t message);

#endif
