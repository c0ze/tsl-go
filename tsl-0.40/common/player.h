/*
  player.h
*/

#ifndef _PLAYER_H_
#define _PLAYER_H_

#include <time.h>

#include "main.h"
#include "creature.h"
#include "magic.h"
#include "input.h"
#include "select.h"

enum flag_mode_t
{
  flag_mode_stats,
  flag_mode_summary,
  flag_mode_morgue
};
typedef enum flag_mode_t flag_mode_t;

/* Character creation */
creature_t * create_character(void);

/* In-game */
void player_control(creature_t * creature);
unsigned int inventory_action(creature_t * creature, const action_t input);
blean_t try_to_move(creature_t * creature,
		    signed int move_y, signed int move_x);
void look_at_location(const creature_t * observer, const level_t * level,
		      const unsigned int y, const unsigned int x,
		      const blean_t forced);
blean_t needs_rest(creature_t * creature, const blean_t message);
void break_rest(creature_t * creature);

void display_summary(creature_t * creature);
void inspect_facet_aug(const item_t * item);

void msg_one(creature_t * creature, const char * message);
void msg_the(creature_t * creature, const char * message);

void throw_or_fire(creature_t * creature, const dir_t dir);

void status_flags(const creature_t * creature, char * buf, const flag_mode_t mode);

void list_item(const item_t * item);

blean_t try_to_rest(creature_t * creature);

blean_t dwim(creature_t * creature);

void msg_expire(effect_t * effect);

#endif
