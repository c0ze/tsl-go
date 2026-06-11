/*
  level.h - contains definitions for level_t (the maps on which all
  the action takes place) and associated structures, various constants
  and functions for inquiry and modification of levels.

  There are two aspects of level_t - at first, it will just be a
  skeleton to hold parameters for the dungeon generator. No map data
  will be added until we call build_dungeon(level). After that, it's
  the structure that will be used for play.
*/

#ifndef _LEVEL_H_
#define _LEVEL_H_

#include <stdint.h>

#include "main.h"
#include "rules.h"
#include "tiles.h"
#include "content.h"
#include "gent.h"

#define STD_ENEMIES 6

/*
  How many links levels _can_ have, not how many actual stairs they
  should have. Usually uses 3 at most.
*/
#define STAIRS 4

struct level_t
{
  /* What is this level called? */
  char * name;

  unsigned int level_index;

  /* */
  unsigned int floor_type;

  /* This will be printed (once) when entering the level. */
  char * message;

  /* The size of this level; these should never be changed after build_level(). */
  unsigned int size_y;
  unsigned int size_x;

  /* Which level tile_stair* will take us to (-1 for none). */
  signed int link[STAIRS];

  /* The tiles that make up this level. */
  tile_t ** map;

  /*
    What the player knows of the level; this can differ significantly
    from what is really present in map[][].
  */
  gent_t ** memory;

  /* */
  unsigned int respawn_counter;
  unsigned int respawn_speed;

  /* How difficult we want this level to be - see content.*. */
  unsigned wanted_challenge;
  unsigned int water;
  unsigned int lava;
  unsigned int challenge;
  unsigned int supplies;
  unsigned int std_enemy[STD_ENEMIES];
  unsigned int monsters;

  /* The encounter table for this level */
  encounter_t * encounter;
  unsigned int encounters;

  /*
    The first item and trap on this level; the rest (if any) follow as
    a double-linked list. See item.* and trap.*.
  */
  item_t * first_item;
  trap_t * first_trap;

  /*
    CREATURE is a dynamic array of CREATURES creature_t
    pointers. These are the creatures present on this level. Note that
    indices may (and will) be NULL. For complex reasons, we can't make
    these a linked list - the game loop in play() is too fragile so it
    was easier to do it like this instead.
  */
  creature_t ** creature;
  unsigned int creatures;

  /*
    How many attempts the chosen level generator has made at
    generating this level. Ideally, this should never be more than
    1. This is only intended for debugging and tuning level
    generators.
  */
  unsigned int generated;

  /* Has the player visited this level yet? */
  blean_t visited;

  /* Parameters for the dungeon generator. */
  /*
    RFE: These are just for modbuild, make them like an array instead
    generator_parm[] that has different enums for different
    generators.
  */
  uint32_t wanted_modules;
  unsigned int starting_module;
  encounter_t boss;
};



/* (De)allocation */
level_t * alloc_level(const unsigned int new_index, const char * new_name);
void del_level(level_t * level);
void set_level_message(level_t * level, const char * new_msg);

/*
  Dungeon generation - these differ from the "map drawing" ones by
  controlling *how* to build rather than *what* to build...
*/
void clear_level(level_t * level);
void add_stairs(level_t * level);
void build_dungeon(level_t * level);
void add_boss(level_t * level);
void add_pools(level_t * level);

/* These are for fixing some undesired side-effects of other functions */
void remove_dead_ends(level_t * level, const unsigned int max);
void improve_walls(level_t * level);

/* Testing various stuff */
blean_t is_valid_level(level_t * level);
blean_t on_map(const level_t * level,
	       const signed int y,
	       const signed int x);
unsigned int count_adjacent_traversables(const level_t * level,
					 const blean_t memory,
					 const unsigned int y,
					 const unsigned int x);
unsigned int count_adjacent_floors(const level_t * level,
				   const unsigned int y,
				   const unsigned int x);
unsigned int count_adjacent_walls_doors(const level_t * level,
					const unsigned int y,
					const unsigned int x,
					const blean_t diagonals);

void destroy_all_items(level_t * level, const unsigned int y, const unsigned int x);

#endif
