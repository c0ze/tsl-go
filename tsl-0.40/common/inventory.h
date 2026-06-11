/*
  inventory.h - this lets creatures hold and use items.

  The distinction between item.* and inventory.* is that item.* deals
  with individual items while inventory.* is more interested in groups
  of items.
*/

#ifndef _INVENTORY_H_
#define _INVENTORY_H_

#include "main.h"
#include "item.h"

item_t * find_item_in_inventory(const creature_t * owner,
				const unsigned int number);
item_t * attach_item_to_creature(creature_t * creature, item_t * item);
item_t * get_item_by_letter(const creature_t * creature,
			    const char sought_letter);
blean_t use_or_destroy(creature_t * creature, item_t * item);
void switch_inventory(creature_t * a, creature_t * b);

int count_inventory(const creature_t * owner,  blean_t (* criteria) (const item_t *));
item_t * find_known_item(creature_t * owner, const int treasure);

#endif
