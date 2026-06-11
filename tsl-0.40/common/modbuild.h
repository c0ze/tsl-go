/*
  modbuild.h
*/

#ifndef _MODBUILD_H_
#define _MODBUILD_H_

#include <stdint.h>

#include "main.h"
#include "level.h"

#define MODULES 900

typedef unsigned long int /* uint32_t*/ chunk_bitmask_t;

#define MODULE_WIDE       1
#define MODULE_NARROW     2
#define MODULE_CHAPEL     4
#define MODULE_ROOM       8
#define MODULE_CAVE       32
#define MODULE_FIELDS     64
#define MODULE_CELLS      128
#define MODULE_CITY       256
#define MODULE_WIDE_F     512
#define MODULE_WIDE_ROOM  1024

#define CHAMBER_START         0
#define CHAMBER_KING          1
#define CHAMBER_NECROMANCER   2
#define CHAMBER_LURKER        3
#define CHAMBER_DRAGON        4
#define CHAMBER_TEST          5
#define CHAMBER_ARENA         6
#define CHAMBER_CHAPEL        7
#define CHAMBER_ENDGAME       8
#define CHAMBER_VAULT         9
#define CHAMBER_LAB           10

struct module_t
{
  char ** data;
  unsigned int lines;
  chunk_bitmask_t category;
  unsigned int rarity;
  unsigned int used;
};
typedef struct module_t module_t;

module_t * module[MODULES];



void init_modules(void);
void del_module(module_t * m);
void del_all_modules(void);

module_t * new_module(void);
module_t * new_module_noreg(void);
void add_line(module_t * mod, const char * line);

blean_t module_allowed(level_t * level, module_t * mod,
		       const unsigned int connect_y,
		       const unsigned int connect_x,
		       const unsigned int connect_len,
		       const char connection,
		       unsigned int * off_y,
		       unsigned int * off_x,
		       const chunk_bitmask_t wanted_modules);

blean_t attach_module(level_t * level,
		      const unsigned int connect_y,
		      const unsigned int connect_x,
		      const unsigned int connect_len,
		      const char connection,
		      const chunk_bitmask_t wanted_modules);

void rotate_module(module_t * old);
void mirror_module(module_t * old);

void put_module(level_t * level,
		module_t * mod,
		const unsigned int t,
		const unsigned int l);

void put_special_module(level_t * level, const unsigned int design);

void build_module_dungeon(level_t * level);

void build_chapel(level_t * level);

#endif
