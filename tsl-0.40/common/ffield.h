/*
  ffield.h - forcefields

  Forcefields stop projectiles and damages anyone that moves through them.

  tile_forcefield appear between two tile_generator.

  build_forcefields() should be called any time a generator is
  modified. It works by resetting all forcefields to floor, then
  selecting all generators and for each tracing a line in NESW
  directions until another generator is encountered. It will then fill
  the space between them tile_forcefield. There must only be floor
  tiles between them. It will also check that each generator isn't
  affected by any effect_disable (attached to the player structure).
  
  See also: forcefield(), elements.* (for damage)
*/

#ifndef _FFIELD_H_
#define _FFIELD_H_

#include "level.h"

void build_forcefields(level_t * level);
void forcefield_line(level_t * level,
		     const unsigned int old_y, const unsigned int old_x,
		     const signed int y_speed, const signed int x_speed);
blean_t forcefield_disabled(level_t * level, const unsigned int y, const unsigned int x);
void disable_forcefield(level_t * level, const unsigned int y, const unsigned int x);

#endif
