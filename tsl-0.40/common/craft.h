/*
  craft.h

  Item interactions. Applying and combining items.
*/

#ifndef _CRAFT_H_
#define _CRAFT_H_

#include "item.h"

blean_t apply(creature_t * creature, item_t * item);

use_func_t apply_func(const item_t * item);

blean_t grind(creature_t * creature, item_t * item);
blean_t torch(creature_t * creature, item_t * item);
blean_t hacksaw(creature_t * creature, item_t * item);
blean_t dataprobe(creature_t * creature, item_t * item);

blean_t butcher(creature_t * creature, item_t * item);

blean_t can_be_poisoned(const item_t * item);
blean_t poison_item(creature_t * creature, item_t * item);
blean_t set_trap(creature_t * creature, item_t * item);
void upgrade_ammo(item_t * item);

#endif
