/*
  area.c

  An area in this context is not an interval but a set of coordinate
  pairs. This is implemented as a linked list of area_t structures.

  For a spell area of effect the first area_t will typically be the
  casters own location and thus ignored in the spell effect logic.
*/

#ifndef _AREA_H_
#define _AREA_H_

#include "main.h"
#include "creature.h"


struct area_t
{
  unsigned int y;
  unsigned int x;

  unsigned int param;

  area_t * next;
};



area_t * new_area(const unsigned int y, const unsigned int x);
area_t * grow_area(area_t * area, const unsigned int y, const unsigned int x);
void del_area(area_t * area);
area_t * area_edge(area_t * area, level_t * level);

area_t * in_area(area_t * area, const unsigned int y, const unsigned int x);
unsigned int sum_area(area_t * area);

void area_ball(area_t * area, level_t * level,
	       const unsigned int center_y, const unsigned int center_x,
	       const unsigned int probability);

void area_cone(area_t * area, level_t * level,
	       unsigned int y, unsigned int x,
	       const dir_t dir, unsigned int range,
	       const blean_t ray);

void area_cloud(area_t * area, level_t * level,
		unsigned int size);

#endif
