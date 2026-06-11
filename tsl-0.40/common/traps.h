/*
  traps.h
 */

#ifndef _TRAPS_H_
#define _TRAPS_H_

#include "main.h"
#include "level.h"
#include "gent.h"
#include "teleport.h"



enum traptype_t
{
  trap_booby,
  trap_blink,
  trap_dart,
  trap_glass,
  trap_plate,
  trap_polymorph,
  trap_flash,
  trap_web,

  trap_medkit,
  trap_win,
  trap_teleporter,
  trap_inscription,
  
  /* TODO: Implement these */
  trap_gas,
  trap_acid
};
typedef enum traptype_t traptype_t;


#define TRAP_PARAMS 4


struct trap_t
{
  /*
    Identifies what kind of trap we're dealing with; should be one of
    traptype_t.
  */
  traptype_t type;

  /*
    What this trap should look like when it's detected.
  */
  gent_t gent;

  /*
    How many times this trap can be activated. If nonzero, it will be
    decremented for each activation, and when it reaches zero the trap
    will be destroyed. If *set* to zero, the check will be bypassed
    and the trap can be activated an unlimited number of times.
  */
  unsigned int activations_remaining;

  /*
    Is it possible for the player to see this trap? If true, it will
    be completely hidden from the players view, but still trigger
    normally.
  */
  blean_t hidden;

  /*
    Is the player aware of this trap? A trap can't be revealed unless
    HIDDEN is false.
  */
  blean_t revealed;

  /* The Perception needed to detect this trap. */
  unsigned int difficulty;

  /* Where the trap is. */
  level_t * location;
  unsigned int y;
  unsigned int x;

  /* The linked list of traps on the level. */
  trap_t * next_trap;
  trap_t * prev_trap;

  /* General-purpose parameters */
  unsigned int param[TRAP_PARAMS];
};



trap_t * alloc_trap(traptype_t new_type);
void del_trap(trap_t * trap);
trap_t * attach_trap(level_t * level, trap_t * trap);
trap_t * detach_trap(trap_t * trap);
void place_trap(trap_t * trap, const unsigned int new_y, const unsigned int new_x);
trap_t * find_trap(const level_t * level, const unsigned int y, const unsigned int x);
blean_t activate_trap(trap_t * trap);
blean_t broken_glass(level_t * level, const unsigned int y, const unsigned int x);

#endif
