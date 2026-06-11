#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include "main.h"
#include "fov.h"
#include "traps.h"
#include "game.h"
#include "effect.h"
#include "options.h"



/*
  Calculates what areas of the current level are visible to
  OBSERVER. The result will be stored in OBSERVER->FOV.
*/
void build_fov(creature_t * observer)
{
  signed int middle_y;
  signed int middle_x;

  signed int y;
  signed int x;

  signed int level_y;
  signed int level_x;

  unsigned int count;

  unsigned int amount;
  unsigned int bounce;

  level_t * level;

  /* Temporary storage */
  unsigned int local_fov[FOV_RANGE * 2 + 1][FOV_RANGE * 2 + 1];

  if (observer == NULL)
    return;

  level = observer->location;

  if (level == NULL)
    return;

  /* These are at the center of the FOV field. */
  middle_y = middle_x = FOV_RANGE;

  /* Blindness and sleep - the FOV will remain blacked out. */
  if (is_blinded(observer) ||
      get_effect_by_id(observer, effect_sleep))
  {
    for (y = 0; y < FOV_RANGE * 2 + 1; y++)
      for (x = 0; x < FOV_RANGE * 2 + 1; x++)
	observer->fov[y][x] = false;

    /* We need this to see ourselves. */
    observer->fov[middle_y][middle_x] = true;

    return;
  }

  /* Clear the arrays */
  for (y = 0; y < FOV_RANGE * 2 + 1; y++)
  {
    for (x = 0; x < FOV_RANGE * 2 + 1; x++)
    {
      level_y = observer->y + y - middle_y;
      level_x = observer->x + x - middle_x;

      local_fov[y][x] = 0;

      if (on_map(level, level_y, level_x))
      {
	if (tile_info[level->map[level_y][level_x]]->opaque)
	{
	  local_fov[y][x] = 256;
	}
      }
    }
  }
  
  /* How far can we see? */
  amount = attr_current(observer, attr_vision);
  bounce = 5;

  /* Center tile is always lit. */
/*  fov[middle_y][middle_x] = 255;*/

  if (amount >= 1)
  {
    fov_ray(local_fov, amount, middle_y, middle_x, 0, +1, bounce);
    fov_ray(local_fov, amount, middle_y, middle_x, 0, -1, bounce);
    fov_ray(local_fov, amount, middle_y, middle_x, +1, 0, bounce);
    fov_ray(local_fov, amount, middle_y, middle_x, -1, 0, bounce);
  }

  if (amount >= 2)
  {
    fov_ray(local_fov, amount - 1, middle_y - 1, middle_x - 1, -1, +1, bounce);
    fov_ray(local_fov, amount - 1, middle_y - 1, middle_x - 1, +1, -1, bounce);
    fov_ray(local_fov, amount - 1, middle_y + 1, middle_x + 1, -1, +1, bounce);
    fov_ray(local_fov, amount - 1, middle_y + 1, middle_x + 1, +1, -1, bounce);
    
    fov_ray(local_fov, amount - 1, middle_y - 1, middle_x + 1, -1, -1, bounce);
    fov_ray(local_fov, amount - 1, middle_y - 1, middle_x + 1, +1, +1, bounce);
    fov_ray(local_fov, amount - 1, middle_y + 1, middle_x - 1, -1, -1, bounce);
    fov_ray(local_fov, amount - 1, middle_y + 1, middle_x - 1, +1, +1, bounce);

    fov_ray(local_fov, amount - 1, middle_y, middle_x - 1, -1, -1, bounce);
    fov_ray(local_fov, amount - 1, middle_y, middle_x - 1, +1, -1, bounce);
    fov_ray(local_fov, amount - 1, middle_y, middle_x + 1, -1, +1, bounce);
    fov_ray(local_fov, amount - 1, middle_y, middle_x + 1, +1, +1, bounce);

    fov_ray(local_fov, amount - 1, middle_y - 1, middle_x, -1, -1, bounce);
    fov_ray(local_fov, amount - 1, middle_y - 1, middle_x, -1, +1, bounce);
    fov_ray(local_fov, amount - 1, middle_y + 1, middle_x, +1, -1, bounce);
    fov_ray(local_fov, amount - 1, middle_y + 1, middle_x, +1, +1, bounce);

    fov_ray(local_fov, amount - 1, middle_y, middle_x, -1, -1, bounce);
    fov_ray(local_fov, amount - 1, middle_y, middle_x, -1, +1, bounce);
    fov_ray(local_fov, amount - 1, middle_y, middle_x, +1, +1, bounce);
    fov_ray(local_fov, amount - 1, middle_y, middle_x, +1, -1, bounce);
  }
  
  for (y = 0; y < FOV_RANGE * 2; y++)
  {
    for (x = 0; x < FOV_RANGE * 2; x++)
    {
      count = 0;
      
/*      for (i = 1; i <= 128; i *= 2)
	if (local_fov[y][x] & i)
	  count++;
      
	  if (count >= 1)*/

      if (local_fov[y][x] & 255)
	local_fov[y][x] |= 512;
    }
  }

/*
  for (y = 0; y < FOV_RANGE * 2; y++)
  {
    for (x = 0; x < FOV_RANGE * 2; x++)
    {
      if ((local_fov[y][x] & 256) == 0)
	continue;

      count = 0;

      if ((y > 0) && (local_fov[y - 1][x] & 512))
	count++;

      if ((x < FOV_RANGE * 2) && (local_fov[y][x + 1] & 512))
	count++;

      if ((y < FOV_RANGE * 2) && (local_fov[y + 1][x] & 512))
	count++;

      if ((x > 0) && (local_fov[y][x - 1] & 512))
	count++;

      if (count >= 1)
	local_fov[y][x] |= 1024;
    }
  }
*/

  for (y = 1; y < FOV_RANGE * 2 - 1; y++)
  {
    for (x = 1; x < FOV_RANGE * 2 - 1; x++)
    {
      if (local_fov[y][x] & 256)
	continue;

      count = 0;

      if ((local_fov[y - 1][x - 1] & 256) &&
	  (local_fov[y + 1][x] & 512) &&
	  (local_fov[y][x + 1] & 512))
	count++;

      if ((local_fov[y + 1][x + 1] & 256) &&
	  (local_fov[y - 1][x] & 512) &&
	  (local_fov[y][x - 1] & 512))
	count++;

      if ((local_fov[y + 1][x - 1] & 256) &&
	  (local_fov[y - 1][x] & 512) &&
	  (local_fov[y][x + 1] & 512))
	count++;

      if ((local_fov[y - 1][x + 1] & 256) &&
	  (local_fov[y + 1][x] & 512) &&
	  (local_fov[y][x - 1] & 512))
	count++;

      if (count >= 1)
	local_fov[y][x] |= 1024;
    }
  }

  for (y = 1; y < FOV_RANGE * 2 - 1; y++)
  {
    for (x = 1; x < FOV_RANGE * 2 - 1; x++)
    {
      if (local_fov[y][x] & 1024)
	local_fov[y][x] |= 512;
    }
  }

  /* Let's transfer what we've learned to the observers own FOV field. */

  for (y = 0; y < FOV_RANGE * 2 + 1; y++)
  {
    for (x = 0; x < FOV_RANGE * 2 + 1; x++)
    {
      observer->fov[y][x] = false;
      
      if (local_fov[y][x] & 512)
	observer->fov[y][x] = true;
      
      if (local_fov[y][x] & 1024)
	observer->fov[y][x] = true;

      if (options.debug_fov)
	continue;

      if ((y > 0) && (x > 0) && (local_fov[y - 1][x - 1] & 512))
	observer->fov[y][x] = true;
      
      if ((y > 0) && (local_fov[y - 1][x] & 512))
	observer->fov[y][x] = true;
      
      if ((y > 0) && (x < FOV_RANGE * 2) && (local_fov[y - 1][x + 1] & 512))
	observer->fov[y][x] = true;
      
      if ((x > 0) && (local_fov[y][x - 1] & 512))
	observer->fov[y][x] = true;
      
      if ((x < FOV_RANGE * 2) && (local_fov[y][x + 1] & 512))
	observer->fov[y][x] = true;
      
      if ((y < FOV_RANGE * 2) && (x > 0) && (local_fov[y + 1][x - 1] & 512))
	observer->fov[y][x] = true;
      
      if ((y < FOV_RANGE * 2) && (local_fov[y + 1][x] & 512))
	observer->fov[y][x] = true;
      
      if ((y < FOV_RANGE * 2) && (x < FOV_RANGE * 2) && (local_fov[y + 1][x + 1] & 512))
	observer->fov[y][x] = true;
    }
  }

  /*
    Walls within the 3x3 around the observer should always be displayed.
  */
/*  for (y = middle_y - 1; y <= middle_y + 1; y++)
    for (x = middle_x - 1; x <= middle_x + 1; x++)
      if (local_fov[y][x] & 256 && abs(y - middle_y) <= 1 && abs(x - middle_x) <= 1)
      observer->fov[y][x] = true;*/
  
  observer->fov[middle_y][middle_x] = true;

  return;
} /* build_fov */



void fov_ray(unsigned int map[FOV_RANGE * 2 + 1][FOV_RANGE * 2 + 1], const unsigned charge,
	     const unsigned int y, const unsigned int x,
	     const signed int y_speed, const signed int x_speed,
	     const unsigned int bounce)
{
  unsigned int dir;

  if (map == NULL)
    return;

  if ((y < 0) || (x < 0) || (y > FOV_RANGE * 2) || (x > FOV_RANGE * 2))
    return;

  if (charge == 0)
    return;

  if (bounce == 0)
    return;

  if (map[y][x] & 256)
    return;

  if ((y_speed == -1) && (x_speed == -1))
    dir = 1;
  else if ((y_speed == -1) && (x_speed == 0))
    dir = 2;
  else if ((y_speed == -1) && (x_speed == +1))
    dir = 4;
  else if ((y_speed == 0) && (x_speed == +1))
    dir = 8;
  else if ((y_speed == +1) && (x_speed == +1))
    dir = 16;
  else if ((y_speed == +1) && (x_speed == 0))
    dir = 32;
  else if ((y_speed == +1) && (x_speed == -1))
    dir = 64;
  else if ((y_speed == 0) && (x_speed == -1))
    dir = 128;
  else
    return;

  /* Mark that we've been here */
  map[y][x] |= dir;

  if (map[y + y_speed][x + x_speed] & 256)
  {
    switch (dir)
    {
      case 1:
	fov_ray(map, charge - 1, y - 1, x,      0, +1, bounce - 1);
	fov_ray(map, charge - 1, y,     x - 1, +1,  0, bounce - 1);
	break;

      case 2:
	fov_ray(map, charge - 1, y, x + 1, +1, +1, bounce - 1);
	fov_ray(map, charge - 1, y, x - 1, +1, -1, bounce - 1);

      case 4:
	fov_ray(map, charge - 1, y - 1, x,      0, -1, bounce - 1);
	fov_ray(map, charge - 1, y,     x + 1, +1,  0, bounce - 1);
	break;

      case 8:
	fov_ray(map, charge - 1, y - 1, x, -1, -1, bounce - 1);
	fov_ray(map, charge - 1, y + 1, x, +1, -1, bounce - 1);
	break;

      case 16:
	fov_ray(map, charge - 1,     y, x + 1, -1,  0, bounce - 1);
	fov_ray(map, charge - 1, y + 1,     x,  0, -1, bounce - 1);
	break;

      case 32:
	fov_ray(map, charge - 1, y, x + 1, -1, +1, bounce - 1);
	fov_ray(map, charge - 1, y, x - 1, -1, -1, bounce - 1);
	break;

      case 64:
	fov_ray(map, charge - 1, y,     x - 1, -1,  0, bounce - 1);
	fov_ray(map, charge - 1, y + 1, x,      0, +1, bounce - 1);
	break;

      case 128:
	fov_ray(map, charge - 1, y - 1, x, -1, +1, bounce - 1);
	fov_ray(map, charge - 1, y + 1, x, +1, +1, bounce - 1);
	break;
    }
  }
  else
  {
    fov_ray(map, charge - 1, y + y_speed, x + x_speed, y_speed, x_speed, bounce);
  }
    
  return;
} /* fov_ray */



/*
  Returns true if OBSERVER can see TARGET, otherwise false.
*/
blean_t can_see_creature(const creature_t * observer,
			 const creature_t * target)
{
  level_t * level;
  
  if (observer == NULL || target == NULL)
  {
    return false;
  }

  if (observer == target)
    return true;
  
  if (is_blinded(observer))
    return false;

  level = observer->location;

  if (level == NULL || level != target->location)
  {
    return false;
  }

  if (on_map(level, target->y, target->x) == false ||
      on_map(level, observer->y, observer->x) == false)
  {
    return false;
  }

  /* RFE: I don't think this is needed, since the FOV matrix should be empty anyway. Safe to remove? */
/*  if (attr_current(observer, attr_blindness) > 0)
  {
    return false;
    }*/
  
  /* Is the target within the LOS of the observer? */
  if (can_see(observer, target->y, target->x))
    return true;

  /* */
  if (abs(observer->y - target->y) <= 1 &&
      abs(observer->x - target->x) <= 1)
  {
    return true;
  }
  
  return false;
} /* can_see_creature */



/*
  Returns true if {TARGET_Y, TARGET_X} is within sight of OBSERVER on
  the level it is on.
*/
blean_t can_see(const creature_t * observer,
		const unsigned int target_y, const unsigned int target_x)
{
  level_t * level;
  signed grid_y;
  signed grid_x;
/*  unsigned int i;*/
/*  creature_t * ally;*/

  if (observer == NULL)
    return false;

  level = observer->location;

  if (on_map(level, target_y, target_x) == false)
    return false;

  grid_y = FOV_RANGE - observer->y + target_y;
  grid_x = FOV_RANGE - observer->x + target_x;

  if ((grid_y < 0) ||
      (grid_x < 0) ||
      (grid_y >= FOV_RANGE * 2 + 1) ||
      (grid_x >= FOV_RANGE * 2 + 1))
  {
    return false;
  }
  
  if (observer->fov[grid_y][grid_x])
  {
    return true;
  }

/*  if (is_player(observer))
  {
    for (i = 0; i < level->creatures; i++)
    {
      ally = level->creature[i];

      if (ally == NULL ||
	  ally == observer) ||
	  observer->alignment != ally->alignment)
      {
	continue;
      }

      if (can_see(ally, target_y, target_x))
	return true;
    }
    }*/

  return false;
} /* can_see */



/*
  Returns true if the player can see any hostile creature.

  This is intended for actions that span over many turns but should be
  interrupted if some enemies wanders close.
*/
blean_t can_see_anyone()
{
  unsigned int i;
  unsigned int y;
  unsigned int x;

  creature_t * temp;
  level_t * level;

  if (game->player == NULL)
    return false;

  level = get_current_level();

  if (level == NULL)
    return false;

  for (i = 0; i < level->creatures; i++)
  {
    temp = level->creature[i];

    if (temp == NULL ||
	is_player(temp))
    {
      continue;
    }

    if (temp->detected == false)
      continue;
    
    y = temp->y;
    x = temp->x;
    
    if (on_map(level, y, x) == false)
      continue;

/*    if (temp->alignment == alignment_player)
      continue;*/

    if (can_see(game->player, y, x) == true)
    {
      return true;
    }
  }

  return false;
} /* can_see_anyone */
