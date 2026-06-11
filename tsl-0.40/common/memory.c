#include <stdlib.h>

#include "memory.h"
#include "stuff.h"
#include "main.h"
#include "fov.h"
#include "game.h"
#include "traps.h"



/*
  Copies visible parts of the level into level memory.
*/
void update_level_memory(level_t * level)
{
  unsigned int y;
  unsigned int x;
  unsigned int mask;

  trap_t * trap;

  if (level == NULL)
    return;

  /* Loop through the map */
  for (y = 0; y < level->size_y; y++)
  {
    for (x = 0; x < level->size_x; x++)
    {
      /* Only update the parts that are visible */
      if (can_see(game->player, y, x) == false)
      {
	/* RFE: Add this (under condition of some player attribute) to disable level memory: */
	/* level->memory[y][x] = tile_internal_blank;*/

	continue;
      }

      /* This tile is seen, update it in level memory */
      level->memory[y][x] = tile_info[level->map[y][x]]->gent;
    }
  }

  /* Add traps the player can see to level memory. */
  trap = level->first_trap;
  
  for (trap = level->first_trap; trap != NULL; trap = trap->next_trap)
  {
    if (trap != NULL &&
	trap->hidden == false &&
	trap->revealed == true &&
	can_see(game->player, trap->y, trap->x))
    {
      level->memory[trap->y][trap->x] = trap->gent;
    }
  }

  /*
    Now we'll go through the map a second time, fixing broken
    T-sections. We couldn't do this until we had a fully updated view.
  */
  for (y = 0; y < level->size_y; y++)
  {
    for (x = 0; x < level->size_x; x++)
    {
      /*
	We only need to do this with T-sections and skip anything else.
	
	An interesting side effect: if applied to corners they will
	be broken into straight lines, until we know for certain
	what's on the other side. This is technically more correct
	since it truly represents what the _character_ knows, but it
	feels silly.
      */
      if (level->map[y][x] != tile_wall_nes &&
	  level->map[y][x] != tile_wall_nsw &&
	  level->map[y][x] != tile_wall_new &&
	  level->map[y][x] != tile_wall_esw &&
	  level->map[y][x] != tile_wall_cross)
      {
	continue;
      }

      if (level->memory[y][x] == gent_blank ||
	  level->memory[y][x] == gent_obstacle)
      {
	continue;
      }
     
      /*
	We're examining a T-section wall. Now we need to build a
	bitmask how much the player knows about what's behind
	it. Check all cardinal directions if they should connect to
	the current tile.
      */
      mask = 0;
      
      if (memory_should_connect(level, y - 1, x) == true)
	mask |= 1;
      
      if (memory_should_connect(level, y, x - 1) == true)
	mask |= 2;

      if (memory_should_connect(level, y + 1, x) == true)
	mask |= 4;

      if (memory_should_connect(level, y, x + 1) == true)
	mask |= 8;

      /*
	Deal with things like
	
	##
	A#
	##
	
	A does _not_ need to connect east if we can _assume_ all 5
	tiles around it are also connected. This is where the pretty
	unbroken walls are made.
      */
      if (believe_should_connect(level, y - 1, x    ) &&
	  believe_should_connect(level, y - 1, x + 1) &&
	  believe_should_connect(level, y    , x + 1) &&
	  believe_should_connect(level, y + 1, x + 1) &&
	  believe_should_connect(level, y + 1, x    ))
      {
	mask &= ~8;
      }

      if (believe_should_connect(level, y - 1, x    ) &&
	  believe_should_connect(level, y - 1, x - 1) &&
	  believe_should_connect(level, y    , x - 1) &&
	  believe_should_connect(level, y + 1, x - 1) &&
	  believe_should_connect(level, y + 1, x    ))
      {
	mask &= ~2;
      }

      if (believe_should_connect(level, y    , x - 1) &&
	  believe_should_connect(level, y - 1, x - 1) &&
	  believe_should_connect(level, y - 1, x    ) &&
	  believe_should_connect(level, y - 1, x + 1) &&
	  believe_should_connect(level, y    , x + 1))
      {
	mask &= ~1;
      }

      if (believe_should_connect(level, y    , x - 1) &&
	  believe_should_connect(level, y + 1, x - 1) &&
	  believe_should_connect(level, y + 1, x    ) &&
	  believe_should_connect(level, y + 1, x + 1) &&
	  believe_should_connect(level, y    , x + 1))
      {
	mask &= ~4;
      }

      /* get_wall_tile() is guaranteed to return a valid tile type. */
      level->memory[y][x] = tile_info[get_wall_tile(mask)]->gent;
    }
  }

  return;
} /* update_level_memory */



/*
  Returns true if {Y, X} in LEVELs level memory is a type of tile that
  should connect with other walls.
*/
blean_t memory_should_connect(const level_t * level,
			      const unsigned int y,
			      const unsigned int x)
{
  if (on_map(level, y, x) == false)
    return false;

/*  if (level->memory[y][x] == gent_blank)
    return true;*/

  if (level->memory[y][x] == gent_door_closed ||
      level->memory[y][x] == gent_door_open ||
      level->memory[y][x] == gent_wall_v ||
      level->memory[y][x] == gent_wall_h ||
      level->memory[y][x] == gent_wall_es ||
      level->memory[y][x] == gent_wall_sw ||
      level->memory[y][x] == gent_wall_ne ||
      level->memory[y][x] == gent_wall_nw ||
      level->memory[y][x] == gent_wall_nes ||
      level->memory[y][x] == gent_wall_nsw ||
      level->memory[y][x] == gent_wall_new ||
      level->memory[y][x] == gent_wall_esw ||
      level->memory[y][x] == gent_wall_cross)
  {
    return true;
  }
  
  return false;
} /* memory_should_connect */



/*
  Same as memory_should_connect(), but with a couple of tweaks.
*/
blean_t believe_should_connect(const level_t * level,
			       const unsigned int y,
			       const unsigned int x)
{
  /*
    This should usually _not_ connect, but without this T-sections
    will appear on the map edge.
  */
  if (on_map(level, y, x) == false)
    return true;
  
  /*  */
/*  if (level->map[y][x] == tile_water ||
      level->map[y][x] == tile_lava ||
      level->map[y][x] == tile_generator ||
      level->map[y][x] == tile_forcefield)
    return false;*/

  /* */
/*  if (level->memory[y][x] == gent_water ||
      level->memory[y][x] == gent_lava ||
      level->memory[y][x] == gent_generator ||
      level->memory[y][x] == gent_forcefield)
      return false;*/

  /*
    We check map for floors since magic mapping will leave us with a
    lot of walls surrounded by gent_blank and these can be floors as
    well as other walls.
  */
  if (memory_should_connect(level, y, x) ||
      (level->memory[y][x] == gent_blank && 
       level->map[y][x] != tile_floor))
  {
    /* || tile_info[level->map[y][x]]->walkable == false */
    return true;
  }
  
  return false;
} /* believe_should_connect */
