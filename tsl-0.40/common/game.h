/*
  game.h - the main game loop and other general-purpose things.

  Note: Player-specific code should be in player.* or one of its
  relatives.
*/

#ifndef _GAME_H_
#define _GAME_H_

#include <time.h>

#include "main.h"
#include "creature.h"
#include "level.h"
#include "places.h"
#include "item.h"
#include "treasure.h"
#include "facet.h"

struct game_t
{
  signed int ability_slot[10]; /* see ability.h */

  level_t * level_list[LEVELS];

  item_t * item_template[ITEMS];

  /* Facets currently available to the player, to be selected at random */
  facet_t available_facets[FACET_SLOTS];

  /* Facets that apply to the player */
  item_t * first_facet;

  unsigned long turns;
  time_t started;
  time_t died;
  blean_t game_over;
  blean_t won;
  blean_t item_known[ITEMS];
  blean_t monster_known[monsters];
  creature_t * native;
  unsigned int template;

  treasure_t uniqitem[UNIQITEM_SETS][UNIQITEM_SET_SIZE];

  /*
    Used for the mark/recall spells. recall_location is signed so it
    can be set to -1 - no location set.
  */
  signed int recall_location; 
  unsigned int recall_y;
  unsigned int recall_x;

  /* These should be accessible from anywhere */
  creature_t * player;
};

game_t * game;


/* What tick we're at; this is just for timing purposes, so it doesn't matter if it overflows. */
unsigned long tick;

/* Set to true when level changes; this should ensure that the creature list doesn't break. */
extern blean_t level_changed;

/* Main game */
void play(void);
level_t * get_current_level(void);

/* Travel */
void change_level(const unsigned int level_index);

/* Stuff that is done at every tick or turn */
void try_to_detect_enemies(void);
void try_to_detect_traps(void);
void pass_time_on_effects(creature_t * creature);
void energy(creature_t * creature);
void increment_counters(creature_t * creature);
void decrement_levitation(creature_t * creature);
void decrement_light_source_lifetime(creature_t * creature);
void respawn(level_t * level);

void advance_pos(creature_t * creature);
void reset_seq(creature_t * creature);

void scan_monsters(creature_t * observer);

/* Starting a new game */
game_t * start_new_game(void);

/* Ending a game */
void end_game(game_t * game);

void move_everyone(level_t * level, unsigned int old_y,
		   unsigned int old_x, level_t * new_level,
		   unsigned int new_y,unsigned int new_x);

#endif
