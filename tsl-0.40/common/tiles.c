#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

#include "main.h"
#include "stuff.h"
#include "level.h"
#include "gent.h"
#include "find.h"



/*
  Initialises the tile definitions.
 */
void init_tiles()
{
  tile_t i;
  
  for (i = 0; i < TILES; i++)
    tile_info[i] = NULL;
  
  /* Add tiles */
  i = tile_void;
  tile_info[i] = build_tile("void");
  tile_info[i]->opaque = false;
  tile_info[i]->walkable = true;
  tile_info[i]->gent = gent_blank;

  i = tile_floor;
  tile_info[i] = build_tile("floor");
  tile_info[i]->opaque = false;
  tile_info[i]->walkable = true;
  tile_info[i]->gent = gent_floor;

  i = tile_water;
  tile_info[i] = build_tile("water");
  tile_info[i]->opaque = false;
  tile_info[i]->walkable = false;
  tile_info[i]->gent = gent_water;

  i = tile_lava;
  tile_info[i] = build_tile("lava");
  tile_info[i]->opaque = false;
  tile_info[i]->walkable = false;
  tile_info[i]->gent = gent_lava;

  i = tile_ice;
  tile_info[i] = build_tile("ice");
  tile_info[i]->opaque = false;
  tile_info[i]->walkable = true;
  tile_info[i]->gent = gent_ice;

  i = tile_wall;
  tile_info[i] = build_tile("wall");
  tile_info[i]->opaque = true;
  tile_info[i]->walkable = false;
  tile_info[i]->gent = gent_blank;

  i = tile_wall_v;
  tile_info[i] = build_tile("wall");
  tile_info[i]->opaque = true;
  tile_info[i]->walkable = false;
  tile_info[i]->gent = gent_wall_v;

  i = tile_wall_h;
  tile_info[i] = build_tile("wall");
  tile_info[i]->opaque = true;
  tile_info[i]->walkable = false;
  tile_info[i]->gent = gent_wall_h;

  i = tile_wall_es;
  tile_info[i] = build_tile("wall");
  tile_info[i]->opaque = true;
  tile_info[i]->walkable = false;
  tile_info[i]->gent = gent_wall_es;

  i = tile_wall_sw;
  tile_info[i] = build_tile("wall");
  tile_info[i]->opaque = true;
  tile_info[i]->walkable = false;
  tile_info[i]->gent = gent_wall_sw;

  i = tile_wall_ne;
  tile_info[i] = build_tile("wall");
  tile_info[i]->opaque = true;
  tile_info[i]->walkable = false;
  tile_info[i]->gent = gent_wall_ne;

  i = tile_wall_nw;
  tile_info[i] = build_tile("wall");
  tile_info[i]->opaque = true;
  tile_info[i]->walkable = false;
  tile_info[i]->gent = gent_wall_nw;

  i = tile_wall_esw;
  tile_info[i] = build_tile("wall");
  tile_info[i]->opaque = true;
  tile_info[i]->walkable = false;
  tile_info[i]->gent = gent_wall_esw;

  i = tile_wall_nsw;
  tile_info[i] = build_tile("wall");
  tile_info[i]->opaque = true;
  tile_info[i]->walkable = false;
  tile_info[i]->gent = gent_wall_nsw;

  i = tile_wall_new;
  tile_info[i] = build_tile("wall");
  tile_info[i]->opaque = true;
  tile_info[i]->walkable = false;
  tile_info[i]->gent = gent_wall_new;

  i = tile_wall_nes;
  tile_info[i] = build_tile("wall");
  tile_info[i]->opaque = true;
  tile_info[i]->walkable = false;
  tile_info[i]->gent = gent_wall_nes;

  i = tile_wall_cross;
  tile_info[i] = build_tile("wall");
  tile_info[i]->opaque = true;
  tile_info[i]->walkable = false;
  tile_info[i]->gent = gent_wall_cross;

  i = tile_pentagram;
  tile_info[i] = build_tile("pentagram");
  tile_info[i]->opaque = false;
  tile_info[i]->walkable = true;
  tile_info[i]->gent = gent_pentagram;

  i = tile_block;
  tile_info[i] = build_tile("block");
  tile_info[i]->opaque = true;
  tile_info[i]->walkable = false;
  tile_info[i]->gent = gent_block;

  i = tile_capsule;
  tile_info[i] = build_tile("capsule");
  tile_info[i]->opaque = true;
  tile_info[i]->walkable = false;
  tile_info[i]->draw_floor = true;
  tile_info[i]->gent = gent_capsule;

  i = tile_terminal;
  tile_info[i] = build_tile("terminal");
  tile_info[i]->opaque = true;
  tile_info[i]->walkable = false;
  tile_info[i]->draw_floor = true;
  tile_info[i]->gent = gent_terminal;

  i = tile_forcefield;
  tile_info[i] = build_tile("forcefield");
  tile_info[i]->opaque = false;
  tile_info[i]->walkable = false;
  tile_info[i]->gent = gent_forcefield;

  i = tile_generator;
  tile_info[i] = build_tile("generator");
  tile_info[i]->opaque = false;
  tile_info[i]->walkable = false;
  tile_info[i]->gent = gent_generator;

  i = tile_stair0;
  tile_info[i] = build_tile("stair");
  tile_info[i]->opaque = false;
  tile_info[i]->walkable = true;
  tile_info[i]->gent = gent_stairs;

  i = tile_stair1;
  tile_info[i] = build_tile("stair");
  tile_info[i]->opaque = false;
  tile_info[i]->walkable = true;
  tile_info[i]->gent = gent_stairs;

  i = tile_stair2;
  tile_info[i] = build_tile("stair");
  tile_info[i]->opaque = false;
  tile_info[i]->walkable = true;
  tile_info[i]->gent = gent_stairs;

  i = tile_stair3;
  tile_info[i] = build_tile("stair");
  tile_info[i]->opaque = false;
  tile_info[i]->walkable = true;
  tile_info[i]->gent = gent_stairs;

/*  i = tile_stair_up;
  tile_info[i] = build_tile("stair up");
  tile_info[i]->opaque = false;
  tile_info[i]->walkable = true;
  tile_info[i]->gent = gent_stair_up;

  i = tile_stair_down;
  tile_info[i] = build_tile("stair down");
  tile_info[i]->opaque = false;
  tile_info[i]->walkable = true;
  tile_info[i]->gent = gent_stair_down;

  i = tile_branch_up;
  tile_info[i] = build_tile("stair up");
  tile_info[i]->opaque = false;
  tile_info[i]->walkable = true;
  tile_info[i]->gent = gent_stair_up;

  i = tile_branch_down;
  tile_info[i] = build_tile("stair down");
  tile_info[i]->opaque = false;
  tile_info[i]->walkable = true;
  tile_info[i]->gent = gent_stair_down;*/

  i = tile_door_open;
  tile_info[i] = build_tile("open door");
  tile_info[i]->opaque = false;
  tile_info[i]->walkable = true;
  tile_info[i]->gent = gent_door_open;

  i = tile_door_closed;
  tile_info[i] = build_tile("closed door");
  tile_info[i]->opaque = true;
  tile_info[i]->walkable = false;
  tile_info[i]->gent = gent_door_closed;

  i = tile_door_locked;
  tile_info[i] = build_tile("locked door");
  tile_info[i]->opaque = true;
  tile_info[i]->walkable = false;
  tile_info[i]->gent = gent_door_closed;

  i = tile_door_secret_h;
  tile_info[i] = build_tile("secret door");
  tile_info[i]->opaque = true;
  tile_info[i]->walkable = false;
  tile_info[i]->gent = gent_wall_h;

  i = tile_door_secret_v;
  tile_info[i] = build_tile("secret door");
  tile_info[i]->opaque = true;
  tile_info[i]->walkable = false;
  tile_info[i]->gent = gent_wall_v;

  /* This is only for internal use in "seen" memory */
/*  i = tile_internal_trap;
  tile_info[i] = build_tile("a trap");
  tile_info[i]->opaque = false;
  tile_info[i]->walkable = true;
  tile_info[i]->gent = gent_internal_trap;

  i = tile_internal_medkit;
  tile_info[i] = build_tile("a medkit");
  tile_info[i]->opaque = false;
  tile_info[i]->walkable = true;
  tile_info[i]->gent = gent_medkit;*/

  /* This is only for internal use in "seen" memory */
/*  i = tile_internal_blank;
  tile_info[i] = build_tile("nothing");
  tile_info[i]->opaque = false;
  tile_info[i]->walkable = true;
  tile_info[i]->gent = gent_blank;

  i = tile_internal_obstacle;
  tile_info[i] = build_tile("nothing");
  tile_info[i]->opaque = false;
  tile_info[i]->walkable = true;
  tile_info[i]->gent = gent_obstacle;*/

  /*
    Tiles of these type should never appear in-game. They're just for
    distinguishing certain tiles from others during level generaton.
  */
  i = tile_internal_dummy;
  tile_info[i] = build_tile("BUG");
  tile_info[i]->opaque = false;
  tile_info[i]->walkable = false;
  tile_info[i]->gent = gent_undefined;

  i = tile_internal_con_n;
  tile_info[i] = build_tile("BUG");
  tile_info[i]->opaque = false;
  tile_info[i]->walkable = false;
  tile_info[i]->gent = gent_undefined;

  i = tile_internal_con_e;
  tile_info[i] = build_tile("BUG");
  tile_info[i]->opaque = false;
  tile_info[i]->walkable = false;
  tile_info[i]->gent = gent_undefined;

  i = tile_internal_con_s;
  tile_info[i] = build_tile("BUG");
  tile_info[i]->opaque = false;
  tile_info[i]->walkable = false;
  tile_info[i]->gent = gent_undefined;

  i = tile_internal_con_w;
  tile_info[i] = build_tile("BUG");
  tile_info[i]->opaque = false;
  tile_info[i]->walkable = false;
  tile_info[i]->gent = gent_undefined;

  i = tile_internal_start;
  tile_info[i] = build_tile("BUG");
  tile_info[i]->opaque = false;
  tile_info[i]->walkable = false;
  tile_info[i]->gent = gent_undefined;

  i = tile_internal_reserved;
  tile_info[i] = build_tile("BUG");
  tile_info[i]->opaque = false;
  tile_info[i]->walkable = false;
  tile_info[i]->gent = gent_undefined;

  i = tile_internal_sweet;
  tile_info[i] = build_tile("BUG");
  tile_info[i]->opaque = false;
  tile_info[i]->walkable = true;
  tile_info[i]->gent = gent_obstacle;

  i = tile_internal_boss;
  tile_info[i] = build_tile("BUG");
  tile_info[i]->opaque = false;
  tile_info[i]->walkable = false;
  tile_info[i]->gent = gent_undefined;

  return;
} /* init_tiles */



/*
  Frees all tile definitions.
 */
void del_tiles()
{
  int i;
  
  for (i = 0; i < TILES; i++)
  {
    if (tile_info[i] != NULL)
    {
      free(tile_info[i]->name);
      free(tile_info[i]);
      tile_info[i] = NULL;
      
      mem_alloc.tiles--;
    }
  }
  
  return;
} /* del_tiles */



/*
  Builds a new tile_t called NEW_NAME. If sufficient memory couldn't
  be allocated for the structure, out_of_memory() will be called and
  we will DIE.
 */
tile_info_t * build_tile(const char * new_name)
{
  tile_info_t * local;
  
  if (new_name == NULL)
    return NULL;
  
  local = malloc(sizeof(tile_info_t));
  if (local == NULL) out_of_memory();
  mem_alloc.tiles++;
  
  local->name = mydup(new_name);

  local->gent = gent_undefined;
  
  if (local->name == NULL)
  {
    free(local->name);
    free(local);
    out_of_memory();
  }

  local->draw_floor = false;
  
  return local;
} /* build_tile */


/*
  Returns the tile at {Y, X} on LEVEL.
 */
tile_t get_tile(level_t * level,
		const unsigned int y,
		const unsigned int x)
{
  if (on_map(level, y, x))
    return level->map[y][x];
  else
    return tile_void;
} /* get_tile*/



/*
  Sets the tile at {Y, X} on LEVEL to TILE.
*/
void set_tile(level_t * level,
	      const unsigned int y,
	      const unsigned int x,
	      const tile_t tile)
{
  if (on_map(level, y, x))
  {
    level->map[y][x] = tile;
  }
  
  return;
} /* set_tile */






/*
  Replaces all FIND tiles on LEVEL with REPLACE.
*/
void replace_tile(level_t * level,
		  const tile_t find,
		  const tile_t replace)
{
  unsigned int y;
  unsigned int x;

  if (level == NULL)
    return;

  for (y = 0; y < level->size_y; y++)
    for (x = 0; x < level->size_x; x++)
      if (level->map[y][x] == find)
	level->map[y][x] = replace;
      
  return;
} /* replace_tile */



/*
  Returns true if the tile at {Y, X} on LEVEL is a wall, otherwise
  false.
*/
blean_t is_wall(const level_t * level,
		const unsigned int y,
		const unsigned int x)
{
  tile_t tile;

  if (on_map(level, y, x) == false)
    return false;

  tile = level->map[y][x];

  switch (tile)
  {
    case tile_wall:
    case tile_wall_v:
    case tile_wall_h:
    case tile_wall_es:
    case tile_wall_sw:
    case tile_wall_ne:
    case tile_wall_nw:
    case tile_wall_nes:
    case tile_wall_nsw:
    case tile_wall_new:
    case tile_wall_esw:
    case tile_wall_cross:
    case tile_door_secret_v:
    case tile_door_secret_h:
      return true;

    default:
      return false;
  }
} /* is_wall */



blean_t is_weak_wall(const level_t * level, const int y, const int x)
{
  if (on_map(level, y, x) == false)
    return false;

  if (wall_or_door(level, y, x) == false)
    return false;

  if ((is_flyable(level, y - 1, x) && is_flyable(level, y + 1, x)) ||
      (is_flyable(level, y, x - 1) && is_flyable(level, y, x + 1)))
    return true;
    
  return false;
} /* is_weak_wall */



/*
  Returns true if the tile at {Y, X} on LEVEL is some kind of door,
  false if it is not.
*/
blean_t is_door(const level_t * level,
		const unsigned int y,
		const unsigned int x)
{
  tile_t tile;

  if (on_map(level, y, x) == false)
    return false;

  tile = level->map[y][x];

  switch (tile)
  {
    case tile_door_closed:
    case tile_door_open:
    case tile_door_secret_v:
    case tile_door_secret_h:
      return true;

    default:
      return false;
  }
} /* is_door */



/*
  Returns true if {Y, X} on LEVEL is walkable, otherwise false. A
  square is walkable if

  1) the tile is walkable, i.e. not a wall or similar
  2) no creature occupies the square

  If MEMORY is true we will check level memory instead of map data. In
  this mode no check will be done for creatures.
*/
blean_t is_walkable(const level_t * level,
		    const blean_t memory,
		    const unsigned int y,
		    const unsigned int x)
{
  if (on_map(level, y, x) == false)
    return false;

  if (memory)
  {
    if (tile_info[level->memory[y][x]]->walkable == false)
      return false;
  }
  else
  {   
    if (tile_info[level->map[y][x]]->walkable == false)
      return false;
    
    if (find_creature(level, y, x) != NULL)
      return false;
  }

  return true;
} /* is_walkable */



/*
  Returns true if {Y, X} on LEVEL is traversable, otherwise false. A
  square is traversable if 1) it is walkable (see is_walkable()), or
  2) it is traversable by other means, e.g. player action such as
  swimming (water), "force-walking" (lava), disabling forcefields or
  opening doors.

  If MEMORY is true we will check level memory instead of map data.
*/
blean_t is_traversable(const level_t * level, const blean_t memory,
		       const unsigned int y, const unsigned int x)
{
  tile_t ** map;

  if (on_map(level, y, x) == false)
    return false;
  
/*  if (memory)
    map = level->memory;
    else*/
/*  {*/
    map = level->map;

    if (find_creature(level, y, x) != NULL)
      return false;
/*  }*/
    
  if (is_walkable(level, memory, y, x) ||
      map[y][x] == tile_lava ||
      map[y][x] == tile_forcefield ||
      map[y][x] == tile_generator ||
      map[y][x] == tile_water ||
      map[y][x] == tile_door_closed ||
      map[y][x] == tile_door_locked ||
      map[y][x] == tile_door_secret_v ||
      map[y][x] == tile_door_secret_h ||
      map[y][x] == tile_internal_start ||
      map[y][x] == tile_internal_reserved)
  {
    return true;
  }

  return false;
} /* is_traversable */



/*
  Returns true if {Y, X} on LEVEL is (safely) traversable by air.
*/
blean_t is_flyable(const level_t * level,
		   const unsigned int y, const unsigned int x)
{
  if (on_map(level, y, x) == false)
    return false;
  
  if (find_creature(level, y, x) != NULL)
    return false;

  if (is_walkable(level, false, y, x) ||
      level->map[y][x] == tile_lava ||
      level->map[y][x] == tile_water)
  {
    return true;
  }

  return false;
} /* is_flyable */



/*
  Returns true if the tile at {Y, X} on LEVEL "looks like" a wall to
  improve_walls(), i.e. walls surrounding doorways should appears
  connected to the door and not "rounded" to a connecting corridor.
*/
/*
  RFE: This could be merged with memory_should_connect().
*/
blean_t wall_or_door(const level_t * level,
			const unsigned int y,
			const unsigned int x)
{
  if (on_map(level, y, x) == false)
    return false;

  /* Why are open doors skipped here? I don't remember. */
  if (is_wall(level, y, x) ||
      (level->map[y][x] == tile_door_secret_v) ||
      (level->map[y][x] == tile_door_secret_h) ||
      (level->map[y][x] == tile_door_locked) ||
      (level->map[y][x] == tile_door_closed))
  {
    return true;
  }

  return false;
} /* wall_or_door */



/*
  Returns true if there is one or more TILE tiles on LEVEL.
*/
blean_t has_tile(const level_t * level, const tile_t tile)
{
  unsigned int y;
  unsigned int x;

  if (level == NULL)
    return false;

  for (y = 0; y < level->size_y; y++)
  {
    for (x = 0; x < level->size_x; x++)
    {
      if (level->map[y][x] == tile)
	return true;
    }
  }
      
  return false;
} /* has_tile */



tile_t get_wall_tile(const unsigned int mask)
{
  tile_t tile;

  switch (mask)
  {
    case 1:  tile = tile_wall_v;         break;
    case 2:  tile = tile_wall_h;         break;
    case 3:  tile = tile_wall_nw;        break;
    case 4:  tile = tile_wall_v;         break;
    case 5:  tile = tile_wall_v;         break;
    case 6:  tile = tile_wall_sw;        break;
    case 7:  tile = tile_wall_nsw;       break;
    case 8:  tile = tile_wall_h;         break;
    case 9:  tile = tile_wall_ne;        break;
    case 10: tile = tile_wall_h;         break;
    case 11: tile = tile_wall_new;       break;
    case 12: tile = tile_wall_es;        break;
    case 13: tile = tile_wall_nes;       break;
    case 14: tile = tile_wall_esw;       break;
    case 15: tile = tile_wall_cross;     break;
    default: tile = tile_floor;          break;
  }
  
  return tile;
} /* get_wall_tile */
