#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "main.h"
#include "stuff.h"
#include "explode.h"
#include "level.h"
#include "creature.h"
#include "elements.h"
#include "area.h"
#include "fov.h"
#include "game.h"
#include "ui.h"
#include "player.h"
#include "losegame.h"
#include "ai.h"
#include "sleep.h"
#include "ffield.h"
#include "find.h"
#include "combat.h"



/*
  Creates an explosion on LEVEL, caused by CAUSE (optional) using
  SOURCE (optional). It will be centered at {CENTER_Y, CENTER_X} and
  do DAM damage (before resistances) to all creatures that can't dodge
  it. E_TYPE specifies the type of message that should be displayed
  for cases where CAUSE and SOURCE aren't enough. If WATCH and
  CASUALTY are set, CASUALTY will be set true if WATCH gets killed.
*/
void explosion(level_t * level,
	       creature_t * cause, item_t * source,
	       const unsigned int center_y, const unsigned int center_x,
	       const explosion_type_t e_type, const unsigned int dam,
	       creature_t * watch, blean_t * casualty)
{
  char line[80];
  blean_t message;
  signed int y;
  signed int x;
  creature_t * creature;
  signed int damage_taken;
  blean_t seen;
  area_t * area;
  area_t * temp_area;
  char source_name[50];
  blean_t wall_msg;
  blean_t door_msg;
/*  char * temp_name;*/
  
  if (level == NULL)
    return;

  if (casualty != NULL)
    *casualty = false;
    
  seen = false;
  message = false;

  door_msg = false;
  wall_msg = false;

/*  if (e_type == expl_thrown)
  if (source)
  {
    temp_name = get_item_name(source);
    strcpy(source_name, temp_name);
    free(temp_name);
  }
  else*/
    strcpy(source_name, "something");

  /* Determine affected tiles. */
  area = new_area(center_y, center_x);
  area_ball(area, level, center_y, center_x, 0);

  /*
    Display affected tiles. If we saw any of them, set seen true so we
    will pause after the explosion.
  */
  if (area_of_effect(game->player, area, missile_explosion))
    seen = true;
    
  for (temp_area = area; temp_area != NULL; temp_area = temp_area->next)
  {
    y = temp_area->y;
    x = temp_area->x;

    if (get_tile(level, y, x) == tile_generator)
    {
      if (can_see(game->player, y, x))
	queue_msg("The generator is destroyed!");

      set_tile(level, y, x, tile_floor);
      build_forcefields(level);
    }

    if (is_weak_wall(level, y, x))
    {
      if (wall_or_door(level, y, x))
      {
	temp_area->param = 1;

	if (can_see(game->player, y, x))
	{
	  if (is_wall(level, y, x) && !wall_msg)
	  {
	    if (is_player(cause))
	      queue_msg("You destroy a wall!");
	    else
	      queue_msg("A wall collapses!");

	    wall_msg = true;
 	  }
	  else if (is_door(level, y, x) && !door_msg)
	  {
	    if (is_player(cause))
	      queue_msg("You destroy a door!");
	    else
	      queue_msg("A door is destroyed!");

	    door_msg = true;
 	  }
	}
      }
    }

    creature = find_creature(level, y, x);
      
    if (creature != NULL)
    {
      /* Check if the creature was hit! */
      
      reveal_mimic(creature);
      
      if (!is_helpless(creature) && (tslrnd() % 100 < attr_current(creature, attr_dodge)))
      {
	/* The creature dodged! */
	if (is_player(creature))
	{
	  queue_msg("You dodge the explosion!");
	  message = true;
	}
	else
	{
	  if (can_see(game->player, y, x))
	  {
	    msg_one(creature, "dodges the explosion!");
	    message = true;
	  }
	}
      }
      else
      {
	/* The creature was hit! */
	if (source)
	  damage_taken = random_damage(NULL, NULL, source);
	else
	  damage_taken = 1 + tslrnd() % EXPLOSION_DAMAGE;

	damage_taken = damage(creature, damage_armor(creature, damage_taken, damage_fire));
	
	if (is_player(creature))
	{
	  /* It was the player that was hit */
	  message = true;

	  /* If the explosion is the players own creation... */
	  if (cause == game->player)
	    queue_msg("You are caught in the explosion!");
	  else
	    queue_msg("You are caught in an explosion!");
	  
	  /* If the player is resistant... */
	  if (damage_taken == 0)
	    queue_msg("You shrug off the explosion!");
	  else if (damage_taken < 0)
	    queue_msg("You absorb the flames!");
	  
	  if (killed(creature))
	  {
	    queue_msg("You die...");
	    msgflush_wait();
	      
	    /*
	      This is so we don't lose a pointer (source has been
	      removed from the casters inventory) when we exit. This
	      will make the pointer (if any) back in use_power()
	      invalid, but it doesn't matter since we won't go back
	      there anyway.
	    */
	    del_item(source);
	    
	    if (cause == game->player)
	    {
	      switch (e_type)
	      {
		case expl_explosion:
		  check_for_player_death("were killed by an explosion");
		  break;
		  
		case expl_wand:
		  check_for_player_death("were killed by an exploding wand");
		  break;
		  
		case expl_book:
		  check_for_player_death("were killed by an exploding book");
		  break;
		    
		case expl_fireball:
		  check_for_player_death("fried yourself with a fireball");
		  break;
		    
		case expl_fired:
		case expl_thrown:
		  sprintf(line, "blew yourself up with %s", source_name);
		  check_for_player_death(line);
		  break;
		    
		case expl_booby_trap:
		  check_for_player_death("were killed by a booby trap");
		  break;
	      }
	    }
	    else if (cause != NULL)
	    {
	      switch (e_type)
	      {
		case expl_wand:
		  sprintf(line, "were killed by an exploding wand held by %s", cause->name_one);
		  break;
		  
		case expl_book:
		  sprintf(line, "were killed by an exploding wand held by %s", cause->name_one);
		  break;
		    
		case expl_booby_trap:
		case expl_explosion:
		  sprintf(line, "were killed by an explosion caused by %s", cause->name_one);
		  break;
		  
		case expl_fireball:
		  sprintf(line, "were fried by a fireball cast by %s", cause->name_one);
		  break;

		case expl_thrown:
		  sprintf(line, "were blown up by %s thrown by %s", source_name, cause->name_one);
		  break;

		case expl_fired:
		  sprintf(line, "were blown up by %s fired by %s", source_name, cause->name_one);
		  break;
	      }
	      
	      check_for_player_death(line);
	    }
	    else
	    {
	      switch (e_type)
	      {
		case expl_booby_trap:
		  check_for_player_death("were killed by a booby trap");
		  break;
		  
		case expl_wand:
		  check_for_player_death("were killed by an exploding wand");
		  break;
		  
		case expl_book:
		  check_for_player_death("were killed by an exploding book");
		  break;
		  
		case expl_explosion:
		  check_for_player_death("were killed by an explosion");
		  break;
		  
		case expl_fireball:
		  check_for_player_death("were killed by a fireball");
		  break;

		case expl_thrown:
		case expl_fired:
		  sprintf(line, "were killed by %s", source_name);
		  check_for_player_death(line);
		  break;
	      }
	    }
	  } /* killed */

	  add_anim(anim_type_damage, creature->uid, damage_taken, creature->y, creature->x);
	}
	else
	{
	  /* It was an NPC that was hit */
	    
	  if (can_see(game->player, y, x))
	  {
	    if (damage_taken > 0)
	    {
	      if (killed(creature))
	      {
		/* If the creature was killed... */
		msg_one(creature, "is consumed by flames!");
		message = true;
		
		if (creature == watch)
		{
		  if (casualty != NULL)
		    *casualty = true;
		}
	      }
	      else
	      {
		/* The creature was just hit, not killed */
		msg_one(creature, "gets burned!");
		message = true;
	      }
	    }
	    else
	    {
	      /* The creature was immune. */
	      msg_one(creature, "seems unaffected.");
	      message = true;
	    }
	  } /* visible */

	  add_anim(anim_type_damage, creature->uid, damage_taken, creature->y, creature->x);

	  if (killed(creature))
	  {		
	    kill_creature(creature, false); /* burning !=> blood */
	  }
	  else
	  {
	    creature_sleep(creature, false);
	    aggravate(creature);
	  }
	} /* NPC */
      } /* hit */
    } /* creature != NULL */

    if (expose_tile_to_fire(level, y, x) == true)
      message = true;
  } /* temp_area */

    
  for (temp_area = area; temp_area != NULL; temp_area = temp_area->next)
  {
    y = temp_area->y;
    x = temp_area->x;

    /* We do this as well in case there's a generator on either side */
    if (temp_area->param)
    {
      set_tile(level, y, x, tile_floor);
      build_forcefields(level);
    }
  }

  del_area(area);

  if (seen)
  {
    /* The explosion was seen */
    
    if (message == false)
    {
      /* No other message was printed, so lets print this at least */
      switch (e_type)
      {
	case expl_explosion:
	case expl_booby_trap:
	  queue_msg("It explodes!");
	  break;

	case expl_fireball:
	case expl_wand:
	case expl_book:
	case expl_thrown:
	case expl_fired:
	  /* Never mind; there will always be a "it explodes!" message first anyway */
	  break;
      }
    }
    
    msgflush_wait();
    clear_msgbar();

    draw_level();
  }
  else
  {
    queue_msg("You hear an explosion!");
  }
  
  /* Alert enemies on the level */
  draw_attention(level, center_y, center_x, noise_terrible);
  
  return;
} /* explosion */

