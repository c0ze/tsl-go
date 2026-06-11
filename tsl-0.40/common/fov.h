/*
  fov.h
*/

#ifndef _FOV_H_
#define _FOV_H_

#include "main.h"
#include "level.h"
#include "creature.h"
/*#include <stdint.h>*/

void build_fov(creature_t * observer);

void fov_ray(unsigned int map[FOV_RANGE * 2 + 1][FOV_RANGE * 2 + 1], const unsigned charge,
	     const unsigned int y, const unsigned int x,
	     const signed int y_speed, const signed int x_speed,
	     const unsigned int bounce);

blean_t can_see_creature(const creature_t * observer,
			 const creature_t * target);
blean_t can_see(const creature_t * observer,
		const unsigned int target_y,
		const unsigned int target_x);
blean_t can_see_anyone(void);

#endif
