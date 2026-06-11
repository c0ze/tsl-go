/*
  tiles.h
*/

#ifndef _TILES_H_
#define _TILES_H_

#include "gent.h"

enum tile_t
{
  tile_void,
  tile_floor,
  tile_wall,
  tile_wall_v,
  tile_wall_h,
  tile_wall_es,
  tile_wall_sw,
  tile_wall_ne,
  tile_wall_nw,
  tile_wall_nes,
  tile_wall_nsw,
  tile_wall_new,
  tile_wall_esw,
  tile_wall_cross,
  tile_water,
  tile_block,
  tile_ice,
  tile_lava,
  tile_stair0,
  tile_stair1,
  tile_stair2,
  tile_stair3,
/*  tile_stair_down,
  tile_stair_up,
  tile_branch_down,
  tile_branch_up,*/
  tile_pentagram,
  tile_capsule,
  tile_terminal,
  tile_forcefield,
  tile_generator,
  tile_door_open,
  tile_door_closed,
  tile_door_locked,
  tile_door_secret_h,
  tile_door_secret_v,
/*  tile_internal_trap,
  tile_internal_medkit,
  tile_internal_blank,
  tile_internal_obstacle,*/
  tile_internal_dummy,
  tile_internal_con_n,
  tile_internal_con_e,
  tile_internal_con_s,
  tile_internal_con_w,
  tile_internal_start,
  tile_internal_reserved,
  tile_internal_sweet,
  tile_internal_boss,
  TILES
};
typedef enum tile_t tile_t;

struct tile_info_t
{
  gent_t gent;
  blean_t walkable;
  blean_t opaque;
  blean_t draw_floor;
  char * name;
};

/* Contains definition of the tiles */
tile_info_t * tile_info[TILES];

/* Tile-related */
void init_tiles(void);
void del_tiles(void);
tile_info_t * build_tile(const char * new_name);

void set_tile(level_t * level,
	      const unsigned int y, const unsigned int x,
	      const tile_t tile);
tile_t get_tile(level_t * level,
		const unsigned int y, const unsigned int x);
void replace_tile(level_t * level,
		  const tile_t find,
		  const tile_t replace);

blean_t is_walkable(const level_t * level, const blean_t memory,
		    const unsigned int y, const unsigned int x);
blean_t is_traversable(const level_t * level, const blean_t memory,
		       const unsigned int y, const unsigned int x);
blean_t is_flyable(const level_t * level,
		   const unsigned int y, const unsigned int x);
blean_t is_wall(const level_t * level,
		const unsigned int y, const unsigned int x);
blean_t is_door(const level_t * level,
		const unsigned int y, const unsigned int x);
blean_t wall_or_door(const level_t * level,
		     const unsigned int y, const unsigned int x);
blean_t has_tile(const level_t * level, const tile_t tile);

blean_t is_weak_wall(const level_t * level, const int y, const int x);

tile_t get_wall_tile(const unsigned int mask);

#endif
