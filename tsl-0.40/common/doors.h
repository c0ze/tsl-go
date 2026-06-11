/*
  doors.h
*/

#ifndef _DOORS_H_
#define _DOORS_H_

#include "main.h"

blean_t close_door(creature_t * creature);
blean_t open_door(creature_t * creature,
		  const int door_y,
		  const int door_x);
blean_t key_open(creature_t * creature, item_t * key);
void unlock_door(creature_t * creature,
		 level_t * level,
		 const int door_y,
		 const int door_x,
		 item_t * key);
void maybe_locked_door(level_t * level, const int y, const int x);
void replace_doors(level_t * level);

#endif
