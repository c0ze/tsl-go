/*
*/

#ifndef _FIND_H_
#define _FIND_H_

#include <stdint.h>
#include "level.h"

enum find_criteria_t
{
  find_unoccupied, /* unoccupied floor tile (no creature, no trap) */
  find_obscured, /* unoccupied tile the player can't see */
  find_secret_door, /* a secret door (v or h) */
  find_water, /* unoccupied water */
  find_closed_door,
  find_3x3, /* the center of any 3x3 area of unoccupied floor tiles */
  find_noitems, /* floor without items */
  find_start, /* player start location */
  find_sweet, /* a "sweet spot" */
  find_boss, /* a boss location */
  find_traversable, /* any traversable tile */
  find_ffpos, /* a forcefield generator placeholder */
  find_generator, /* a PROPER forcefield generator */
  find_unconn
};
typedef enum find_criteria_t find_criteria_t;


creature_t * find_creature(const level_t * level,
			   const unsigned int y,
			   const unsigned int x);

unsigned int find_items(const level_t * level,
			const unsigned int y,
			const unsigned int x,
			item_t *** pointer);

int * find_all_spots(const level_t * level,
		     int * result,
		     const find_criteria_t criteria);


blean_t find_spot(const level_t * level,
		  int * target_y, int * target_x,
		  const find_criteria_t criteria);

#endif
