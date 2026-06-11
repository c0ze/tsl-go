/*
  ability.h

  Abilities can be mapped to shortcuts (by default keys 1-0). This map
  is kept in game->ability_slot (-1 means that shortcut isn't mapped
  to any ability).

*/

#ifndef _ABILITY_H_
#define _ABILITY_H_

#include "main.h"
#include "creature.h"

int count_abilities(const creature_t * creature);

int get_ability_cost(const creature_t * user, const attr_index_t ability);

/* RFE: Rename these use instead of invoke */
blean_t try_to_invoke_ability(creature_t * user, const attr_index_t ability);
blean_t invoke_ability(creature_t * user, const attr_index_t ability, item_t * source);

/* Anyone can use abilities but only the players needs these. */
void list_shortcuts(char * buf, const blean_t all);
blean_t ability_menu(creature_t * user, signed int sel);
int select_ability(creature_t * user);
int select_ability_slot(void);
void map_to_shortcut(signed int ability);
void unmapped_abilities(item_t * item);
void explain_ability(char * dest, const unsigned int ability);

#endif
