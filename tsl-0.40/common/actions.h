/*
  actions.h
*/

#ifndef _ACTIONS_H_
#define _ACTIONS_H_

#include "monster.h"
#include "main.h"
#include "eat.h"

enum noise_level_t
{
  noise_faint,
  noise_loud,
  noise_terrible
};
typedef enum noise_level_t noise_level_t;

void equip_artifact(creature_t * owner, item_t * item);

void split_creature(creature_t * creature);
blean_t use_item(creature_t * creature, item_t * item);
void recharge(creature_t * recharger, item_t * item, item_t * source);
void drop_item(creature_t * owner, item_t * item, const blean_t silent);
void floor_effect(item_t * item);
blean_t yell(const creature_t * creature);
void aggravate(creature_t * creature);
void set_enemy_target(creature_t * creature,
		      const unsigned int y, const unsigned int x);
blean_t interact(creature_t * creature);
void traverse_branch(const unsigned int type);

void instant_reveal(creature_t * creature);
blean_t reveal_mimic(creature_t * mimic);
blean_t reveal_creature(creature_t * creature);

blean_t pickup(creature_t * creature, item_t * item, const blean_t first_item);
blean_t stairs(creature_t * creature, const blean_t message);

void draw_attention(level_t * level,
		    const unsigned int source_y,
		    const unsigned int source_x,
		    const unsigned int range);

blean_t shift_action(creature_t * creature, const dir_t dir);
blean_t can_fire_weapon(creature_t * creature, const blean_t prohibit_throw);
blean_t spend_charge(item_t * item);

blean_t throw_prompt(creature_t * creature, item_t * item);
blean_t label_prompt(item_t * item);

use_func_t use_func(const item_t * item);

#endif
