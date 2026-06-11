#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

#include "main.h"
#include "stuff.h"
#include "level.h"
#include "item.h"
#include "monster.h"
#include "treasure.h"
#include "rules.h"
#include "traps.h"
/*#include "places.h"*/
#include "actions.h"
#include "dungeon.h"
#include "game.h"
#include "modbuild.h"
#include "find.h"
#include "ffield.h"
#include "find.h"
#include "doors.h"



/*
  Allocates and initialises a new level_t structure. If enough memory
  isn't available, out_of_memory() will be called. No map data will be
  generated at this point.
*/
level_t * alloc_level(const unsigned int new_index, const char * new_name)
{
  level_t * local;
  int i;

  local = malloc(sizeof(level_t));
  if (local == NULL) out_of_memory();
  mem_alloc.levels++;

  if (new_name == NULL)
    return NULL;

  /* */
  local->level_index = new_index;

  /* Set the level name */
  local->name = mydup(new_name);

  if (local->name == NULL)
    out_of_memory();

  mem_alloc.chars += strlen(local->name) + 1;  

  /* Set some defaults */
  local->wanted_modules = MODULE_ROOM;
  
  for (i = 0; i < STAIRS; i++)
    local->link[i] = -1;

  local->size_y = 80;
  local->size_x = 60;

  local->visited = false;

  local->encounters = 0;
  local->encounter = NULL;

  local->starting_module = CHAMBER_START;
  local->boss = encounter_undefined;

  /*
    Set up an empty creature array - we will adjust the size of this
    later as needed.
  */
  local->creatures = 1;
  local->creature = malloc(sizeof(creature_t *) * local->creatures);
  if (local->creature == NULL) out_of_memory();
  
  for (i = 0; i < local->creatures; i++)
    local->creature[i] = NULL;

  /* None of these exist yet */
  local->first_item = NULL;
  local->first_trap = NULL;

  /* Check if this level should have a welcome message. */
  local->message = NULL;
/*  set_level_message(local, level_message(location_index, depth));*/

  local->respawn_counter = 0;
  local->respawn_speed = RESPAWN_SPEED;

  local->challenge = 0;
  local->wanted_challenge = 0;
  local->water = 0;
  local->lava = 0;
  local->supplies = 0;
  local->monsters = 0;

  for (i = 0; i < STD_ENEMIES; i++)
    local->std_enemy[i] = monster_undefined;

  /* DON'T allocate map */
  local->map = NULL;
  local->memory = NULL;

  return local;
} /* alloc_level */



/*
  Frees LEVEL and everything on it.
 */
void del_level(level_t * level)
{
  int i;

  if (level == NULL)
    return;

  if (level->name != NULL)
  {
    mem_alloc.chars -= strlen(level->name) + 1;
    free(level->name);
  }

  if (level->encounter != NULL)
  {
    free(level->encounter);
  }

  /* Remove the map data */
  if (level->map != NULL)
  {
    for (i = 0; i < level->size_y; i++)
    {
      free(level->map[i]);
      free(level->memory[i]);
    }
    
    free(level->map);
    free(level->memory);
  }
  
  /* Clear any level message */
  set_level_message(level, NULL);

  /* Delete all creatures on the level. */
  for (i = 0; i < level->creatures; i++)
  {
    if (level->creature[i] != NULL)
    {
      del_creature(level->creature[i]);
      level->creature[i] = NULL;
    }
  }

  /* Delete every item on the level. */
  while(level->first_item != NULL)
  {
    del_item(detach_item(level->first_item));
  }

  /* Delete every trap on the level. */
  while(level->first_trap != NULL)
  {
    del_trap(detach_trap(level->first_trap));
  }

  free(level);

  mem_alloc.levels--;

  return;
} /* del_level */



/*
  Sets NEW_MSG as the message to be displayed upon entering LEVEL. Any
  previous message will be cleared.
*/
void set_level_message(level_t * level, const char * new_msg)
{
  if (level == NULL)
    return;

  /* If we have a message already, clear it */
  if (level->message != NULL)
  {
    mem_alloc.chars -= strlen(level->message) + 1;
    free(level->message);
  }

  if (new_msg == NULL)
  {
    level->message = NULL;
  }
  else
  {
    level->message = mydup(new_msg);

    if (level->message == NULL)
      out_of_memory();

    mem_alloc.chars += strlen(level->message) + 1;
  }

  return;
} /* set_level_message */



/*
  Removes dead ends from LEVEL. This function will go over the map at
  most MAX times, removing any corridor tiles that are adjacent
  (diagonals don't count) to less than two other traversable tiles.

  This should be done before any doors are added to the level.
*/
void remove_dead_ends(level_t * level, const unsigned int max)
{
  int y;
  int x;
  int score;
  int finished;
  int count;

  if (level == NULL)
    return;

  if (max == 0)
    return;

  count = 0;

  /*
    Keep doing this until FINISHED is left unaltered at the end of
    the loop, meaning no tile has been changed (no more dead ends) or
    COUNT reaches MAX iterations.
  */
  do
  {
    finished = 1;
    
    for (y = 0; y < level->size_y; y++)
    {
      for (x = 0; x < level->size_x; x++)
      {
	if (is_traversable(level, false, y, x) == false ||
	    get_tile(level, y, x) == tile_generator ||
	    get_tile(level, y, x) == tile_forcefield)
	  continue;

	score = 0;
	
	/*
	  Check how many of this tile neighbours are traversable. We
	  can't use count_adjacent_traversables() for this since it
	  also counts diagonals.
	*/
	if (is_traversable(level, false, y - 1, x))
	  score++;
	
	if (is_traversable(level, false, y + 1, x))
	  score++;
	
	if (is_traversable(level, false, y, x + 1))
	  score++;
	
	if (is_traversable(level, false, y, x - 1))
	  score++;
	
	if (score < 2)
	{
	  /* This is a dead end. */
	  finished = 0;
	  set_tile(level, y, x, tile_wall);
	}
      }
    }
    
    count++;
    
    /* Are we finished? */
    if (count == max)
      return;
  } while (finished == 0);
  
  return;
} /* remove_dead_ends */



/*
  Builds a dungeon of the appropriate type on LEVEL. The level
  generated will be valid for play (according to is_valid_level()).
*/
void build_dungeon(level_t * level)
{
  unsigned int i;

  if (level == NULL)
    return;

  /* This will only be performed once for each level */
  map_test.calls++;
  
  /* Allocate map */
  if (level->map == NULL)
  {
    level->map = malloc(sizeof(tile_t *) * level->size_y);
    if (level->map == NULL) out_of_memory();
    level->memory = malloc(sizeof(gent_t *) * level->size_y);
    if (level->memory == NULL) out_of_memory();
    
    for (i = 0; i < level->size_y; i++)
    {
      level->map[i] = malloc(sizeof(tile_t *) * level->size_x);
      if (level->map[i] == NULL) out_of_memory();
      level->memory[i] = malloc(sizeof(gent_t *) * level->size_x);
      if (level->memory[i] == NULL) out_of_memory();
    }
  }
  
  /*
    Zero the local "generated levels" counter. This gives us some
    statistics on how efficient our dungeon generation routines
    are.
  */
/*  level->generated = 0;*/
  
  /* This will be repeated until we get a valid level... */
  do
  {
    /* Increment the internal "levels generated" counter */
    level->generated++;
    
    /* Increment the global map test statistics as well */
    map_test.generated++;
    
    /* Clear the level */
    clear_level(level);
    
    /* Try to build a dungeon  */
    if (level->level_index == LEVEL_CHAPEL)
      build_chapel(level);
    else
      build_module_dungeon(level);

    if (level->level_index != LEVEL_CATACOMBS)
      remove_dead_ends(level, 90);

    add_stairs(level);
    add_pools(level);
    
    /* This one doesn't need to validate. */
    if (level->level_index == LEVEL_TEST)
      break;
  } while (is_valid_level(level) == false);

  replace_doors(level);
  improve_walls(level);

  /*
    These will be generated once we enter the level, but we add them
    already so content doesn't get generated in their path.
  */
  build_forcefields(level);

  add_boss(level);
  add_content(level);

  replace_tile(level, tile_internal_reserved, tile_floor);
  replace_tile(level, tile_internal_sweet, tile_floor);

  return;
} /* build_dungeon */



void add_boss(level_t * level)
{
  int y;
  int x;

  if (level == NULL)
    return;

  if (level->boss != encounter_undefined)
  {
    /* We need a boss start tile to place a boss. */
    if (find_spot(level, &y, &x, find_boss) == false)
      return;

    /*
      We'll clear the floor before we place the boss. Those bosses
      that need it set to something else (The Lurker) will do this in
      their own encounter code.

      DO NOT find_spot for find_boss, since it doesn't exist anymore! (I did.)
    */
    set_tile(level, y, x, tile_floor);

    add_encounter(level, y, x, level->boss);
  }
  
  return;
} /* add_boss */



/*
  Adds pools of water and lava to LEVEL.
*/
void add_pools(level_t * level)
{
  while (level->water > 0)
  {
    level->water--;
    add_encounter(level, 0, 0, encounter_water);
  }

  while (level->lava > 0)
  {
    level->lava--;
    add_encounter(level, 0, 0, encounter_lava);
  }

  return;
} /* add_water */



/*
  Checks a number of conditions for LEVEL. If all are met, the level
  is valid for play and true is returned. If the level is invalid
  (erroneous, unsolvable or somehow unsuitable for play), false is
  returned.
  
  If a level is invalid, the reason will be recorded to the global
  map_test structure (see main.h) for statistics.
*/
blean_t is_valid_level(level_t * level)
{
  blean_t ** connected;
  int y;
  int x;
  int i;
  int start_y;
  int start_x;
  blean_t finished;

  blean_t stair[STAIRS];

  int traversable_tiles;
  int connected_tiles;

  if (level == NULL)
    return false;
  
  /* Count traversable tiles */
  traversable_tiles = 0;
  
  for (y = 0; y < level->size_y; y++)
  {
    for (x = 0; x < level->size_x; x++)
    {
      if (is_traversable(level, false, y, x))
      {
	traversable_tiles++;
      }
    }
  }
  
  /* Do we have enough traversable space? */
  if (traversable_tiles < level->size_y * level->size_x * 0.07)
  {
    /* We don't. */
    /* TODO: Re-enable this */
/*    map_test.traversable++;
      return false;*/
  }
  
  /*
    We want to check that there are no disjoint areas. We will use a
    floodfill algorithm to count the number of tiles we can reach,
    and compare it to the number of tiles that *should* be walkable
    (naturally, if they differ there is some area that can't be
    reached from anywhere else).
  */

  /* Find a starting point. */
  if (find_spot(level, &start_y, &start_x, find_traversable) == false)
  {
    /* We couldn't find any starting point; this map is invalid. */
    map_test.connected++;
    return false;
  }
 
  /* Create a temporary mask of the level */
  connected = malloc(sizeof(blean_t *) * level->size_y);

  if (connected == NULL)
    out_of_memory();

  for (y = 0; y < level->size_y; y++)
  {
    connected[y] = malloc(sizeof(blean_t) * level->size_x);

    if (connected[y] == NULL)
      out_of_memory();

    for (x = 0; x < level->size_x; x++)
      connected[y][x] = false;
  }

  /*
    Floodfill the connected matrix and check that there are no
    disjoint areas.
  */
   connected[start_y][start_x] = true;

  /*
    RFE: Maybe this should instead just check if all stairs and the
    start location are connected. There could be sealed off rooms.
  */
  /*
    Keep looping until all reachable tiles have been detected,
    i.e. finished is left unaltered at the end of the loop.
  */
  do
  {
    finished = true;
    
    for (y = 0; y < level->size_y; y++)
    {
      for (x = 0; x < level->size_x; x++)
      {
	if (is_traversable(level, false, y, x) == false)
	  continue;
	
	/*
	  Skip tiles that are already connected; these will just keep
	  resetting FINISHED and hang the function.
	*/
	if (connected[y][x] == true)
	  continue;

	if ((on_map(level, y - 1, x + 1) && (connected[y - 1][x + 1])) ||
	    (on_map(level, y - 1, x    ) && (connected[y - 1][x    ])) ||
	    (on_map(level, y - 1, x - 1) && (connected[y - 1][x - 1])) ||
	    (on_map(level, y,     x - 1) && (connected[y    ][x - 1])) ||
	    (on_map(level, y + 1, x - 1) && (connected[y + 1][x - 1])) ||
	    (on_map(level, y + 1, x    ) && (connected[y + 1][x    ])) ||
	    (on_map(level, y + 1, x + 1) && (connected[y + 1][x + 1])) ||
	    (on_map(level, y,     x + 1) && (connected[y    ][x + 1])))
	{
	  /*
	    This square is walkable AND adjacent to an
	    already connected square, so this one must be
	    connected too.
	  */
	  connected[y][x] = true;
	  finished = false;
	}
      }
    }
  } while (finished == false);
  
  /* Count connected tiles */
  connected_tiles = 0;
  
  for (y = 0; y < level->size_y; y++)
    for (x = 0; x < level->size_x; x++)
      if (connected[y][x])
	connected_tiles++;

  for (y = 0; y < level->size_y; y++)
  {
    free(connected[y]);
  }

  free(connected);
  connected = NULL;

  /* If they differ, something strange is going on... */
  if (connected_tiles != traversable_tiles)
  {
    map_test.connected++;
    return false;
  }
  
  /* Check that we have exits */
  for (i = 0; i < STAIRS; i++)
    stair[i] = false;

  /*
    This is hardly readable but I removed the brackets just to make it
    look EXTREME. Scan the level and check what kinds of stairs it has.
  */
  for (y = 0; y < level->size_y; y++)
    for (x = 0; x < level->size_x; x++)
      for (i = 0; i < STAIRS; i++)
	if (level->map[y][x] == tile_stair0 + i)
	  stair[i] = true;
  
  /* Check that all the stairs we found really have links set. */
  for (i = 0; i < STAIRS; i++)
  {
    if (stair[i] == false && level->link[i] != -1)
    {
      map_test.exits++;
      return false;
    }
  }
  
  /* We haven't found any reason to invalidate this map, so... */
  return true;
} /* is_valid_level */



/*
  Adds stairs to LEVEL. If LEVEL is at the top of the location without
  link_up, no up stair is added, if it's at the bottom without
  link_down no down stair is added.
*/
void add_stairs(level_t * level)
{
  int y;
  int x;
  int i;

  if (level == NULL)
    return;

  for (i = 0; i < STAIRS; i++)
  {
    if (has_tile(level, tile_stair0 + i) == false && (level->link[i] != -1))
    {
      if (find_spot(level, &y, &x, find_sweet) == false)
	if (find_spot(level, &y, &x, find_3x3) == false)
	  find_spot(level, &y, &x, find_unoccupied);
      
      set_tile(level, y, x, tile_stair0 + i);
    }
  }

  return;
} /* add_stairs */



/*
  Returns true if {Y, X} is within the map boundaries, otherwise
  false.
*/
blean_t on_map(const level_t * level,
	       const signed int y,
	       const signed int x)
{
  if (level == NULL)
    return false;

  if ((y >= 0) && (y < level->size_y) &&
      (x >= 0) && (x < level->size_x))
  {
    return true;
  }
  else
  {
    return false;
  }
} /* on_map */



/*
  Resets a level completely (except for the dungeon generation
  statistics, as we want to preserve these).
 */
void clear_level(level_t * level)
{
  unsigned int y;
  unsigned int x;

  unsigned int i;

  if (level == NULL)
    return;

  for (y = 0; y < level->size_y; y++)
  {
    for (x = 0; x < level->size_x; x++)
    {
      level->map[y][x] = tile_wall;
      level->memory[y][x] = gent_blank;
    }
  }
  
  /* Delete all creatures */
  for (i = 0; i < level->creatures; i++)
  {
    del_creature(level->creature[i]);
    level->creature[i] = NULL;
  }
  
  /* Delete all items */
  while (level->first_item)
  {
    del_item(detach_item(level->first_item));
  }
  
  /* Delete all traps */
  while (level->first_trap)
  {
    del_trap(level->first_trap);
  }
  
  level->challenge = 0;
  
  return;
} /* clear_level */



/*
  Counts the number of tiles around {Y, X} on LEVEL that are
  traversable (i.e. walkable, or accessible by other means).

  If MEMORY is true we will use level memory instead of map.

  See also is_traversable().
*/
unsigned int count_adjacent_traversables(const level_t * level,
					 const blean_t memory,
					 const unsigned int y,
					 const unsigned int x)
{
  unsigned int traversables;

  if (level == NULL)
    return 0;

  traversables = 0;

  if (is_traversable(level, memory, y - 1, x - 1) == true)
    traversables++;

  if (is_traversable(level, memory, y - 1, x    ) == true)
    traversables++;

  if (is_traversable(level, memory, y - 1, x + 1) == true)
    traversables++;

  if (is_traversable(level, memory, y    , x + 1) == true)
    traversables++;
  
  if (is_traversable(level, memory, y + 1, x + 1) == true)
    traversables++;
  
  if (is_traversable(level, memory, y + 1, x    ) == true)
    traversables++;

  if (is_traversable(level, memory, y + 1, x - 1) == true)
    traversables++;

  if (is_traversable(level, memory, y    , x - 1) == true)
    traversables++;

  return traversables;
} /* count_adjacent_traversables */



/*
  Counts the number of floor tiles around {Y, X} on LEVEL. See also
  count_adjacent_traversables().
*/
unsigned int count_adjacent_floors(const level_t * level,
				   const unsigned int y,
				   const unsigned int x)
{
  unsigned int floors;
  
  if (level == NULL)
    return 0;
  
  floors = 0;

  if (on_map(level, y - 1, x - 1))
    if (level->map[y - 1][x - 1] == tile_floor)
      floors++;

  if (on_map(level, y - 1, x))
    if (level->map[y - 1][x] == tile_floor)
      floors++;

  if (on_map(level, y - 1, x + 1))
    if (level->map[y - 1][x + 1] == tile_floor)
      floors++;

  if (on_map(level, y, x + 1))
    if (level->map[y][x + 1] == tile_floor)
      floors++;

  if (on_map(level, y + 1, x + 1))
    if (level->map[y + 1][x + 1] == tile_floor)
      floors++;

  if (on_map(level, y + 1, x))
    if (level->map[y + 1][x] == tile_floor)
      floors++;

  if (on_map(level, y + 1, x - 1))
    if (level->map[y + 1][x - 1] == tile_floor)
      floors++;

  if (on_map(level, y, x - 1))
    if (level->map[y][x - 1] == tile_floor)
      floors++;

  return floors;
} /* count_adjacent_floors */



/*
  Counts the number of wall and door tiles around {Y, X} on LEVEL.
*/
unsigned int count_adjacent_walls_doors(const level_t * level,
					const unsigned int y,
					const unsigned int x,
					const blean_t diagonals)
{
  unsigned int count;
  
  if (level == NULL)
    return 0;
  
  count = 0;

  if (wall_or_door(level, y - 1, x))
    count++;

  if (diagonals &&
      wall_or_door(level, y - 1, x + 1))
    count++;

  if (wall_or_door(level, y, x + 1))
    count++;

  if (diagonals &&
      wall_or_door(level, y + 1, x + 1))
    count++;

  if (wall_or_door(level, y + 1, x))
    count++;

  if (diagonals &&
      wall_or_door(level, y + 1, x - 1))
    count++;

  if (wall_or_door(level, y, x - 1))
    count++;

  if (diagonals &&
      wall_or_door(level, y - 1, x - 1))
    count++;

  return count;
} /* count_adjacent_walls_doors */



/*
  Changes all-black walls into pretty lines if they adjacent to
  walkable tiles.
*/
void improve_walls(level_t * level)
{
  unsigned int mask;
  unsigned int y;
  unsigned int x;
  tile_t tile;

  if (level == NULL)
    return;

  for (y = 0; y < level->size_y; y++)
  {
    for (x = 0; x < level->size_x; x++)
    {
      /*
	Skip non-wall special tiles and secret doors, which we
	shouldn't change the appearance of.
      */
      if (is_wall(level, y, x) == false ||
	  level->map[y][x] == tile_door_secret_v ||
	  level->map[y][x] == tile_door_secret_h)
      {
	continue;
      }
      
      /*
	If it's completely isolated (i.e. the player will never see
        it), skip it.
      */
      if (count_adjacent_traversables(level, false, y, x) == 0)
      {
	continue;
      }

      mask = 0;
      
      if (wall_or_door(level, y - 1, x))
	mask |= 1;

      if (wall_or_door(level, y, x - 1))
	mask |= 2;

      if (wall_or_door(level, y + 1, x))
	mask |= 4;

      if (wall_or_door(level, y, x + 1))
	mask |= 8;

      tile = get_wall_tile(mask);

      set_tile(level, y, x, tile);
    }
  }

  /* Remove doors without walls on both sides */
  for (y = 0; y < level->size_y; y++)
  {
    for (x = 0; x < level->size_x; x++)
    {
      if (is_door(level, y, x))
      {
	if ((is_wall(level, y - 1, x) == true  && is_wall(level, y + 1, x) == false) ||
	    (is_wall(level, y - 1, x) == false && is_wall(level, y + 1, x) == true ) ||
	    (is_wall(level, y, x - 1) == true && is_wall(level, y, x + 1) == false) ||
	    (is_wall(level, y, x - 1) == false && is_wall(level, y, x + 1) == true) )
	{
	  set_tile(level, y, x, tile_floor);
	  continue;
	}
      }
    }
  }

  return;
} /* improve_walls */




/*
  Destroys all items at Y, X on LEVEL.
*/
void destroy_all_items(level_t * level, const unsigned int y, const unsigned int x)
{
  unsigned int i;
  item_t ** stuff;
  unsigned int items;

  if (level == NULL)
    return;

  items = find_items(level, y, x, &stuff);

  if (items)
  {
    for (i = 0; i < items; i++)
    {
      del_item(stuff[i]);
    }

    free(stuff);
  }

  return;
} /* destroy_all_items */
