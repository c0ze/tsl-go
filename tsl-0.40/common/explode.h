/*
  explode.c - things related to explosions.
*/

#ifndef _EXPLODE_H_
#define _EXPLODE_H_

#include "main.h"
#include "creature.h"

enum explosion_type_t
{
  expl_explosion,
  expl_fireball,
  expl_booby_trap,
  expl_thrown,
  expl_fired,
  expl_wand,
  expl_book
};
typedef enum explosion_type_t explosion_type_t;

void explosion(level_t * level,
	       creature_t * cause, item_t * source,
	       const unsigned int center_y, const unsigned int center_x,
	       const explosion_type_t e_type, const unsigned int dam,
	       creature_t * watch, blean_t * casualty);

#endif
