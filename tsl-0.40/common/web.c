#include <stdlib.h>

#include "main.h"
#include "game.h"
#include "web.h"
#include "stuff.h"
#include "creature.h"
#include "player.h"
#include "area.h"
#include "ai.h"
#include "ui.h"
#include "find.h"
#include "actions.h"
#include "traps.h"
#include "effect.h"
#include "fov.h"
#include "memory.h"
#include "message.h"



/*
  Makes TARGET get stuck in a web. If MESSAGE is false no message will
  be printed.
 */
void web(creature_t * target, const blean_t message)
{
  prolong_effect(target, effect_web, WEB_DURATION);

  if (message)
  {
    if (is_player(target))
    {
      queue_msg("You get stuck in a web!");
      msgflush_wait();
      clear_msgbar();
    }
    else if (can_see(game->player, target->y, target->x))
    {
      msg_one(target, "gets stuck in a web!");
    }
  }
  
  return;
} /* web */



blean_t sticky_web(creature_t * caster, item_t * source, signed int param)
{
  level_t * level;
  unsigned int y;
  unsigned int x;
  creature_t * target;
  blean_t message;
  unsigned int range;
  area_t * area;
  trap_t * trap;
  area_t * temp;
  dir_t dir;

  message = false;

  dir = dir_none;
  
  if (caster == NULL)
    return false;

  level = caster->location;

  if (level == NULL)
    return false;

  range = WEB_RAY_RANGE;

  if (param != dir_none)
  {
    dir = param;
  }
  else if (caster == game->player)
  {
    queue_msg("Shoot a sticky web in which direction?");
    
    msgflush_nowait();
    
    dir = get_direction();
    
    if (dir == dir_none)
    {
      clear_msgbar();
      return false;
    }
  }
  else
  {
    /*
      Let the AI decide what direction we should cast in.
    */
    dir = cone_direction(caster, range, false);
    
    if (dir == dir_none)
      return false;
  }
  
  /* Figure out which tiles are affected. */
  y = caster->y;
  x = caster->x;
  area = new_area(y, x);
  area_cone(area, level, y, x, dir, range, false);

  if (is_player(caster))
  {
    queue_msg("You shoot a sticky web!");
    message = true;
  }
  else if (can_see(game->player, y, x) && caster->detected)
  {
    msg_one(caster, "shoots a sticky web!");
    message = true;
  }
  
  if (area_of_effect(game->player, area->next, missile_web))
  {
    /*
      We have now seen the ray. If this was the first time (we didn't
      see the caster earlier), display another message.
    */
    if (message == false)
    {
      queue_msg("Something shoots a sticky web!");
      message = true;
    }
  }

  for (temp = area->next; temp != NULL; temp = temp->next)
  {
    y = temp->y;
    x = temp->x;

    /* Check if we hit anyone... */
    target = find_creature(level, y, x);

    if (target != NULL)
    {
      reveal_mimic(target);

      web(target, false);

      /* We hit someone. Let's do some damage. */
      if (is_player(target))
	queue_msg("You get stuck in the web!");
      else if (can_see(game->player, y, x))
      {
	msg_one(target, "gets stuck in the web!");
	aggravate(target);
      }
    }
    else
    {
      if (get_tile(level, y, x) == tile_floor &&
	  find_trap(level, y, x) == NULL)
      {
	trap = alloc_trap(trap_web);

	if (can_see(game->player, y, x))
	  trap->revealed = true;

	place_trap(trap, y, x);
	del_trap(attach_trap(level, trap));
      }
    }
  } /* for temp */

  del_area(area);

  area = NULL;
  temp = NULL;

  /*
    If message has been set, then at some point we have seen the ray
    or its effects. If this is the case, we should pause so that the
    player can see what area was affected.
  */
  if (message == true)
  {
    update_level_memory(caster->location);
    msgflush_wait();
    clear_msgbar();
  }

  return true;
} /* web_ray */



/*
  Makes CREATURE struggle to break free of a web.
*/
void struggle_web(creature_t * creature)
{
  effect_t * eff;

  if (creature == NULL)
    return;

  eff = get_effect_by_id(creature, effect_web);

  if (eff == NULL)
    return;

  if (eff->ttl < WEB_STRUGGLE)
  {
    if (is_player(creature))
      msg_expire(eff);
    else if (can_see_creature(game->player, creature))
      msg_the(creature, "breaks free of a web!");

    effect_expire(eff);
  }
  else
  {
    eff->ttl -= WEB_STRUGGLE;

    if (is_player(creature))
      queue_msg("You struggle to get free!");
    else if (can_see_creature(game->player, creature))
      msg_the(creature, "struggles to get free of a web!");
  }

  return;
} /* struggle_web */
