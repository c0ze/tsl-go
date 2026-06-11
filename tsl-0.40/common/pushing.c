#include <stdlib.h>

#include "pushing.h"
#include "game.h"
#include "message.h"
#include "fov.h"
#include "altitude.h"
#include "input.h"
#include "stuff.h"
#include "ai.h"
#include "player.h"
#include "actions.h"
#include "ui.h"
#include "sleep.h"
#include "elements.h"
#include "ffield.h"
#include "memory.h"
#include "traps.h"
#include "find.h"



blean_t push_internal(creature_t * creature, dir_t dir, unsigned int dist)
{
  level_t * level;
  signed int move_y;
  signed int move_x;
  unsigned int old_y;
  unsigned int old_x;
  blean_t moved;

  if (creature == NULL ||
      (level = creature->location) == NULL)
  {
    return false;
  }

  old_y = creature->y;
  old_x = creature->x;

  dir_to_speed(dir, &move_y, &move_x);

  moved = false;

  while (dist > 0)
  {
    if (is_traversable(creature->location, false, creature->y + move_y, creature->x + move_x))
    {
      creature->y += move_y;
      creature->x += move_x;
      moved = true;
    }

    dist--;

    if (get_tile(creature->location, creature->y, creature->x) == tile_forcefield)
      break;
  }

  if (moved)
  {
    if (is_player(creature))
      queue_msg("You are knocked back!");
    else if (can_see(game->player, old_y, old_x))
      msg_one(creature, "is knocked back!");

    creature_sleep(creature, false);
    
    creature->altitude = altitude_floating;
    aggravate(creature);

    /* Check and apply any forcefield present there. */
    if (get_tile(level, creature->y, creature->x) == tile_forcefield)
    {      
      if (forcefield(creature) == false)
      {
	if (is_floating(creature) == false)
	  creature->altitude = altitude_walking;
      }
    }
    else
    {
      /*
	It survived the (potential) forcefield. Drop the creature at
	the new location. Pointer may no longer be valid after this!
	(traps, etc)
      */
      
      change_altitude(creature);
    }

    draw_level();
    msgflush_wait();
    clear_msgbar();

    return true;
  }
  
  return false;
} /* push_internal */




blean_t push(creature_t * caster, item_t * source, signed int param)
{
  level_t * level;
  creature_t * target;
  signed int move_y;
  signed int move_x;
  signed int y;
  signed int x;
  unsigned int range;
  blean_t spell_known;
  blean_t moved;
  blean_t message;
  dir_t dir;

  if (caster == NULL ||
      (level = caster->location) == NULL)
  {
    return false;
  }

  if ((source == NULL) || (identified(source) & known_name))
    spell_known = true;
  else
    spell_known = false;

  if (param != dir_none)
  {
    dir = param;
  }
  else if (caster == game->player)
  {
    /* Let the user select a direction */
    if (spell_known)
      queue_msg("Push in which direction?");
    else
      queue_msg("In which direction?");

    msgflush_nowait();
    
    /* This is the last chance to escape the casting */
    dir = get_direction();

    if (dir == dir_none)
    {
      clear_msgbar();
      return false;
    }
  }
  else
  {
    dir = target_best_direction(caster, PUSH_RANGE);

    if (dir == dir_none)
      return false;
  }

  message = false;

  dir_to_speed(dir, &move_y, &move_x);

  /* Pushing by hand only reaches 1 tile. Wands have a longer range. */
  if (source)
    range = PUSH_RANGE;
  else
    range = 1;

  y = caster->y;
  x = caster->x;

  while (range > 0)
  {
    y += move_y;
    x += move_x;

    /* Check if there is any pushable tile there - move it. */
    if (push_tile(caster->location, y, x, dir, source))
    {
      update_level_memory(caster->location);
      draw_level();
/*      msgflush_wait();*/
      return true;
    }

    /* Check if we hit any creature instead. */
    target = find_creature(level, y, x);

    if (target)
    {
      if ((target->ai_state == ai_mimic && reveal_mimic(target)) ||
	  reveal_creature(target))
      {
	message = true;
	draw_level();
	msgflush_wait();
      }

      moved = push_internal(target, dir, sroll(PUSH_ROLL));

      if (moved && can_see(game->player, y, x))
      {
	message = true;
      }

      break;
    }
    
    range--;
  }

  if (message == true)
  {
    if (is_player(caster))
      make_item_known(source);
  }
  else
  {
    queue_msg("Nothing happens.");
  }

  return true;
} /* push */



/*
  The return value does not say if any tile was pushed, just that something pushable was in the way.
*/
blean_t push_tile(level_t * level, const unsigned int y, const unsigned int x, const dir_t dir, const item_t * source)
{
  signed int move_y;
  signed int move_x;
  signed int new_y;
  signed int new_x;
  tile_t new_tile;
  tile_t old_tile;
  blean_t valid;

  if (level == NULL)
    return false;

  old_tile = get_tile(level, y, x);

  if (is_pushable(level, y, x) == false)
    return false;
  
  dir_to_speed(dir, &move_y, &move_x);
  
  new_y = y + move_y;
  new_x = x + move_x;

  new_tile = get_tile(level, new_y, new_x);
  valid = new_tile == tile_forcefield || new_tile == tile_floor;

  if (valid &&
      find_creature(level, new_y, new_x) == NULL)
  {
    set_tile(level, new_y, new_x, old_tile);
    set_tile(level, y, x, tile_floor);

    if (can_see(game->player, y, x))
    {
      if (old_tile == tile_generator)
	queue_msg("You push a forcefield generator!");
      else if (old_tile == tile_block)
	queue_msg("You push a block!");

      make_item_known(source);
    }
    else if (source == NULL)
    {
      queue_msg("You push something!");
    }

    /* Destroy any trap we move onto. */
    del_trap(find_trap(level, new_y, new_x));

    /* CRUSH any items there. */
    destroy_all_items(level, new_y, new_x);
    
    build_forcefields(level);
    return true;
  }
  
  return false;
} /* push_pile */



/*
  Return true if the tile at Y, X on LEVEL is a type that can be pushed around.
*/
blean_t is_pushable(level_t * level, const unsigned int y, const unsigned int x)
{
  tile_t tile;

  if (level == NULL)
    return false;
  
  tile = get_tile(level, y, x);

  if (tile == tile_generator || tile == tile_block)
    return true;

  return false;
} /* is_pushable */
