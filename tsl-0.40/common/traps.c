#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

#include "main.h"
#include "stuff.h"
#include "rules.h"
#include "traps.h"
#include "item.h"
#include "creature.h"
#include "ui.h"
#include "game.h"
#include "player.h"
#include "actions.h"
#include "effect.h"
#include "places.h"
#include "combat.h"
#include "fov.h"
#include "ai.h"
#include "missile.h"
#include "losegame.h"
#include "altitude.h"
#include "shapeshf.h"
#include "explode.h"
#include "gore.h"
#include "memory.h"
#include "find.h"
#include "wingame.h"
#include "inscript.h"
#include "web.h"



/*
  Returns a pointer to a newly allocated trap of type NEW_TYPE.
*/
trap_t * alloc_trap(traptype_t new_type)
{
  trap_t * local;

  local = malloc(sizeof(trap_t));
  if (local == NULL) out_of_memory();
  mem_alloc.traps++;

  local->location = NULL;
  local->prev_trap = NULL;
  local->next_trap = NULL;

  local->revealed = false;
  local->hidden = false;

  local->difficulty = roll(2, 6);

  local->type = new_type;

  local->gent = gent_blink_trap;

  switch(local->type)
  {
    case trap_dart:
      local->gent = gent_dart_trap;
      local->activations_remaining = roll(10, 6);
      local->difficulty *= 2;
      break;

    case trap_booby:
      local->gent = gent_booby_trap;
      local->activations_remaining = 1;
      break;

    case trap_web:
      local->gent = gent_web;
      local->activations_remaining = 1;
      break;

    case trap_blink:
      local->gent = gent_blink_trap;
      local->activations_remaining = roll(1, 6);
      local->difficulty *= 2;
      break;

      /* These are always only triggered once. */
    case trap_glass:
      local->gent = gent_glass_trap;
      local->activations_remaining = 1;
      break;

    case trap_plate:
      local->gent = gent_plate_trap;
      local->activations_remaining = 1;
      break;

    case trap_flash:
      local->gent = gent_flash_trap;
      local->activations_remaining = 1;
      break;

    case trap_medkit:
      local->gent = gent_medkit;
      local->activations_remaining = 1;
      local->revealed = true;
      break;

    case trap_win:
      local->gent = gent_blink_trap;
      local->activations_remaining = 1;
      local->revealed = true;
      break;

    case trap_teleporter:
      local->gent = gent_blink_trap;
      local->activations_remaining = 1;
      local->revealed = true;
      break;

    case trap_polymorph:
      local->gent = gent_blink_trap;
      local->activations_remaining = 1;
      break;

    case trap_inscription:
      local->hidden = true;
      local->activations_remaining = 1;
      local->param[INSCR_TYPE] = tslrnd() % INSCR_TYPES;
      local->param[INSCR_MESSAGE] = tslrnd() % INSCR_MESSAGES;
      break;

    default:
      local->activations_remaining = 1;
      break;
  }
  
  return local;
} /* alloc_trap */



/*
  Deletes TRAP from the game.
*/
void del_trap(trap_t * trap)
{
  if (trap == NULL)
    return;

  detach_trap(trap);

  free(trap);
  mem_alloc.traps--;
  
  return;
} /* del_trap */



/*
  Attaches a trap to a level. If the trap couldn't be attached for
  some reason, it will be returned. Otherwise, NULL is returned.
*/
trap_t * attach_trap(level_t * level, trap_t * trap)
{
  if ((level == NULL) || (trap == NULL))
    return trap;

  /* Remove the trap from wherever it is right now. */
  detach_trap(trap);
  
  /*
    Insert the new trap at the first location in the linked list of
    traps. The order of traps doesn't matter to rooms, so we'll just
    use the fastest way possible.
  */
  trap->prev_trap = NULL; /* This will be the new *first* item */
  trap->next_trap = level->first_trap; /* Point to the old 1st item as 2nd */
  level->first_trap = trap; /* Set the new item as 1st */
  
  /* Is there a second trap? */
  if (trap->next_trap != NULL)
    trap->next_trap->prev_trap = trap; /* Link the 2nd back to the new first */

  /* Make sure the trap knows where it is. */
  trap->location = level;

  return NULL;
} /* attach_trap */



/*
  Removes TRAP from wherever it might be. Note that this function will
  leave the trap floating in the void; the calling function must
  ensure that it is either attached to another level or deallocated.
*/
trap_t * detach_trap(trap_t * trap)
{
  trap_t * temp;
  
  if (trap == NULL)
    return NULL;

  if (trap->location != NULL)
  {
    /* Loop through the trap list, look for the trap... */
    for (temp = trap->location->first_trap;
	 temp != NULL;
	 temp = temp->next_trap)
    {
      if (temp == trap)
      {
	/* Found it! */
	/* Repoint next traps previous pointer to this ones previous. */
	if (trap->next_trap != NULL)
	  trap->next_trap->prev_trap = trap->prev_trap;
	
	/* Repoint previous traps next pointer to this ones next. */
	if (trap->prev_trap != NULL)
	  trap->prev_trap->next_trap = trap->next_trap;
	
	/* TODO: Investigate: Isn't this implied by prev_trap != NULL? */
	/*
	  If this is the first trap, we need to change the levels
	  pointer as well.
	*/
	if (trap == trap->location->first_trap)
	  trap->location->first_trap = trap->next_trap;
      } /* if trap */
    } /* for */
  } /* if location */
  
  /* Since the trap is now "nowhere", these should be NULLed. */
  trap->next_trap = NULL;
  trap->prev_trap = NULL;
  trap->location = NULL;
  
  return trap;
} /* detach_trap */



/*
  Moves TRAP to {NEW_Y, NEW_X}.
*/
void place_trap(trap_t * trap,
		const unsigned int new_y, const unsigned int new_x)
{
  if (trap == NULL)
    return;

  trap->y = new_y;
  trap->x = new_x;

  return;
} /* place_trap */



/*
  Returns any trap found (or NULL if none) on LEVEL at {Y, X}. Note
  that while it is possible for place_trap() to set several traps at
  the same coordinates, this function (and any that depend on it for
  searching) will only see the trap that happens to be first in the
  list.
*/
trap_t * find_trap(const level_t * level,
		   const unsigned int y, const unsigned int x)
{
  trap_t * temp;

  if (level == NULL)
    return NULL;

  if (level->first_trap == NULL)
    return NULL;

  for (temp = level->first_trap; temp != NULL; temp = temp->next_trap)
  {
    if ((temp->y == y) && (temp->x == x))
      return temp;
  }

  return NULL;
} /* find_trap */



/*
  Activates TRAP. Return true if the creature standing on the trap is
  killed or teleported away.
*/
blean_t activate_trap(trap_t * trap)
{
  char line[80];
  level_t * level;
  creature_t * target;
  item_t * item;
  char * name;
  unsigned int y;
  unsigned int x;
  blean_t target_gone;
/*  unsigned int creature_index;*/

  /*
    We haven't killed or teleported anything yet, but we'll set this
    to true if we do (hey, a rhyme).
  */
  target_gone = false;

  /*
    Check that there actually is a trap, a level and a creature
    standing on that particular trap.
  */
  if (trap == NULL ||
      (level = trap->location) == NULL)
    return false;
  
  y = trap->y;
  x = trap->x;
  target = find_creature(trap->location, y, x);
  
  if (target == NULL)
    return false;

  if (is_floating(target) && trap->type != trap_win)
    return false;
  
  if (is_player(game->player) ||
      can_see(game->player, y, x))
  {
    if (trap->hidden == false)
      trap->revealed = true;
  }

  switch (trap->type)
  {
    case trap_inscription:
      if (is_player(target) && is_blinded(target) == false)
      {
	inscription(trap->param[INSCR_TYPE], trap->param[INSCR_MESSAGE]);
	del_trap(trap);
      }
      return false;
    
    case trap_medkit:
      if (is_player(target))
      {
	queue_msg("Medkit!");
	heal(target, MEDKIT_HEAL_AMOUNT);

	/*
	  Let's cheat a bit and remove the medkit right away so we can
	  rebuild memory. If we don't medkits won't disappear from
	  memory if the player is blinded, but the player _knows_ that
	  medkits are always one use only.
	*/
	del_trap(trap);
	update_level_memory(level);
	return false;
      }
      else
	return false;

      break;

    case trap_win:
      if (is_player(target))
      {
	win_game(ENDING_ASCENSION);
      }
      else
	return false;

      break;

    case trap_web:
      web(target, true);
      break;

    case trap_teleporter:
/*      if (is_player(target))
      {
	change_level(LEVEL_ENDGAME);
	find_spot(game->player->location, &y, &x, find_start);
	replace_tile(game->player->location, tile_internal_start, tile_floor);
	set_creature_coordinates(game->player, y, x);
      }
      else*/
	return false;

      break;

    case trap_polymorph:
      if (is_player(target))
      {
	queue_msg("You step on a polymorph trap!");
	msgflush_wait();

	if (attr_current(target, attr_unchanging) > 0)
	{
	  queue_msg("Nothing happens.");
	  break;
	}
      }
      else if (can_see_creature(game->player, target))
      {
	msg_one(target, "triggers a polymorph trap!");
      }

      shapeshift_random(target, NULL, 0);
      
      build_fov(game->player);

      target_gone = true;
      break;

      /* The creature steps in broken glass. */
    case trap_glass:
      if (attr_current(target, attr_dissolve) > 0)
      {
	if (is_player(target))
	  queue_msg("Some broken glass dissolves under you!");
	else if (can_see(game->player, y, x))
	{
	  sprintf(line, "Some glass dissolves in %s!", target->name_the);
	  upperfirst(line);
	  queue_msg(line);
	}

	break;
      }

      if (is_player(target))
      {
	queue_msg("You step in some broken glass!");
	msgflush_wait();
	clear_msgbar();

	if (attr_current(target, attr_feet_protected) != 0)
	{
	  queue_msg("Fortunately, you are wearing boots.");
	}
      }
      else if (can_see(game->player, y, x))
      {
	msg_one(target, "steps in broken glass!");
      }

      if (attr_current(target, attr_feet_protected) == 0)
      {
	damage(target, 1);

	if (killed(target))
	{
	  if (is_player(target))
	  {
	    /* We've been killed. */
	    queue_msg("You die...");
	    sprintf(line, "succumbed to broken glass on the dungeon floor");
	    check_for_player_death(line);
	  }
	  else
	  {
	    creature_death(target, NULL, NULL);
	  }
	}
      }

      break;

      /* The creature steps on an electrified plate. */
    case trap_plate:
      if (is_player(target))
      {
	queue_msg("You step on an electrified plate!");
	msgflush_wait();
	clear_msgbar();
      }
      else if (can_see(game->player, y, x))
      {
	msg_one(target, "steps on an electrified plate!");
      }

      damage(target, sroll(PLATE_DAMAGE));
      
      if (killed(target))
      {
	if (is_player(target))
	{
	  /* We've been killed. */
	  queue_msg("You die...");
	  sprintf(line, "were electrocuted by an electrified plate");
	  check_for_player_death(line);
	}
	else
	{
	  creature_death(target, NULL, NULL);
	}
      }

      break;

    /* Teleport the creature to a random location. */
    case trap_blink:
      if (is_player(target))
      {
	queue_msg("You trigger a blink trap!");

	/*
	  We have to update the seen memory here, or the trap that was
	  just stepped on might not show up. The easiest way to do
	  this is to simply re-set the creatures coordinates.
	*/
	set_creature_coordinates(target, y, x);
      }
      else
      {
	if (can_see(game->player, y, x))
	{
	  msg_one(target, "triggers a blink trap!");

	  if (trap->activations_remaining == 1)
	  {
	    level->memory[y][x] = level->map[y][x];
	  }
	}
      }

      cast_blink(target, NULL, 0);

      target_gone = true;
      break; /* trap_blink */

      /* Blind the creature */
    case trap_flash:
      if (is_blinded(target))
	break;

      if (is_player(target))
      {
	queue_msg("You trigger a trap!");
	queue_msg("You are blinded by a bright flash!");

	/*
	  We have to update the seen memory here, or the trap that was
	  just stepped on might not show up. The easiest way to do
	  this is to simply re-set the creatures coordinates.
	*/
	set_creature_coordinates(target, y, x);
      }
      else
      {
	if (can_see(game->player, y, x))
	  msg_one(target, "triggers a flash trap!");
      }

      prolong_effect(target, effect_blindness, DEFAULT_BLIND_TIME);

      build_fov(target);
      
      if (is_player(target))
      {
	/* Hack: Override seen memory so we record this trap is gone (flash traps are one activation only). */
	level->memory[y][x] = tile_info[level->map[y][x]]->gent;

	draw_level();
	display_stats(target); /* Update so we see "Blind" */

	msgflush_wait();
	clear_msgbar();
      }
      break; /* trap_flash */

      /* Shoot the creature with a dart. */
    case trap_dart:
      /* Build the missile we'll use. */
      item = build_item(treasure_d_small);

      name = get_item_name(item);

      if (is_player(target))
      {
	sprintf(line, "%s shoots out at you!", name);
	upperfirst(line);
	queue_msg(line);
	msgflush_wait();
      }
      else if (can_see(game->player, y, x))
      {
	if (can_see_creature(game->player, target))
	  sprintf(line, "%s shoots out at %s!", name, target->name_the);
	else
	  sprintf(line, "%s shoots out at something!", name);
	  
	upperfirst(line);
	queue_msg(line);
	msgflush_wait();
      }

      free(name);
      name = NULL;

      if (missile_hit(level, NULL, NULL, item,
		      y, x, 0, 0, false,
		      target, &target_gone) == false)
      {
	/*
	  Creature dodged. Place the missile on the floor.
	*/
	attach_item_to_level(level, item);
	place_item(item, y, x);
      }

      draw_attention(target->location, target->y, target->x, 4);
      break;

      /* Blow the creature to pieces (maybe?) with an explosion. */
    case trap_booby:
      if (is_player(target))
      {
	queue_msg("You trigger an explosion!");
      }
      else
      {
	if (can_see(game->player, y, x))
	  msg_one(target, "triggers an explosion!");
      }
      
      explosion(level, target, NULL, y, x, expl_booby_trap, sroll(BOOBY_TRAP_DAMAGE),
		target, &target_gone);
      break; /* trap_fire */

      /* This should never happen... */
    default:
      queue_msg("BUG: Trap with unknown type activated!");
      break;
  }

  /* Does this trap have a limited number of activations? */
  if (trap->activations_remaining > 0)
  {
    /* It does; decrease the number of remaining activations. */
    trap->activations_remaining--;
    
    /* If the trap is "spent", delete it from play. */
    if (trap->activations_remaining == 0)
      del_trap(trap);
  }

  return target_gone;
} /* activate_trap */



/*
  Adds a broken glass trap at Y, X on LEVEL, if it's a floor tile and
  there is no trap there already.
*/
blean_t broken_glass(level_t * level, const unsigned int y, const unsigned int x)
{
  trap_t * trap;

  if (find_trap(level, y, x) ||
      get_tile(level, y, x) != tile_floor)
    return false;

  trap = alloc_trap(trap_glass);
  
  if (attach_trap(level, trap))
  {
    del_trap(trap);
    return false;
  }

  place_trap(trap, y, x);

  if (can_see(game->player, y, x))
    trap->revealed = true;

  return true;
} /* broken_glass */
