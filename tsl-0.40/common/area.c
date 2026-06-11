#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <ctype.h>

#include "area.h"
#include "stuff.h"
#include "level.h"
#include "game.h"
#include "creature.h"
#include "find.h"



/*
  Builds a new area_t with its coordinates set to {Y, X}.
*/
area_t * new_area(const unsigned int y, const unsigned int x)
{
  area_t * temp;
  
  temp = malloc(sizeof(area_t));
  
  if (temp == NULL)
    out_of_memory();
  
  temp->next = NULL;

  temp->y = y;
  temp->x = x;

  temp->param = 0;

  return temp;
} /* new_area */



/*
  Frees AREA and any areas it links to.
*/
void del_area(area_t * area)
{
  area_t * temp;

  temp = area;

  while (temp != NULL)
  {
    temp = area->next;
    free(area);
    area = temp;
  }

  return;
} /* del_area */



/*
  Adds {Y, X} to the area set AREA.

  If the coordinates already exist in the set, nothing happens. The
  new area_t will be attached at the end of the linked list.

  A pointer to the latest attached structure will be returned.
*/
area_t * grow_area(area_t * area, const unsigned int y, const unsigned int x)
{
  area_t * temp;

  if (area == NULL)
    return NULL;

  temp = area;

  do
  {
    if (temp->y == y && temp->x == x)
      return NULL; /* We already have this point. */
    
    if (temp->next == NULL)
    {
      temp->next = new_area(y, x);
      return temp->next;
    }

    temp = temp->next;
  }
  while (temp != NULL);

  /* We should never get here */
 
  return NULL;
} /* grow_area */



/*
  Projects a "ball" (a 3x3 tile square) on LEVEL, centered at
  {CENTER_Y, CENTER_X} and with a "one in PROBABILITY" chance of
  affecting each tile (0 probability is treated as 1).
*/
void area_ball(area_t * area, level_t * level,
	       const unsigned int center_y, const unsigned int center_x,
	       const unsigned int probability)
{
  unsigned int y;
  unsigned int x;
  
  if ((area == NULL) || (level == NULL))
    return;
  
  for (y = center_y - 1; y <= center_y + 1; y++)
  {
    for (x = center_x - 1; x <= center_x + 1; x++)
    {
      if (on_map(level, y, x) && 
	  (is_weak_wall(level, y, x) || (wall_or_door(level, y, x) == false)) &&
	  tslrnd() % MAX(1, probability) == 0)
      {
	grow_area(area, y, x);
      }	  
    } 
  }
  
  return;
} /* area_ball */



/*
  Projects a cone on LEVEL starting at {Y, X} and extending in DIR for
  RANGE squares or until it hits an obstacle. The area of the cone is
  the same for cardinal and diagonal directions. If RAY is true, the
  cone will be restricted to a single line and won't swell. The cone
  will be appended to AREA.
*/
void area_cone(area_t * area, level_t * level,
	       unsigned int y, unsigned int x,
	       const dir_t dir, unsigned int range,
	       const blean_t ray)
{
  signed int move_y;
  signed int move_x;
  unsigned int swell;

  if (area == NULL || level == NULL)
    return;

  swell = 0;

  while (range > 0)
  {
    range--;
    swell++;

    swell = MIN(range, swell);

    dir_to_speed(dir, &move_y, &move_x);

    y += move_y;
    x += move_x;

    if (on_map(level, y, x) == false ||
	(is_flyable(level, y, x) == false &&
	 find_creature(level, y, x) == NULL))
    {
      break;
    }

    grow_area(area, y, x);

    if (!ray)
    {
      area_cone(area, level, y, x, (dir + 6) % 8 + 1, swell, true);
      area_cone(area, level, y, x, MAX(1, (dir + 1) % 9), swell, true);
    }
  }
  
  return;
} /* area_breath */



void area_cloud(area_t * area, level_t * level,
		unsigned int size)
{
  area_t * edge;
  area_t * temp;
  unsigned int count;
    
  if (area == NULL ||
      level == NULL ||
      size == 0)
    return;
  
  while (sum_area(area) < size)
  {
    edge = area_edge(area, level);

    count = sum_area(edge);

    if (count == 0)
    {
      /* There's no more room for the cloud. */
      return;
    }

    count = tslrnd() % count;

    temp = edge;

    while (count > 0)
    {
      count--;
      temp = temp->next;
    }
    
    grow_area(area, temp->y, temp->x);

    del_area(edge);
  }

  return;
} /* area_cloud */



/*
  Returns the structure of the coordinate pair {Y, X}, if it is
  present in AREA.
*/
area_t * in_area(area_t * area, const unsigned int y, const unsigned int x)
{
  while (area != NULL)
  {
    if (area->y == y && area->x == x)
      return area;

    area = area->next;
  }

  return NULL;
} /* area_has */



/*
  Returns the total number of elements in AREA (at least 1, if AREA is non-null).
*/
unsigned int sum_area(area_t * area)
{
  unsigned int count;

  count = 0;

  while (area != NULL)
  {
    count++;

    area = area->next;
  }

  return count;
} /* sum_area */



/*
  Returns an area set with all traversable tiles on LEVEL that are
  orthogonally adjacent to the ones in AREA.

  Note that this works slightly different than most other area
  functions - AREA is only for input and won't be modified. The result
  must be saved and freed.
*/
area_t * area_edge(area_t * area, level_t * level)
{
  unsigned int y;
  unsigned int x;
  area_t * edge;
  area_t * temp;

  if (area == NULL ||
      level == NULL)
    return NULL;

  edge = new_area(area->y, area->x);
  
  for (temp = area; temp != NULL; temp = temp->next)
  {
    y = temp->y;
    x = temp->x;
    
    if (y < 1 ||
	x < 1 ||
	y >= level->size_y ||
	x >= level->size_x)
    {
      continue;
    }

    if (in_area(area, y - 1, x) == NULL &&
	(is_walkable(level, false, y - 1, x) ||
	 find_creature(level, y - 1, x) ||
	 get_tile(level, y - 1, x) == tile_internal_sweet))
      grow_area(edge, y - 1, x);
    
    if (in_area(area, y, x - 1) == NULL &&
	(is_walkable(level, false, y, x - 1) ||
	 find_creature(level, y, x - 1) ||
	 get_tile(level, y, x - 1) == tile_internal_sweet))
      grow_area(edge, y, x - 1);
    
    if (in_area(area, y + 1, x) == NULL &&
	(is_walkable(level, false, y + 1, x) ||
	 find_creature(level, y + 1, x) ||
	 get_tile(level, y + 1, x) == tile_internal_sweet))
      grow_area(edge, y + 1, x);
    
    if (in_area(area, y, x + 1) == NULL &&
	(is_walkable(level, false, y, x + 1) ||
	 find_creature(level, y, x + 1) ||
	 get_tile(level, y, x + 1) == tile_internal_sweet))
      grow_area(edge, y, x + 1);
  }

  /*
    Remove the head of the list (the one containing coordinates of the
    first element in AREA.
  */
  temp = edge->next;
  edge->next = NULL;
  del_area(edge);
  return temp;
} /* area_edge */
