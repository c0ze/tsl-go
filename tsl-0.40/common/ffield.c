#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

#include "main.h"
#include "game.h"
#include "stuff.h"
#include "ffield.h"
#include "find.h"
#include "elements.h"
#include "traps.h"
#include "effect.h"



/*
  Resets and regenerates forcefields on LEVEL.
*/
void build_forcefields(level_t * level)
{
  int y;
  int x;

  blean_t creature_affected[200];
  int creatures;

  int i;
  int number;

  int * generators;

  /* Check which creature were _not_ in a forcefield before. */
  creatures = MIN(200, level->creatures);

  for (i = 0; i < creatures; i++)
  {
    creature_affected[i] = false;

    if (level->creature[i] != NULL)
    {
      if (get_tile(level, level->creature[i]->y, level->creature[i]->x) != tile_forcefield)
	creature_affected[i] = true;
    }
  }

  /*
    Start rebuilding forcefields. First swap all present forcefields
    to floor, then get a list of generators and for each trace a line
    in all four directions, filling it with forcefields until a
    non-floor tile is encountered.
  */
  replace_tile(level, tile_forcefield, tile_floor);
  
  generators = find_all_spots(level, &number, find_generator);
  
  for (i = 0; i < number; i++)
  {
    y = generators[i * 2];
    x = generators[i * 2 + 1];

    if (forcefield_disabled(level, y, x))
      continue;

    forcefield_line(level, y, x, -1, 0);
    forcefield_line(level, y, x, +1, 0);
    forcefield_line(level, y, x, 0, -1);
    forcefield_line(level, y, x, 0, +1);
  }

  free(generators);

  /* Any creatures that weren't in a forcefield earlier but are now should take damage. */
  for (i = 0; i < creatures; i++)
  {
    if (creature_affected[i] &&
	get_tile(level, level->creature[i]->y, level->creature[i]->x) == tile_forcefield)
    {
      forcefield(level->creature[i]);
    }
  }

  return;
} /* build_forcefields */



/*
  Traces a line from OLD_Y, OLD_X on LEVEL in the Y_SPEED/X_SPEED
  direction. If it runs into a generator without encountering any
  obstacle a forcefield will be generated.
*/
void forcefield_line(level_t * level,
		     const unsigned int old_y, const unsigned int old_x,
		     const signed int y_speed, const signed int x_speed)
{
  tile_t there;
  unsigned int y;
  unsigned int x;
  blean_t ok;

  ok = false;

  y = old_y;
  x = old_x;

  while (1)
  {
  play_it_again:

    y += y_speed;
    x += x_speed;

    there = get_tile(level, y, x);

    if (there == tile_generator)
    {
      if (forcefield_disabled(level, y, x))
	return;

      if (ok)
	break;
      
      ok = true;
      y = old_y;
      x = old_x;
      goto play_it_again;

    }
    else if (there == tile_floor || there == tile_forcefield || there == tile_internal_reserved)
    {
      if (ok)
      {
	set_tile(level, y, x, tile_forcefield);
	del_trap(find_trap(level, y, x));
      }
    }
    else
      break;
  }

  return;
} /* forcefield_line */



/*
  Returns true if the forcefield at Y, X is disabled.
*/
blean_t forcefield_disabled(level_t * level, const unsigned int y, const unsigned int x)
{
  effect_t * eff;
  creature_t * player;

  /* Check if the player has an effect_disable for the given coordinates. */

  player = game->player;

  for (eff = player->first_effect; eff != NULL; eff = eff->next_effect)
  {
    if (eff->id == effect_disable &&
	eff->param[EFFECT_Y] == y &&
	eff->param[EFFECT_X] == x)
    {
      return true;
    }
  }

  return false;
} /* forcefield_disabled */



/*
  Disables the forcefield generator at Y, X.
*/
void disable_forcefield(level_t * level, const unsigned int y, const unsigned int x)
{
  effect_t * eff;

  /*
    Attach an effect to the game->player structure that points to the coordinates.
  */

  eff = alloc_effect();

  eff->id = effect_disable;

  eff->param[EFFECT_Y] = y;
  eff->param[EFFECT_X] = x;
  eff->ttl = 5;

  add_effect(game->player, eff);

  build_forcefields(level);

  return;
} /* disable_forcefield */
