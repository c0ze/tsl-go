#include <stdlib.h>
#include <stdio.h>

#include "main.h"
#include "stuff.h"
#include "teleport.h"
#include "game.h"
#include "player.h"
#include "message.h"
#include "ui.h"
#include "fov.h"
#include "losegame.h"
#include "creature.h"
#include "altitude.h"
#include "traps.h"
#include "elements.h"
#include "effect.h"
#include "find.h"
#include "options.h"



/*
  Teleports CREATURE to TARGET_Y, TARGET_X. If another creatures is
  there, the one with less HP is killed (both if they have the same
  amount of health). Teleporting into a wall is always fatal.
*/
blean_t teleport_creature(creature_t * creature, const unsigned int target_y, const unsigned int target_x)
{
  creature_t * whos_there;
  char line[80];
  level_t * level;
  tile_t target_tile;

  if (creature == NULL)
    return false;

  level = creature->location;

  reveal_mimic(creature);

  /* Is there anyone at the destination? */
  whos_there = find_creature(level, target_y, target_x);
  reveal_mimic(whos_there);
  
  /* Move the target & update map */
  set_creature_coordinates(creature, target_y, target_x);

  /* Clear webbed status. */
  effect_expire(get_effect_by_id(creature, effect_web));
  
  /* What's there? */
  target_tile = get_tile(level, target_y, target_x);

  if (is_wall(level, target_y, target_x))
  {
    /* Target was teleported into a wall */
    if (is_player(creature))
    {
      queue_msg("You materialize within a wall.");
      queue_msg("You die...");
      
      set_attr(creature, attr_killed, 1);
      sprintf(line, "were buried alive within a wall");
      check_for_player_death(line); /* goodbye */
    }
    
    kill_creature(creature, false);
  }
  else if (target_tile == tile_generator ||
	   target_tile == tile_door_closed ||
	   target_tile == tile_terminal)
  {
    if (is_player(creature))
    {
      queue_msg("You materialize within something that was not entirely healthy to teleport into.");
      queue_msg("You die...");
      
      set_attr(creature, attr_killed, 1);
      sprintf(line, "were killed in a teleportation mishap");
      check_for_player_death(line); /* goodbye */
    }
    
    kill_creature(creature, false);
  }
  
  if (whos_there != NULL)
  {
    /* Destination is already occupied by someone else */
    
    /* Make the two creatures damage each other; see who survives */
    while ((killed(whos_there) == false) && (killed(creature) == false))
    {	  
      damage(creature,   1);
      damage(whos_there, 1);
    }
    
    if (is_player(creature))
    {
      /* Can the player see what they ended up in? */
      sprintf(line, "You materialize within %s.",
	      can_see(game->player, target_y, target_x) ? whos_there->name_one : "something");
      
      queue_msg(line);
      reveal_mimic(whos_there);
      
      if (killed(creature))
      {
	/* The player was killed! */
	queue_msg("You are torn apart!");
	queue_msg("You die...");
	
	sprintf(line, "teleported into %s and died", whos_there->name_one);
	
	/* Goodbye */
	check_for_player_death(line);
      }
	
      /*
	The player is still alive, so let's assume the other creature
	died. This won't happen even if they killed each other, since
	then we would already have game over'd.
      */
      if (can_see(game->player, target_y, target_x))
      {
	msg_one(whos_there, "is torn apart!");
      }
      else
      {
	sprintf(line, "Something is torn apart!");
	queue_msg(line);
      }
    }
    else if (is_player(whos_there))
    {
      /* The player was teleported into */
      if (can_see(game->player, target_y, target_x))
      {
	msg_one(creature, "materializes within you.");
	reveal_mimic(creature);
      }
      else
      {
	sprintf(line, "Something materializes within you.");
	queue_msg(line);
      }
      
      if (killed(whos_there))
      {
	queue_msg("You are torn apart!");
	queue_msg("You die...");
	
	sprintf(line, "were telefragged by %s", creature->name_one);
	
	/* Goodbye */
	check_for_player_death(line);
      }
	
      /* The player survived. */
      if (can_see(game->player, target_y, target_x))
	msg_one(creature, "is torn apart!");
      else
      {
	sprintf(line, "It is torn apart!");
	queue_msg(line);
      }
    }
    else if (can_see(game->player, target_y, target_x))
    {
      /* The player wasn't involved, but within view */
      
      sprintf(line, "materializes within %s.", whos_there->name_one);
      msg_one(creature, line);
      reveal_mimic(whos_there);
      
      if (killed(whos_there))
	msg_one(whos_there, "is torn apart!");
      
      if (killed(creature))
	msg_one(creature, "is torn apart!");
    }
  }
  
  if (killed(whos_there))
    kill_creature(whos_there, true);
  
  if (killed(creature))
  {
    kill_creature(creature, true);
  }
  else
  {
    if (target_tile == tile_capsule)
    {
      if (is_player(creature))
	capsule(creature, target_y, target_x);
    }
    else if (find_trap(level, target_y, target_x))
    {
      activate_trap(find_trap(level, target_y, target_x));
    }
    else if (target_tile == tile_forcefield)
    {
      if (is_player(creature))
	queue_msg("You materialize in a forcefield!");
	
      forcefield(creature);
    }
    else if (target_tile == tile_water || target_tile == tile_lava)
    {
      /* "Drop" target into what's there */
      creature->altitude = altitude_floating;
      change_altitude(creature);
    }
    else if (is_player(creature))
    {
      if (creature->altitude == altitude_swimming)
      {
	change_altitude(creature);
      }

      look_at_location(creature, creature->location, target_y, target_x, true);
    }
  }
  
  return false;
} /* teleport_creature */


/*
  If CASTER is under player control: Lets the user select a location
  to teleport from (often the casters own square). All items and
  creatures are then moved from that location to a second location
  specified by the user.
*/
blean_t cast_teleport(creature_t * caster, item_t * source, signed int param)
{
  unsigned int new_y;
  unsigned int new_x;

  unsigned int orig_y;
  unsigned int orig_x;

  creature_t * target;

  item_t ** item_list;
  unsigned int items;
  unsigned int i;

  level_t * level;

  level = caster->location;
  target = NULL;
  item_list = NULL;
  
  if ((caster == NULL) || (level == NULL))
    return false;

  if (caster != game->player)
  {
    /* RFE: For now, computer controlled teleport works just like blink. */
    return cast_blink(caster, source, 0);
  }

  /* Player-controlled */
  
  /* Reveal what the invoked item (if any) is. */
  if (source != NULL)
    id_if_not_known(source);

  queue_msg("What do you want to teleport?");
  msgflush_nowait();
    
  if (get_position_on_map(caster->location, &orig_y, &orig_x) == false)
    return false; /* Cancelled */

  queue_msg("Where to?");
  msgflush_nowait();
    
  if (get_position_on_map(caster->location, &new_y, &new_x) == false)
    return false; /* Cancelled */

  if ((orig_y == new_y) && (orig_x == new_x))
  {
    /* Absolutely nothing happens, but spell was "cast" anyway. */

    /* RFE: Should we "drop" the creature from a higher altitude
     * either way, to reactivate traps? */
    return true;
  }
    
  /*
    Find everything at the origin. We don't care about items at the
    destination nor do we care about creatures there -
    teleport_creature() takes care of those.
  */
  target = find_creature(level, orig_y, orig_x);
  items = find_items(level, orig_y, orig_x, &item_list);
  
  /* Move any items present on the square */
  for (i = 0; i < items; i++)
    place_item(item_list[i], new_y, new_x);
  
  free(item_list);

  /* TODO: Teleporting enemies should make them very angry. */

  /* Move creatures */
  if (target != NULL)
  { 
    if (is_player(target))
      queue_msg("Suddenly, you are somewhere else.");

    teleport_creature(target, new_y, new_x);
  } /* target */

  recenter_if_needed();
  draw_level();
  
  return true;
} /* cast_teleport */



/*
  Moves CASTER two tiles in any direction (supplied as PARAM, or it will prompt for a direction).
*/
blean_t phase(creature_t * caster, item_t * source, signed int param)
{
  level_t * level;
  blean_t spell_known;

  dir_t dir;

  signed int target_y;
  signed int target_x;

  char line[80];

  if (caster == NULL)
    return false;
  
  level = caster->location;

  if (level == NULL)
    return false;

  /* Do we know what spell we're trying to use? Unidentified wands, etc. */
  if (source == NULL || (identified(source) & known_name))
    spell_known = true;
  else
    spell_known = false;

  /* Did we get a direction? */
  if (param != dir_none)
  {
    dir = param;
  }
  else if (is_player(caster))
  {
    /* Let the user select a direction. */
    sprintf(line, "%sin which direction?", spell_known ? "Phase " : "");
    upperfirst(line);
    queue_msg(line);
    msgflush_nowait();
    
    /* This is the last chance to escape the casting */
    dir = get_direction();
  }
  else
  {
    /* Non-player caster - don't do this if we don't know why. */
    queue_msg("BUG: phasing without a direction.");
    return false;
  }

  /* Should we abort? */
  if (dir == dir_none)
  {
    if (is_player(caster))
      clear_msgbar();

    return false;
  }
  
  make_item_known(source);

  /*
    Calculate new coordinates (current + two steps in chosen
    direction), move caster using regular teleport routines.
  */

  dir_to_speed(dir, &target_y, &target_x);

  target_y *= 2;
  target_x *= 2;

  target_y += caster->y;
  target_x += caster->x;

  if (on_map(level, target_y, target_x) == false)
  {
    queue_msg("Can't phase outside map.");
    return false;
  }

  if (spell_known && options.safe_teleport && unsafe_teleport(level->memory[target_y][target_x]))
  {
    if (prompt_yn("This could be unsafe. Do you really want to phase there?") != 1)
    {
      clear_msgbar();
      return false;
    }
  }

  queue_msg("You phase.");
  teleport_creature(caster, target_y, target_x);

  recenter_if_needed();
  draw_level();

  return true;
} /* phase */


/*
  Moves TARGET to a random walkable location not already occupied by someone.
*/
blean_t cast_blink(creature_t * target, item_t * source, signed int param)
{
  unsigned int old_y;
  unsigned int old_x;
  blean_t can_see_new;
  blean_t can_see_old;

  /* Remember from where we blinked */
  old_y = target->y;
  old_x = target->x;

  /* Move the caster to a random spot */
  find_random_free_spot(target);

  can_see_old = can_see(game->player, old_y, old_x);
  can_see_new = can_see(game->player, target->y, target->x);

  if (is_player(target))
  {
    /* It was the player that blinked */
    queue_msg("You blink away...");
    msgflush_wait(); /* Oh, the suspense is killing me! */

    add_anim(anim_type_attention, target->uid, 0, target->y, target->x);

    queue_msg("Suddenly, you are somewhere else.");
    look_at_location(target, target->location, target->y, target->x, true);
    identify(source);
  }
  else
  {
    add_anim(anim_type_attention, target->uid, 0, target->y, target->x);
   
    if (can_see_new && can_see_old)
    {
      /*
	A non-player creature blinked, but reappeared within sight of
	the player.
      */
      msg_one(target, "is suddenly in another place!");
    }
    else if (can_see(game->player, old_y, old_x))
    {
      /* A non-player creature blinked out of view */
      msg_one(target, "blinks away!");
    }
    else if (can_see(game->player, target->y, target->x))
    {
      /* A non-player creature blinked into view */
      msg_one(target, "suddenly appears!");
    }
  }

  /* Activate any trap the caster ended up on. */
  activate_trap(find_trap(target->location, target->y, target->x));

  return true;
} /* cast_blink */



/*
  Marks CASTERs location in the game data. This will be used if recall
  is cast.
*/
blean_t cast_mark(creature_t * caster, item_t * source, signed int param)
{
  unsigned short int i;

  if (caster == NULL ||
      caster->location == NULL ||
      is_player(caster) == false)
  {
    return false;
  }
  
  game->recall_y = caster->y;
  game->recall_x = caster->x;

  /* Find out what level the caster is on. */
  for (i = 0; i < LEVELS; i++)
  {
    if (game->level_list[i] == caster->location)
    {
      game->recall_location = i;
      break;
    }
  }
  
  identify(source);
  queue_msg("Destination marked.");

  return true;
} /* cast_mark */



blean_t cast_recall(creature_t * caster, item_t * source, signed int param)
{
  char line[80];

  if (caster == NULL ||
      is_player(caster) == false)
  {
    return false;
  }

  change_level(game->recall_location);
  set_creature_coordinates(caster, game->recall_y, game->recall_x);

  identify(source);

  sprintf(line, "You find yourself back in %s.",
	  get_current_level()->name);
  
  queue_msg(line);
  
  return true;
} /* cast_recall */



/*
  Returns true if teleporting into what looks like GENT could be
  unsafe. This includes walls and most obstacles.
*/
blean_t unsafe_teleport(const gent_t gent)
{
  switch (gent)
  {
    case gent_blank:
    case gent_obstacle:
    case gent_wall_v:
    case gent_wall_h:
    case gent_wall_es:
    case gent_wall_sw:
    case gent_wall_ne:
    case gent_wall_nw:
    case gent_wall_nes:
    case gent_wall_nsw:
    case gent_wall_new:
    case gent_wall_esw:
    case gent_wall_cross:
    case gent_door_closed:
    case gent_terminal:
    case gent_forcefield:
    case gent_generator:
    case gent_lava:
      return true;
    
    default:
      return false;
  }

  return false;
} /* unsafe_teleport */
