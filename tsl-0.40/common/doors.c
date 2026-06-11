#include <stdlib.h>
#include <stdio.h>

#include "doors.h"
#include "main.h"
#include "stuff.h"
#include "player.h"
#include "message.h"
#include "input.h"
#include "ui.h"
#include "actions.h"
#include "fov.h"
#include "game.h"
#include "memory.h"
#include "find.h"
#include "inventory.h"
#include "stacks.h"
#include "burdened.h"



/*
  Lets CREATURE select a direction to close a door. Returns TRUE if
  the action cost a turn, FALSE if it was cancelled or invalid.
*/
/* RFE: Can this be merged with open_door()? */
unsigned int close_door(creature_t * creature)
{
  dir_t dir;
  signed int move_y;
  signed int move_x;
  signed int stealth_skill;
  unsigned int door_y;
  unsigned int door_x;
  creature_t * some_guy;
  level_t * level;

  if (creature == NULL)
    return 0;

  level = creature->location;

  if (level == NULL)
    return 0;

  /* RFE: Until AI learns how to close doors in an intelligent way... */
  if (is_player(creature) == false)
  {
    queue_msg("BUG: close_door() called for someone else than the player");
    return 0;
  }

  /* Are we allowed to open doors? */
  if (attr_current(creature, attr_p_open_doors))
  {
    char line[80];
    sprintf(line, "As %s, you cannot close doors.", creature->name_one);
    queue_msg(line);
    return false;
  }

  /* Prompt the player for which door to close. */
  queue_msg("Close which door?");
  msgflush_nowait();

  dir = get_direction();
  
  if (dir == dir_none)
  {
    /* The player cancelled, no turn spent. */
    clear_msgbar();
    return false;
  }

  /* Find out where the door is. */
  dir_to_speed(dir, &move_y, &move_x);

  door_y = creature->y + move_y;
  door_x = creature->x + move_x;
  
  if (on_map(level, door_y, door_x) == false)
  {
    queue_msg("BUG: Tried to close door outside map!");
    return false;
  }
  
  /* TODO: What if the player is blind? */
  if (level->map[door_y][door_x] == tile_door_closed)
  {
    queue_msg("It is already closed.");
    return false;
  }

  /*
    With tile_door_closed already covered, we don't care about any
    other tiles than tile_door_open from here on.
  */
  if (level->map[door_y][door_x] != tile_door_open)
  {
    queue_msg("There is no door there.");
    return false;
  }

  /* Is there anyone in the way? */
  some_guy = find_creature(level, door_y, door_x);
    
  if (some_guy != NULL)
  {
    /* There is. Can we see who it is? */
    if (can_see_creature(creature, some_guy))
    {
      msg_one(some_guy, "is in the way!");

      /*
	If we didn't know someone was there, this will cost a turn
	(since it revealed information).
      */
      if (some_guy->detected == false)
      {
	some_guy->detected = true;
	draw_level();
	return true;
      }
    }
    else
    {
      /*
	This happens if the player is blinded, etc. This won't cost a
	turn since the player is already at such a disadvantage.
      */
      queue_msg("There is something in the way.");
    }

    return false;
  }

  /*
    Is there an item blocking? If there is, we either knew about it *
    or we're blind, so this won't cost a turn.
  */
  if (find_items(level, door_y, door_x, NULL) > 0)
  {
    queue_msg("There is something in the way.");
    return false;
  }

  /* Ok, close it. */
  level->map[door_y][door_x] = tile_door_closed;

  /*
    Note in level memory we have closed the door. This is to ensure
    this change is reflected even when the player is blinded. If they
    can see this will be overwritten anyway.
  */
  level->memory[door_y][door_x] = gent_door_closed;
  
  /* Easiest way to update FOV */
  set_creature_coordinates(creature, creature->y, creature->x);
  
  /*
    Make some noise if the player fails a stealth roll. This must be done
    after we set the tile and rebuild FOV, or we will see "... wakes up!"
    messages from creatures _behind_ the door we just closed.
  */
  stealth_skill = attr_current(creature, attr_stealth);

  if (roll_xn(stealth_skill, DOOR_STEALTH))
  {
    queue_msg("You silently close the door.");
  }
  else
  {
    queue_msg("You slam the door shut.");
    draw_attention(level, door_y, door_x, noise_loud);
  }

  draw_level();

  return true;
} /* close_door */



/*
  Lets CREATURE open a door at {DOOR_Y, DOOR_X} on its current
  level. Returns TRUE if the action cost a turn, FALSE if it was
  cancelled or invalid.
*/
blean_t open_door(creature_t * creature,
		  const int door_y,
		  const int door_x)
{
  signed int stealth_skill;
  level_t * level;
  int temp;
  item_t * item;
  char line[100];
  
  if (creature == NULL)
    return false;

  level = creature->location;

  if (level == NULL)
    return false;

  /* This should never happen, or something has gone wrong in UI/AI. */
  if (on_map(level, door_y, door_x) == false)
  {
    queue_msg("BUG: Tried to open door outside map!");
    return false;
  }
  
  /*
    Are we allowed to open doors? This behaves differently for player
    and monster. The player will just get a message. Monsters will
    spend a turn and maybe make some noise to the player.
  */
  if (attr_current(creature, attr_p_open_doors))
  {
    if (is_player(creature))
    {
      sprintf(line, "As %s, you cannot open doors.", creature->name_one);
      queue_msg(line);
      return false;
    }
    else if (maybe())
    {
      /*
	We consider the player to be within hearing distance if they
	can _see_ the door. Messages vary with the type of monster and
	their unarmed attack.
      */
      if (can_see(game->player, door_y, door_x))
      {
	if (creature->id == monster_ghoul ||
	    creature->id == monster_drowned_one)
	{
	  queue_msg("You hear something moaning behind the door.");
	}
	else if (creature->unarmed == virtual_claws)
	{
	  queue_msg("You hear something clawing at the door.");
	}
	else if (creature->unarmed == virtual_fists ||
		 creature->unarmed == virtual_kick)
	{
	  queue_msg("You hear something kicking the door.");
	}
	else
	{
	  queue_msg("You hear something scratching on the door.");
	}
      }

      return true;
    }
    else
    {
      return false;
    }
  }

  if (get_tile(level, door_y, door_x) == tile_door_open)
  {
    if (is_player(creature))
      queue_msg("It is already open.");

    return false;
  }

  /* Locked doors */
  if (get_tile(level, door_y, door_x) == tile_door_locked)
  {
    if (!is_player(creature))
      return false;

    queue_msg("It is locked.");

    item = find_item_in_inventory(creature, treasure_key);

    temp = 0;

    if (item)
    {
      temp = stack_size(item);
      sprintf(line, "Unlock it (%d %s)?", temp, (temp > 1 ? "keys" : "key"));
      temp = prompt_yn(line);
    }

    if (temp == 1)
    {
      unlock_door(creature, level, door_y, door_x, item);
      return true;
    }
    else if (temp == 0)
    {
      item = find_item_in_inventory(creature, treasure_crowbar);

      if (item)
	temp = prompt_yn("Attempt to break it with your crowbar?");
      else
	temp = prompt_yn("Attempt to break it?");
      
      if (temp == 1)
      {
	if (item || roll_xn(5, 3))
	{
	  if (item)
	    queue_msg("You break the door open with your crowbar!");
	  else
	    queue_msg("You break the door open!");

	  set_tile(level, door_y, door_x, tile_floor);
	  
	  level->memory[door_y][door_x] = gent_floor;
	  set_creature_coordinates(creature, creature->y, creature->x);
	  draw_level();
	}
	else
	{
	  queue_msg("You fail to force the door.");
	  draw_attention(level, door_y, door_x, noise_loud);
	}
	
	return true;
      }
    }

    queue_msg("Never mind, then.");
    return false;
  }
  
  /* We don't care about any other tiles than tile_door_closed from here on. */
  if (level->map[door_y][door_x] != tile_door_closed)
  {
    if (is_player(creature))
      queue_msg("There is no door there.");

    return false;
  }

  /* Ok, open it. */
  level->map[door_y][door_x] = tile_door_open;
  
  if (is_player(creature))
  {
    /* Make some noise if the player fails a stealth roll. */
    stealth_skill = attr_current(creature, attr_stealth);

    if (roll_xn(stealth_skill, DOOR_STEALTH))
    {
      queue_msg("The door opens silently.");
    }
    else
    {
      queue_msg("The door squeaks on its hinges.");
      draw_attention(level, door_y, door_x, noise_loud);
    }
  }
  else if (can_see(game->player, door_y, door_x))
  {
    /* Monster opened a door, we saw it. */
    queue_msg("A door opens.");
    break_rest(game->player);
    /*    draw_level();*/
  }
  else
  {
    /* Monster opened a door, we hear it across the level. */
    queue_msg("You hear a door open.");
  }

  /*
    Note in level memory we have closed the door. This is to ensure
    this change is reflected even when the player is blinded. If they
    can see this will be overwritten anyway.
  */
  if (is_player(creature) || can_see(game->player, door_y, door_x))
  {
    level->memory[door_y][door_x] = gent_door_open;
    set_creature_coordinates(creature, creature->y, creature->x);
    draw_level();
  }

  return true;
} /* open_door */



/*
  Player object-verb interface for using keys. CREATURE should be the
  player, KEY a key to use.
*/
blean_t key_open(creature_t * creature, item_t * key)
{
  int door_y;
  int door_x;
  int move_y;
  int move_x;
  level_t * level;
  dir_t dir;
  char line[100];

  if (creature == NULL ||
      (level = creature->location) == NULL ||
      is_player(creature) == false)
  {
    return false;
  }

  if (attr_current(creature, attr_p_open_doors))
  {
    sprintf(line, "As %s, you cannot unlock doors.", creature->name_one);
    queue_msg(line);
    return false;
  }

  queue_msg("Unlock what?");
  msgflush_nowait();

  dir = get_direction();

  if (dir == dir_none)
  {
    clear_msgbar();
    return false; /* Cancelled */
  }

  dir_to_speed(dir, &move_y, &move_x);

  door_y = creature->y + move_y;
  door_x = creature->x + move_x;

  if (get_tile(level, door_y, door_x) == tile_door_locked)
  {
    unlock_door(creature, level, door_y, door_x, key);
    return true;
  }
  
  queue_msg("There is nothing there to unlock.");

  return false;
} /* key_open */



/*
  Unlocks the door on LEVEL at DOOR_Y, DOOR_X. If KEY is provided one
  will be spent.
 */
void unlock_door(creature_t * creature,
		 level_t * level,
		 const int door_y,
		 const int door_x,
		 item_t * key)
{
  int w;

  set_tile(level, door_y, door_x, tile_door_open);

  if (is_player(creature))
    queue_msg("You unlock the door.");
  
  /* Destroy one key */
  if (key)
  {
    w = key->weight;
    del_item(get_item_from_stack(key));
    unburdened(creature, w);
  }
  
  build_fov(creature);
  update_level_memory(creature->location);
  draw_level();

  return;
} /* unlock_door */



/*
  Makes Y, X on LEVEL a door, with a random chance of it being locked
  or just closed.
*/
void maybe_locked_door(level_t * level, const int y, const int x)
{
  if (maybe())
    set_tile(level, y, x, tile_door_locked);
  else
    set_tile(level, y, x, tile_door_closed);

  return;
} /* maybe_locked_door */



/*
  Replaces secret doors on LEVEL with real doors. Only a small number will be left secret.
 */
void replace_doors(level_t * level)
{
  int * shit;
  int total;
  int doors_to_keep;
  int y;
  int x;
  int i;

  doors_to_keep = 3 + roll(1, 5);

  shit = find_all_spots(level, &total, find_secret_door);
  free(shit);

  while (total > doors_to_keep)
  {
    total--;

    find_spot(level, &y, &x, find_secret_door);

    if (maybe())
      set_tile(level, y, x, tile_floor);
    else
      set_tile(level, y, x, tile_door_closed);
  }

  shit = find_all_spots(level, &total, find_closed_door);

  for (i = 0; i < total; i += 2)
  {
    y = shit[i];
    x = shit[i + 1];
    
    maybe_locked_door(level, y, x);
  }

  free(shit);

  return;
} /* replace_doors */
