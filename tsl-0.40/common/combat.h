/*
  combat.h - code related to melee and ranged combat.
*/

#ifndef _COMBAT_H_
#define _COMBAT_H_

#include "main.h"
#include "creature.h"

void melee_hit(creature_t * attacker, creature_t * defender, item_t * weapon);
blean_t weapon_effect(creature_t * attacker, creature_t * defender, item_t * weapon);

unsigned int damage_armor(creature_t * creature, signed int damage, const damage_type_t dt);

void hand_string(char * dest, const creature_t * creature, const blean_t plural);
void get_attack_strings(const attack_string_t str, char * third, char * self);
blean_t attack(creature_t * attacker, creature_t * defender);
blean_t attack_internal(creature_t * attacker,
			creature_t * defender,
			item_t * weapon);

blean_t weapon_break(creature_t * attacker, item_t * weapon);

item_t * next_weapon(creature_t * attacker);

#endif
