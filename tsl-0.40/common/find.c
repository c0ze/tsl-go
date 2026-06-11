#include <stdlib.h>
#include <string.h>

#include "stuff.h"
#include "find.h"
#include "traps.h"
#include "fov.h"
#include "game.h"


/*
  Searches a level for a creature located at the coordinates {Y,
  X}. If none was found, NULL will be returned, otherwise the creature
  found.
*/
creature_t * find_creature(const level_t * level,
			   const unsigned int y,
			   const unsigned int x)
{
  unsigned int i;
  
  if (level == NULL)
    return NULL;

  /* Loop through all creatures */
  for (i = 0; i < level->creatures; i++)
  {
    if (level->creature[i] == NULL)
      continue;
    
    /* Do the coordinates match? */
    if ((level->creature[i]->y == y) &&
	(level->creature[i]->x == x))
    {
      /* This was what we wanted to know. */
      return level->creature[i];
    }
  }
  
  /* No one was found... */
  return NULL;
} /* find_creature */



/*
  Sets the pointer located at POINTER to point to *an array* of item_t
  pointers. The array itself should be free()d after use. The array
  will include all items found at {Y, X} on LEVEL. The value returned
  is the number of items found.
*/
unsigned int find_items(const level_t * level,
			const unsigned int y,
			const unsigned int x,
			item_t *** pointer)
{
  item_t * temp;
  item_t ** ret;
  unsigned int count;

  if (on_map(level, y, x) == false)
  {
    return 0;
  }
  
  count = 0;
  ret = NULL;
  
  for (temp = level->first_item; temp != NULL; temp = temp->next_item)
  {
    if ((temp->y == y) && (temp->x == x))
    {
      count++;

      if (count > strlen(item_letters))
	break;

      if (pointer != NULL)
      {
	ret = realloc(ret, sizeof(item_t) * count);
	if (ret == NULL) out_of_memory();
	
	ret[count - 1] = temp;
	
	temp->letter = item_letters[count - 1];
      }
    }
  }
  
  /* If no items were found, ret will still be NULL. */
  if (pointer != NULL)
    *pointer = ret;

  return count;
} /* find_items */



/*
  Returns a list of all spots on LEVEL that MATCH criteria. The result
  will be an array where even number n is the y and n+1 the x portion
  of a coordinate pair. The length will be a multiple of two and
  stored at RESULT. If no spots match NULL is returned and RESULT set
  to 0.
*/
int * find_all_spots(const level_t * level,
		     int * result,
		     const find_criteria_t criteria)
{
  int y;
  int x;
  int match_count;
  int * matches;
  int * temp;
  blean_t matched;

  match_count = 0;
  matches = NULL;

  for (y = 0; y < level->size_y; y++)
   {
    for (x = 0; x < level->size_x; x++)
    {
      matched = false;

      switch (criteria)
      {
	case find_unoccupied:
	case find_obscured:
	  if (level->map[y][x] == tile_floor &&
	      find_creature(level, y, x) == NULL &&
	      find_trap(level, y, x) == NULL)
	  {
	    matched = true;

	    if (criteria == find_obscured &&
		can_see(game->player, y, x))
	    {
	      matched = false;
	    }
	  }
	  break;

	case find_water:
	  if (level->map[y][x] == tile_water)
	    matched = true;
	  break;

	case find_secret_door:
	  if (level->map[y][x] == tile_door_secret_v ||
	      level->map[y][x] == tile_door_secret_h)
	    matched = true;
	  break;

	case find_3x3:
	  if (level->map[y][x] == tile_floor &&
	      count_adjacent_floors(level, y, x) == 8)
	  {
	    matched = true;
	  }
	  break;

	case find_noitems:
	  if (level->map[y][x] == tile_floor &&
	      find_items(level, y, x, NULL) == 0)
	  {
	    matched = true;
	  }
	  break;

	case find_unconn:
	  if (y >= 1 &&
	      x >= 1 &&
	      y < level->size_y - 1 &&
	      x < level->size_x - 1)
	  {
	    if (level->map[y][x] == tile_internal_con_n ||
		level->map[y][x] == tile_internal_con_e ||
		level->map[y][x] == tile_internal_con_s ||
		level->map[y][x] == tile_internal_con_w)
	    {
	      if (level->map[y - 1][x] != tile_internal_con_n &&
		  level->map[y - 1][x] != tile_internal_con_e &&
		  level->map[y - 1][x] != tile_internal_con_s &&
		  level->map[y - 1][x] != tile_internal_con_w &&
		  level->map[y][x - 1] != tile_internal_con_n &&
		  level->map[y][x - 1] != tile_internal_con_e &&
		  level->map[y][x - 1] != tile_internal_con_s &&
		  level->map[y][x - 1] != tile_internal_con_w)
	      {
		matched = true;
	      }
	    }
	  }
	  break;

	case find_start:
	  if (level->map[y][x] == tile_internal_start)
	    matched = true;
	  break;

	case find_sweet:
	  if (level->map[y][x] == tile_internal_sweet)
	    matched = true;
	  break;

	case find_traversable:
	  if (is_traversable(level, false, y, x))
	    matched = true;
	  break;    

	case find_boss:
	  if (level->map[y][x] == tile_internal_boss)
	    matched = true;
	  break;

	case find_generator:
	  if (level->map[y][x] == tile_generator)
	    matched = true;
	  break;

	case find_closed_door:
	  if (level->map[y][x] == tile_door_closed)
	    matched = true;
	  break;

	default:
	  break;
      }

      if (matched == true)
      {
	match_count++;

	temp = realloc(matches, match_count * 2 * sizeof(unsigned int));

	if (temp == NULL)
	{
	  free(matches);
	  out_of_memory();
	}

	matches = temp;

	matches[match_count * 2 - 2] = y;
	matches[match_count * 2 - 1] = x;
      }
    }
  }

  *result = match_count;
  return matches;
} /* find_all_spots */


/*
  Picks a random location on LEVEL that matches a given CRITERIA (see
  find.h). TARGET_Y and TARGET_X are pointers to the locations where
  the chosen coordinate pair will be stored. The coordinates are only
  modified if a location is found.
  
  Returns: true if a suitable coordinate pair was found and stored in
  TARGET_Y and TARGET_X, false if no spot matching the criteria was
  found.
*/
blean_t find_spot(const level_t * level,
		  int * target_y, int * target_x,
		  const find_criteria_t criteria)
{
/*  unsigned int y;
  unsigned int x;
  unsigned int i;
  unsigned int a;
  unsigned int b;*/
  int choice;
  int match_count;
  int * matches;

  if (level == NULL)
    return false;

  matches = find_all_spots(level, &match_count, criteria);

  /* Did we find any spots that matched? */
  if (matches == NULL)
    return false;

/*  for (i = 1; i < match_count; i++)
  {
    y = tslrand() % match_count;
    x = tslrand() % match_count;
    
    if (y == x)
    {
      y++;
      y %= match_count;
    }

    a = matches[y * 2];
    b = matches[y * 2 + 1];

    matches[y * 2] = matches[x * 2];
    matches[y * 2 + 1] = matches[x * 2 + 1];

    matches[x * 2] = a;
    matches[x * 2 + 1] = b;
    }*/

  /*
    Pick one of the suitable spots and store its coordinates at the
    adresses pointed to by TARGET_X, TARGET_X.
  */
  choice = 1 + tslrnd() % match_count;
  *target_y = matches[choice * 2 - 2];
  *target_x = matches[choice * 2 - 1];
  free(matches);
  
  return true;
} /* find_spot */
