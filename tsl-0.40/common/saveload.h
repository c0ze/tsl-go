/*
  saveload.h

  The "save" procedure is based on what we call the "save table". This
  is just a list of pointers to all structures we must save to
  preserve our game.

  The add_*_to_save_table() functions add thing and all its sub-things
  (adding a creatures will also add all its carried items, and so on)
  to the save table. Everything related to a specific game can be
  found under the game_t structure, so add_game_to_save_table() will
  catch it all.

  The write_*_to_savefile() functions write the structures to the
  savefile. It also replaces all pointers with the index of the
  pointed-to struct in the save table (index 0 is reserved for
  NULLs). When the structures are read back in the same order, it's
  trivial for restore_saved_pointers() replace the indices with their
  newly allocated addresses.

  The savefile format is really simple and looks like this:

  1) Number of elements to read - this is acquired by adding all bytes
     until one less than 255 is encountered (if there is indeed a
     multiple of 255, it ends with a 0). Then, for each element:
     
  2) A type identifier byte (one of the SAVE_* constants below),
     stating what kind of structure will follow.

  3) A structure.
*/

#ifndef _SAVELOAD_H_
#define _SAVELOAD_H_

#include <stdio.h>

#include "level.h"
#include "traps.h"
#include "creature.h"
#include "item.h"
#include "effect.h"
#include "game.h"

#define SAVE_NULL         0
#define SAVE_CREATURE     1
#define SAVE_LEVEL        2
#define SAVE_STRING       3
#define SAVE_ITEM         4
#define SAVE_TRAP         5
#define SAVE_GAME         6
#define SAVE_EFFECT       7

typedef int64_t saveptr_t;

/*
  The number of items that should go into the savefile - this should
  be equal to the size of the following two arrays.
*/
unsigned int saveload_items;

/* Pointers to the structures we wish to save */
void ** saveload_stuff;

/*
  What *type* of structure the corresponding index in saved_stuff
  points to - should be one of the SAVE_* constants above.
*/
unsigned int * saveload_type;

void try_to_save_game(void);

void build_saveload_table(void);
saveptr_t get_saveload_index(void * p);
void add_to_saveload_table(void * p, const unsigned int type);
void * get_saveload_pointer(unsigned int index);

void add_level_to_save_table(level_t * l);
void add_trap_to_save_table(trap_t * t);
void add_effect_to_save_table(effect_t * e);
void add_creature_to_save_table(creature_t * c);
void add_item_to_save_table(item_t * i);
void add_game_to_save_table(game_t * p);

blean_t write_savefile(FILE * savefile);
void delete_savefile(void);

blean_t write_effect_to_savefile(FILE * savefile, effect_t * e);
blean_t write_creature_to_savefile(FILE * savefile, creature_t * c);
blean_t write_item_to_savefile(FILE * savefile, item_t * i);
blean_t write_level_to_savefile(FILE * savefile, level_t * l);
blean_t write_trap_to_savefile(FILE * savefile, trap_t * t);
blean_t write_game_to_savefile(FILE * savefile, game_t * g);

void load(void);

void read_effect_from_savefile(FILE * savefile);
void read_creature_from_savefile(FILE * savefile);
void read_item_from_savefile(FILE * savefile);
void read_level_from_savefile(FILE * savefile);
void read_trap_from_savefile(FILE * savefile);
void read_game_from_savefile(FILE * savefile);

game_t * restore_savefile(FILE * savefile);
game_t * restore_saved_pointers(void);

game_t * try_to_load_game(void);

blean_t write_number_as_bytes(FILE * where, const unsigned int number);
signed int read_number_as_bytes(FILE * where);

void saveload_abort(const char * error, const blean_t wipe);

void del_saveload_table(const blean_t wipe);

#endif
