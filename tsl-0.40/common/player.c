#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <time.h>

#ifndef _WIN32
#include <pwd.h>
#include <unistd.h> /* for getlogin() */
#endif

#include "main.h"
#include "stuff.h"
#include "player.h"
#include "ui.h"
#include "level.h"
#include "game.h"
#include "debug.h"
#include "combat.h"
#include "inventory.h"
#include "item.h"
#include "creature.h"
#include "effect.h"
#include "actions.h"
#include "rules.h"
#include "traps.h"
#include "shapeshf.h"
#include "elements.h"
#include "fov.h"
#include "altitude.h"
#include "help.h"
#include "stacks.h"
#include "saveload.h"
#include "losegame.h"
#include "ability.h"
#include "missile.h"
#include "options.h"
#include "facet.h"
#include "wounds.h"
#include "backstab.h"
#include "memory.h"
#include "doors.h"
#include "burdened.h"
#include "pushing.h"
#include "find.h"
#include "equip.h"
#include "browser.h"
#include "bestiary.h"
#include "web.h"
#include "dwiminv.h"
#include "craft.h"


/*
#include "glyph.h"
*/

/*
  This is the players interface to the game.
*/
void player_control(creature_t * creature)
{
  int move_y;
  int move_x;
  dir_t dir;

  char line[80];

  item_t * item;

  unsigned int y;
  unsigned int x;

  action_t input;

  game->turns++;

  try_to_detect_enemies();
  try_to_detect_traps();

  /*
    There are "multiactions" that span over more than one turn. Only
    the player can rest, so we'll do it here rather than in the main
    game loop.
  */
  if (creature->multiaction == MULTIACTION_REST)
  {
    if (try_to_rest(creature) == false)
      return;
  }

  if (creature->multiaction == MULTIACTION_BREAK_REST)
  { 
    /*
      multi_param is how many turns we have rested so far and is
      incremented for each turn of successful resting.
    */
    if (creature->multi_param > 1)
      sprintf(line, "You rested for %d turns.", creature->multi_param);
    else
      sprintf(line, "You rested for %d turn.", creature->multi_param);
    
    queue_msg(line);
    
    creature->multi_param = 0;
    creature->multiaction = MULTIACTION_NONE;
  }

  /* Reset the history index at the start of every turn. */
  history_index = 0;

  /* Draw everything the player is currently aware of. */
  recenter_if_needed();
  draw_level();

  while (1)
  {
    /* We'll draw this inside the while loop in case we for example
     * detect a weapon is cursed; it needs to be updated. */
    /* TODO: Should this really be done every time? Couldn't we just
     * update it before the loop, then only if something happens that
     * needs it to be updated. */
    display_stats(creature);

    msgflush_nowait();
    input = get_action();
    clear_msgbar();
    
    /* Check which actions are allowed if the player is shapeshifted. */
    if (player_shapeshifted() == true)
    {
      blean_t allowed;

      allowed = true;

      switch(input)
      {
	case action_pickup:
	case action_fire:
	case action_equip:
	case action_remove:
	case action_drop:
	case action_inventory:
	case action_use:
	case action_throw:
	  if (attr_base(creature, attr_carrying_capacity) == 0)
	    allowed = false;
	  break;
	  
	  /* TODO: Unsure about this; in the future I think it's best
	     to the power list to creature_t, in which case we *want*
	     even shapeshifted creatures to be able to remap them. */
/*	case action_ability_config:
	  allowed = false;
	  continue;*/
	  
	default:
	  break;
      }

      if (allowed == false)
      {
	sprintf(line, "As %s, you cannot do that.", creature->name_one);
	queue_msg(line);
	continue;
      }
    }
    
    switch(input)
    {
      case action_filter:
	/* NOP! */
	continue;
      
      case action_cancel:
	history_index = 0;
	continue;

      case action_next:
      case action_previous:
      case action_undefined:
	continue;

      case action_select:
	if (dwim(creature))
	  return;
	else
	  continue;

      case action_recenter:
	center_view_y(creature->location, creature->y);
	center_view_x(creature->location, creature->x);
	draw_level();
	break;

      case action_quiver:
	quiver_cycle(creature);
	break;

      case action_flip:
	flip_status();
	continue;

      case action_status:
	display_summary(creature);
	draw_level();
	continue;

      case action_help:
	help();
	clear_msgbar();
	draw_level();
	continue;

      case action_interact:
	queue_msg("In which direction?");
	msgflush_nowait();

	dir = get_direction();

	if (dir == dir_none)
	{
	  clear_msgbar();
	  continue;
	}
	else
	{
	  creature_t * target;
	  unsigned int new_y;
	  unsigned int new_x;

	  dir_to_speed(dir, &move_y, &move_x);

	  new_y = creature->y + move_y;
	  new_x = creature->x + move_x;
	  
	  if (on_map(creature->location, new_y, new_x) == false)
	  {
	    queue_msg("BUG: Interacting outside map, this shouldn't happen. WHAT HAVE YOU DONE!?");
	    continue;
	  }

	  if (can_see(creature, new_y, new_x) == false)
	  {
	    queue_msg("It's dark...");
	    continue;
	  }

	  target = find_creature(creature->location, new_y, new_x);
	  
	  if (target &&
	      target->detected &&
	      target->ai_state != ai_mimic)
	  {
	    interact(target);
	    return;
	  }
	  else
	  {
	    queue_msg("There is nobody there.");
	    continue;
	  }
	}
	continue;
	
      case action_close:
	if (close_door(creature))
	  return;
	continue;

      case action_rest:
	if (needs_rest(creature, true) == false)
	{
	  continue;
	}

	if (can_see_anyone() == true)
	{
	  queue_msg("Something is near!");
	  continue;
	}

	creature->multi_param = 1;
	creature->multiaction = MULTIACTION_REST;
	return;
	
      case action_ability_config:
	map_to_shortcut(-1);
	clear_msgbar();
	continue;

      case action_ability_1:
      case action_ability_2:
      case action_ability_3:
      case action_ability_4:
      case action_ability_5:
      case action_ability_6:
      case action_ability_7:
      case action_ability_8:
      case action_ability_9:
      case action_ability_10:
      case action_ability:
	if (1)
	{
	  signed int ability_index;
	  signed int ability;
	  
	  if (input == action_ability)
	  {
/* 	    queue_msg("Use which ability?");*/
	    ability = -1;
	  }
	  else
	  {
	    ability_index = action_to_ability(input);
	    ability = game->ability_slot[ability_index];
	  }
	  
	  if (ability_menu(creature, ability))
	    return;
	  
	  continue;
	}
	    
/*	  if (ability != -1)
	  {*/
/*	    if ((get_power_reagent(wer) != -1) &&
		(find_item_in_inventory(creature, get_power_reagent(power)) == NULL))
	    {
	      queue_msg("You don't possess the proper reagent.");
	      continue;
	      }*/
/*	    
	    if (get_ability_cost(creature, ability) > attr_base(creature, attr_ep_current))
	    {
	      queue_msg("You don't have enough energy!");
	      continue;
	    }
	    
	    if (invoke_ability(creature, ability, NULL) == true)
	    {
	      clear_msgbar();
	      return;
	    }
	    else
	      continue;
	  }
	}
	continue;*/

      case action_pickup:
/*	if (pickup(creature, NULL, false))
	return;*/

	if (dwim_pickup(creature))
	  return;
	    
	continue;
	
	/* Object-verb inventory */
      case action_inventory:
	if (dwim_inventory(creature, NULL, NULL))
	  return;
	else
	  continue;

        /*
	  This is an attempt to create a very general way to handle
	  all verb-object inventory operations.
	*/
      case action_equip:
      case action_remove:
      case action_drop:
      case action_throw:
      case action_drink:
      case action_read:
      case action_eat:
      case action_use:
      case action_label:
      case action_apply:
	if (inventory_action(creature, input))
	  return;
	else
	  continue;
	
      case action_wait:
	queue_msg("You wait.");
	display_stats(creature);
	return;

      case action_fire: 
	/* Prompt the player for a direction to fire in. */
	/* We won't allow throwing here since it already has its own action. */
	
	if (can_fire_weapon(creature, true))
	{
	  char * item_name;
	  
	  item = get_item_from_stack(get_equipped(creature, item_type_ammo, 0));
	  
	  item_name = get_item_name(item);
	  sprintf(line, "Fire %s in which direction?", item_name);
	  free(item_name);
	  
	  queue_msg(line);
	  msgflush_nowait();
	
	  dir = get_direction();
	  
	  if (dir == dir_none)
	  {
	    clear_msgbar();
	    continue;
	  }

	  throw_or_fire(creature, dir);
	}

	return;
	
      case action_fire_n:
      case action_fire_ne:
      case action_fire_e:
      case action_fire_se:
      case action_fire_s:
      case action_fire_sw:
      case action_fire_w:
      case action_fire_nw:

	if (input == action_fire_n)        { dir = dir_n; }
	else if (input == action_fire_ne)  { dir = dir_ne; }
	else if (input == action_fire_e)   { dir = dir_e; }
	else if (input == action_fire_se)  { dir = dir_se; }
	else if (input == action_fire_s)   { dir = dir_s; }
	else if (input == action_fire_sw)  { dir = dir_sw; }
	else if (input == action_fire_w)   { dir = dir_w; }
	else if (input == action_fire_nw)  { dir = dir_nw; }
	
	if (shift_action(creature, dir) == false)
	  continue;

	return;
	
      case action_history:
	history();
	continue;
	
      case action_debug:
	debug_menu();
	clear_msgbar();
	continue;
	
      case action_redraw:
	build_fov(creature);

	st_erase();
	mb_erase();
	map_erase();

	mb_flush();
	st_flush();

	draw_level();
	display_stats(creature);
	continue;
	
      case action_version:
	sprintf(line, "You are playing %s %s.", TSL_NAME, TSL_VERSION);
	queue_msg(line);

	#ifdef TSL_SDL
	queue_msg("Uses the Simple DirectMedia Layer.");
        #endif
	continue;
	
      case action_stairs:
	if (stairs(creature, true))
	  return;

	continue;

      case action_save:
	try_to_save_game();
	break;

      case action_options:
	options_menu();
	break;
	
      case action_inspect:
	queue_msg("Select the tile you wish to inspect...");
	msgflush_nowait();
	
	if (get_position_on_map(creature->location, &y, &x))
	{
	  look_at_location(creature, creature->location, y, x, true);
	}
	continue;
	
      case action_quit:
	if (prompt_yn("Really quit?") >= 1)
	{
	  game->game_over = true;
	  check_for_player_death("quit");
	}
	else
	  continue;
	
	/* Movement */
      case action_n:
      case action_e:
      case action_s:
      case action_w:
      case action_ne:
      case action_nw:
      case action_se:
      case action_sw:
	if (input == action_nw) { move_y = -1; move_x = -1; }
	else if (input == action_n) { move_y = -1; move_x =  0; }
	else if (input == action_ne) { move_y = -1; move_x =  1; }
	else if (input == action_w) { move_y =  0; move_x = -1; }
	else if (input == action_e) { move_y =  0; move_x =  1; }
	else if (input == action_sw) { move_y =  1; move_x = -1; }
	else if (input == action_s) { move_y =  1; move_x =  0; }
	else if (input == action_se) { move_y =  1; move_x =  1; }

	if (on_map(creature->location, creature->y + move_y, creature->x + move_x) == false)
	{
	  queue_msg("BUG: Player tried to move outside map.");

	  continue;
	}
	else if (try_to_move(creature, move_y, move_x))
	  return;

	continue;
    } /* switch */
  } /* while 1 */
  
  return;
} /* player_control */



/*
  "Do what I mean". Performs a default action where CREATURE is
  standing.
*/
blean_t dwim(creature_t * creature)
{
  level_t * level;
  
  if (creature == NULL || (level = creature->location) == NULL)
    return false;

  if (pickup(creature, NULL, true))
    return true;

  if (stairs(creature, false))
    return true;

  queue_msg("You wait.");
  display_stats(creature);

  return true;
} /* dwim */



/*
  Returns 1: turn complete, 0: continue, 3: break
 */
unsigned int inventory_action(creature_t * creature, const action_t input)
{
  char line[80];
  char * temp_str;
  char order[30];
  item_t * item;
  item_t * temp_item;
  signed int temp;

  if (creature == NULL)
    return 0;
  
  item = NULL;

  strcpy(order, "ABamr");
  
  if (input == action_eat)
  {
    if (attr_current(creature, attr_p_eat))
    {
      queue_msg(MSG_UNABLE_TO_EAT);
      return 0;
    }
    
    if (is_swimming(creature) == true)
    {
      queue_msg("Not while under water!");
      return 0;
    }
    
    /* We can't eat from the floor if we're floating. */
    /* RFE: Is there a way to state this fact without flushing an extra message? */
    if (is_floating(creature) == false)
    {
      for (temp_item = creature->location->first_item;
	   temp_item != NULL;
	   temp_item = temp_item->next_item)
      {
	if (is_edible(temp_item) &&
	    (temp_item->y == creature->y) &&
	    (temp_item->x == creature->x))
	{
	  /* Display a different prompt depending on how many there are */
	  temp = stack_size(temp_item);
	  temp_str = get_item_name(temp_item);
	  
	  if (temp > 1)
	    sprintf(line, "There are %s here; eat one?", temp_str);
	  else
	    sprintf(line, "There is %s here; eat it?", temp_str);
	  
	  free(temp_str);
	  temp_str = NULL;
	  
	  temp = prompt_yn(line);
	  
	  if (temp >= 1)
	  {
	    item = temp_item;
	    break;
	  }
	  else if (temp == -1)
	  {
	    break;
	  }
	}
      } /* for */
    } /* floating */
  }
  
  /* Have we already selected an item? (i.e. one lying on the floor) */
  if (item == NULL)
  {
    blean_t (* criteria) (const item_t *);
    
    criteria = NULL;
    
    /* If the player isn't carrying any items, we can't do anything :-( */
    if (game->player->first_item == NULL)
    {
      queue_msg("You are not carrying any items.");
      return 0;
    }
    
    if (input == action_drop)
    {
      strcpy(line, "What do you want to drop?");
    }
    else if (input == action_throw)
    {
      if (attr_current(creature, attr_p_throw))
      {
	queue_msg("You are unable to throw!");
	return 0;
      }
      
      if (is_swimming(creature) == true)
      {
	queue_msg("Not while under water!");
	return 0;
      }
      
      strcpy(line, "What do you want to throw?");
      criteria = NULL;
      strcpy(order, "apm");
    }
    else if (input == action_use)
    {
      strcpy(line, "What do you want to use?");
      criteria = &can_activate;
      strcpy(order, "wt");
      /*strict = true;*/
    }
    else if (input == action_read)
    {
      if (is_blinded(creature))
      {
	queue_msg("You cannot see!");
	return 0;
      }
      
      if (attr_current(creature, attr_p_read))
      {
	queue_msg("You are unable to read!");
	return 0;
      }
      
      strcpy(line, "What do you want to read?");
      criteria = &is_readable;
      strcpy(order, "sb");
      /*strict = true;*/
    }
    else if (input == action_drink)
    {
      if (attr_current(creature, attr_p_drink))
      {
	queue_msg(MSG_UNABLE_TO_DRINK);
	return 0;
      }
      
      if (is_swimming(creature) == true)
      {
	queue_msg("Is it not wet enough here already...");
	/*continue;*/
      }
      
      strcpy(line, "What do you want to drink?");
      criteria = &is_drinkable;
      strcpy(order, "p");
    }
    else if (input == action_label)
    {
      if (attr_current(game->player, attr_p_read))
      {
	queue_msg("Unfortunately, you can not read (or write).");
	return 0;
      }
      
      strcpy(line, "Label which item?");
    }
    else if (input == action_eat)
    {
      strcpy(line, "What do you want to eat?");
      criteria = &is_food;
    }
    else if (input == action_apply)
    {
      strcpy(line, "What do you want to apply?");
      criteria = &can_apply;
    }
    else if (input == action_equip)
    {
      strcpy(line, "What do you want to equip?");
      criteria = &is_equipable;
    }
    else if (input == action_remove)
    {
      strcpy(line, "What do you want to remove?");
      criteria = &is_equipped;
    }
    
    if (count_inventory(creature, criteria) == 0)
    {
      /*
	This shouldn't happen if we have a NULL criteria, since
	that means *any* item and we've already caught that
	earlier if we indeed have no items at all. We will only
	list the cases *with* a criteria here.
      */
      
      switch (input)
      {
	case action_equip:
	  queue_msg("You have nothing to equip.");
	  break;
	  
	case action_remove:
	  queue_msg("You have nothing to unequip.");
	  goto break_inv_action;
	  
	case action_eat:
	  queue_msg("You do not have any food.");
	  criteria = NULL;
	  goto cont_inv_action;
	  
	case action_read:
	  queue_msg("You have nothing to read.");
	  goto break_inv_action;
	  
	case action_drink:
	  queue_msg("You have nothing to drink.");
	  goto break_inv_action;
	  
	case action_use:
	  queue_msg("You have nothing to use.");
	  goto break_inv_action;
	  
	case action_apply:
	  queue_msg("You have nothing to apply.");
	  goto break_inv_action;
	  
	default:
	  break;
      }
    }
    
    goto cont_inv_action;
    
  break_inv_action:
    msgflush_nowait();
    return 0;
    
  cont_inv_action:
    queue_msg(line);
    msgflush_nowait();
    item = dwim_select(creature, criteria, order);
/*    item = select_inventory_item(creature, criteria, strict, order);*/
  }
  
  /* We should have decided on an item by now. */
  
  /* Did the user cancel the operation? */
  if (item == NULL)
  {
    clear_msgbar();
    return 0;
  }
  
  /* What was it the user wanted to do? */
  if (input == action_drop) 
  {
    unsigned int quantity;
    
    quantity = stack_size(item);
    
    if (quantity == 1)
    {
      try_to_unequip(item, false);
      drop_item(creature, item, false);
    }
    else
    {
      if (drop_stack(creature, item) == 0)
	return 0;
    }

    clear_msgbar();
    return 1;
  }
  else if (input == action_throw) 
  {
    if (throw_prompt(creature, item))
      return 1;
    else
      return 0; /* cancelled; no turn cost */
  }
  else if (input == action_equip)
  {
    try_to_equip(item);
    return 0;
  }
  else if (input == action_remove)
  {
    try_to_unequip(item, true);
    return 0;
  }
  else if (input == action_eat)
  {
    if (eat(creature, item))
      return 1;
    else
      return 0;
  }
  else if (input == action_apply)
  {
    if (apply(creature, item))
      return 1;
    else
      return 0;
  }
  else if (input == action_use ||
	   input == action_drink ||
	   input == action_read)
  {
    if (use_item(creature, item) == true)
      return 1;
    else
      return 0; /* couldn't use */
  }
  else if (input == action_inventory)
  {
    // TODO: I'm not sure when this happens
/*    if (item_browser(creature, item))
      return 1;
    else
    return 0;*/
  }
  else if (input == action_label)
  {
    label_prompt(item);
    return 0;
  }

  return 0;
} /* inventory_action */



/*
  Describes what (creatures, items, special floor tiles, etc) is at a
  certain location.
*/
void look_at_location(const creature_t * observer, const level_t * level,
		      const unsigned int y, const unsigned int x,
		      const blean_t forced)
{
  item_t ** item_list;
  item_t * item;
  trap_t * trap;
  unsigned int items;
  blean_t same_location;
  blean_t dark;
  creature_t * creature;
  char line[80];
  unsigned int temp;

  if (observer == NULL ||
      level == NULL ||
      y >= level->size_y ||
      x >= level->size_x)
  {
    return;
  }

  /* Is the observer standing on the same spot? */
  if (observer->y == y &&
      observer->x == x &&
      observer->location == level)
  {
    same_location = true;
  }
  else
  {
    same_location = false;
  }
  
  /* We haven't printed any message yet. */
  dark = true;

  /*
    Find out if there is any creature on the square. We are *not*
    interested in the player, as we already should know where it
    is.
  */
  creature = find_creature(level, y, x);
  
  /* Is there any reason to not display the presence of this creature? */
  if (creature == NULL ||
      creature == game->player ||
      creature->detected == false ||
      can_see_creature(game->player, creature) == false)
  {
    creature = NULL;
  }

  /* Check if there is a stair there. These should always be displayed. */

  /*
    RFE: Add a way to suppress this when descending so it doesn't
    show "there is a stair here leading up..." directly on entering a
    new level. I'm trying not to add too many parameters.
  */

  if (level->memory[y][x] == gent_stairs &&
      (level->map[y][x] == tile_stair0 ||
       level->map[y][x] == tile_stair1 ||
       level->map[y][x] == tile_stair2 ||
       level->map[y][x] == tile_stair3))
  {
    if (same_location)
      strcpy(line, "Here is a stair leading to ");
    else
      strcpy(line, "A stair leading to ");

    temp = level->link[level->map[y][x] - tile_stair0];

    if (temp < 0 || temp >= LEVELS || game->level_list[temp] == NULL)
      strcat(line, "BUG");
    else
      strcat(line, game->level_list[temp]->name);

    strcat(line, ".");
    queue_msg(line);

    dark = false;
  }
  
  /* Find all items there */
  items = find_items(level, y, x, &item_list);
  
  /*
    Display terrain. This should only be done if it's within sight of
    the observer, and a) there is any creature or item on top of it
    (that's why we had to check for those first), or b) the location
    is different from that of the observer
  */
  if (can_see(observer, y, x))
  {
    /* We can see this spot, so we shouldn't display "It's dark". */
    dark = false;

    if (items >= 1 ||
	same_location == false ||
	creature != NULL ||
	forced)
    {
      if (is_wall(level, y, x))
      {
	/* This should _not_ happen for "here". */
	queue_msg("A wall.");
      }
      else
      {
	switch (level->map[y][x])
	{
	  case tile_pentagram:
	    queue_msg("A pentagram.");
	    break;
	    
	  case tile_terminal:
	    queue_msg("A computer terminal.");
	    break;
	    
	  case tile_capsule:
	    queue_msg("An augmentation capsule.");
	    break;
	    
	  case tile_door_closed:
	    queue_msg("A closed door.");
	    break;
	    
	  case tile_door_open:
	    queue_msg("An open door.");
	    break;
	    
	  case tile_block:
	    queue_msg("A block.");
	    break;
	    
	  case tile_lava:
	    queue_msg("Lava.");
	    break;
	      
	  case tile_forcefield:
	    queue_msg("A forcefield.");
	    break;
	    
	  case tile_generator:
	    queue_msg("A forcefield generator.");
	    break;
	    
	  case tile_water:
	    queue_msg("Water.");
	    break;
	    
	  default:
	    break;
	}
      } /* not wall */
    }
  }
  
  /* If we can see the spot, display the items that are there. */
  if (can_see(observer, y, x))
  {
    /*
      If there is only a single item, display its name. If there are
      more, display "there are N items here" instead.
    */
    if (items == 1)
    {
      char * name;
      
      item = item_list[0];
      
      name = get_item_name(item);

/*      if (same_location)
      {
	sprintf(line, "There %s %s %s.",
		item_article_is[get_item_article(item)],
		name,
		(same_location ? "here" : "there"));
      }
      else
      {
      }*/

      sprintf(line, "%s.", name);
    
      upperfirst(line);
      free(name);
      name = NULL;
      
      queue_msg(line);
    }
    else if (items > 4)
    {
      if (same_location)
	queue_msg("There are many items here.");
      else
	queue_msg("There are many items there.");
    }
    else if (items > 1)
    {
      unsigned int i;
      char * name;
      
      for (i = 0; i < items; i++)
      {
	item = item_list[i];
	
	name = get_item_name(item);
	
	sprintf(line, "%s.", name);
	
	free(name);
	name = NULL;
	
	line[0] = toupper(line[0]);
	
	queue_msg(line);
      }
    }
  }
    
  free(item_list);

  /*
    Is there a trap there? We won't display any "there is a trap
    _here_", since the player will already have triggered it by then.
    We just display this message when * looking from a distance.
  */

  /*
    RFE: Figure out how we can display this when _looking_ at our own
    location, but leave it out when just landing on that tile.
  */

  if (can_see(observer, y, x))
  {
    trap = find_trap(level, y, x);

    if (trap != NULL &&
	trap->revealed == true &&
	trap->hidden == false &&
	(!same_location || forced))
    {
      switch (trap->type)
      {
	case trap_booby:
	  sprintf(line, "A booby trap.");
	  break;
	  
	case trap_dart:
	  sprintf(line, "A dart trap.");
	  break;
	  
	case trap_blink:
	  sprintf(line, "A blink trap.");
	  break;
	  
	case trap_glass:
	  sprintf(line, "Some broken glass.");
	  break;
	  
	case trap_plate:
	  sprintf(line, "An electrified plate.");
	  break;
	  
	case trap_polymorph:
	  sprintf(line, "A polymorph trap.");
	  break;
	  
	case trap_flash:
	  sprintf(line, "A flash trap.");
	  break;
	  
	case trap_web:
	  sprintf(line, "A web.");
	  break;
	  
	case trap_medkit:
	  sprintf(line, "A medkit.");
	  break;
	  
	case trap_teleporter:
	  sprintf(line, "A teleporter.");
	  break;
	  
	case trap_win:
	  sprintf(line, "The path to ascension.");
	  break;
	  
	default:
	  sprintf(line, "A trap.");
	  break;
      } /* switch */
      
      queue_msg(line);
    }
  } /* can_see */
    
  /* Display any creature located there */
  if (creature != NULL)
  {
    strcpy(line, creature->name_one);

    if (get_effect_by_id(creature, effect_sleep))
      strcat(line, " (asleep).");
    else
      strcat(line, ".");

    upperfirst(line);
    queue_msg(line);
    
    if (creature->ai_state != ai_mimic &&
	prompt_yn("View monster description?") >= 1)
    {
      bestiary(creature->id);
    } 

    dark = false;
  }

  /* If we didn't come up with anything else to print. */
  if (dark)
  {
    queue_msg("It's dark...");
    return;
  }

  return;
} /* look_at_location */



/*
  Runs the "create character" procedure.
*/
creature_t * create_character()
{
  unsigned int i;
  item_t * temp;

  creature_t * new_player;

  /* Set up some initial stuff */
  new_player = alloc_creature();

  new_player->multiaction = MULTIACTION_NONE;

  /* We should see ourself */
  new_player->detected = true;

  /* This is just to avoid confusion */
  new_player->ai_state = ai_player;

  /* Set up basic stats */
  set_attr(new_player, attr_player_ally, 1);

  set_attr(new_player, attr_speed,      BASE_SPEED);
  set_attr(new_player, attr_perception, 3);
  set_attr(new_player, attr_stealth,    3);
  set_attr(new_player, attr_magic,      1);
  set_attr(new_player, attr_attack,     80);
  set_attr(new_player, attr_dodge,      3);
  set_attr(new_player, attr_vision,     1);
  set_attr(new_player, attr_throw_range,3);

  set_attr(new_player, attr_health,     20);

  set_attr(new_player, attr_ep_max,     3);
  set_attr(new_player, attr_ep_current, 3);

  set_creature_name(new_player, "player", "player", "player"); /* Fallback, should never be used */
  new_player->gent = gent_player;
  new_player->id = monster_player;

  /*
    If forcegetname is set in the config file, we will always ask for
    the characters name at this point. We always do this on Windows
    unless we get a name from the config file.
  */
  #ifdef _WIN32
  if (options.default_name == NULL)
    options.forcegetname = true;
  #endif

  if (options.forcegetname)
  {
    char new_name[30];
    
    /* Get character name. Keep reading a new string until the user
       enters something valid. */
    do
    {
      read_string("Character name?", new_name, 29);
    }
    while (strlen(new_name) < 1);
    
    set_creature_name(new_player, new_name, new_name, new_name);
  }
  else if (options.default_name != NULL)
  {
    /* If there is a default name set in the config file, use that. */
    set_creature_name(new_player, options.default_name, options.default_name, options.default_name);
  }
  else
  {
    #ifndef _WIN32
    /* Try to get the username of the player */
    if (getlogin() != NULL)
      set_creature_name(new_player, getlogin(), getlogin(), getlogin());
    else
    {
      struct passwd * pwd;
      
      pwd = getpwuid(getuid());
      
      if (pwd != NULL)
	if (pwd->pw_name != NULL)
	  set_creature_name(new_player, pwd->pw_name, pwd->pw_name, pwd->pw_name);
    }
    #endif
  } /* auto-choose name */

  map_erase();
  map_flush();

  /* Initial equipment */
  temp = build_item(treasure_shotgun);
  use_or_destroy(new_player, temp);

  temp = build_item(treasure_s_buckshot);
  for (i = 0; i < 11; i++)
    attach_item_to_stack(temp, build_item(treasure_s_buckshot));
  use_or_destroy(new_player, temp);

  /* Make sure the starting torch is _full_. */
  temp = build_item(treasure_torch);
  temp->custom[CUSTOM_LIGHT_TICKS] = temp->custom[CUSTOM_LIGHT_MAX_TICKS] - 1;
  use_or_destroy(new_player, temp);

  /* Identify all initial equipment */
  for (temp = new_player->first_item; temp != NULL; temp = temp->next_item)
    identify(temp);

/*
  set_attr(new_player, attr_ep_max, 100);
  set_attr(new_player, attr_vision, 10);
  set_attr(new_player, attr_m_mudball, 10);
  set_attr(new_player, attr_m_force_bolt, 10);

  for (i = 0; i < 30; i++)
    attach_item_to_creature(new_player, build_item(treasure_grenade));
*/

/*    attach_item_to_creature(new_player, build_item(treasure_plain_boring_sword));
    attach_item_to_creature(new_player, build_item(treasure_plain_boring_sword));
    attach_item_to_creature(new_player, build_item(treasure_plain_boring_sword));
    attach_item_to_creature(new_player, build_item(treasure_plain_boring_sword));
    attach_item_to_creature(new_player, build_item(treasure_plain_boring_sword));
    attach_item_to_creature(new_player, build_item(treasure_plain_boring_sword));
    attach_item_to_creature(new_player, build_item(treasure_plain_boring_sword));
    attach_item_to_creature(new_player, build_item(treasure_plain_boring_sword));
    attach_item_to_creature(new_player, build_item(treasure_plain_boring_sword));
    attach_item_to_creature(new_player, build_item(treasure_plain_boring_sword));
    attach_item_to_creature(new_player, build_item(treasure_plain_boring_sword));
    attach_item_to_creature(new_player, build_item(treasure_plain_boring_sword));
    attach_item_to_creature(new_player, build_item(treasure_plain_boring_sword));
    attach_item_to_creature(new_player, build_item(treasure_plain_boring_sword));
    attach_item_to_creature(new_player, build_item(treasure_plain_boring_sword));
    attach_item_to_creature(new_player, build_item(treasure_plain_boring_sword));
    attach_item_to_creature(new_player, build_item(treasure_plain_boring_sword));
    attach_item_to_creature(new_player, build_item(treasure_plain_boring_sword));
    attach_item_to_creature(new_player, build_item(treasure_plain_boring_sword));
    attach_item_to_creature(new_player, build_item(treasure_s_recall));
    attach_item_to_creature(new_player, build_item(treasure_s_amnesia));
    attach_item_to_creature(new_player, build_item(treasure_dataprobe));
    attach_item_to_creature(new_player, build_item(treasure_elixir));*/

/*  set_attr(new_player, attr_a_hide, 1);
    set_attr(new_player, attr_m_flash, 1);*/

/*  unmapped_abilities(NULL);*/

  return new_player;
} /* create_character */



blean_t try_to_move(creature_t * creature,
		    signed int move_y, signed int move_x)
{
  unsigned int new_y;
  unsigned int new_x;
  unsigned int old_y;
  unsigned int old_x;
  blean_t target_gone;
  creature_t * obstacle;
  level_t * level;
  blean_t displace;
  blean_t see_new;
  tile_t new_tile;
  tile_t old_tile;
/*  effect_t * eff;*/
  char line[100];

  if (creature == NULL)
    return false;

  level = creature->location;

  if (level == NULL)
    return false;

  if ((move_y == 0) && (move_x == 0))
    return false;

  old_y = creature->y;
  old_x = creature->x;
  
  new_y = old_y + move_y;
  new_x = old_x + move_x;

  old_tile = get_tile(level, old_y, old_x);
  new_tile = get_tile(level, new_y, new_x);
  see_new = can_see(game->player, new_y, new_x);
  obstacle = find_creature(level, new_y, new_x);

  /*
    When stumbling blindly, we will auto-attack any creatures
    encountered. If we can see and bump into a yet undetected
    creature, the turn will be spent. Fair?
  */

  if (obstacle == NULL)
    displace = false;
  else if (attr_current(obstacle, attr_player_ally))
    displace = true;
  else
  {
    /*
      The destination is occupied. If we can see/sense the creature in
      any way, this will override that we can't see the tile itself.
    */
    if (can_see_creature(creature, obstacle))
      see_new = true;

    if (see_new == false)
      queue_msg("Something blocks your way!");

    if (obstacle->detected == false)
    {
      obstacle->detected = true;

      if (see_new)
      {
	sprintf(line, "You bump into %s!", obstacle->name_one);
	queue_msg(line);
	draw_level();
	msgflush_wait();
	clear_msgbar();
      }

      if (is_blinded(creature) == false)
	return true;
    }

    /* Carry out the attack, but not if we're under water! */
    /* TODO: Should it be possible to fight under water or not? */

/*    if (is_swimming(creature) == false)
      {
      }
      else
      {
        if (IS_SEEN(new_y, new_x) == true)
          queue_msg("You cannot fight under water!");
      
        return 0;
      }*/

    if (creature->attack_pos < 3)
    {
      queue_msg("You can not attack yet!");
      return false;
    }

    if (backstab(creature, obstacle) == false)
      attack(creature, obstacle);
    
    draw_level();
    
    return true;
  }
  
  /*
    There wasn't any enemy there - try to move there.
  */
  if (get_effect_by_id(creature, effect_web))
  {
    struggle_web(creature);
    return true;
  }
  else if (attr_current(creature, attr_p_move))
  {
    queue_msg("You cannot move! You will have to wait.");
    return false;
  }
  else if (new_tile == tile_water)
  {
    set_creature_coordinates(creature, new_y, new_x);
    change_altitude(creature);
    look_at_location(creature, level, new_y, new_x, false);
    draw_level();
  }
  else if (new_tile == tile_lava || new_tile == tile_forcefield)
  {
    /*
      There's lava or a forcefield here! If the player can see, prompt
      if they really want to enter the field - except if they already
      are in such a field. If the player can't see, they will just
      stumble into it. Being blind comes before the prompt, thus
      overrides it.
    */

    if (new_tile == tile_forcefield)
      strcpy(line, "Step into the forcefield?");
    else
      strcpy(line, "Step into the lava?");

    if (new_tile == tile_lava && is_floating(creature))
    {
      /* Since we're floating, we don't care about the lava! */
      set_creature_coordinates(creature, new_y, new_x);
      look_at_location(creature, level, new_y, new_x, false);
      draw_level();
    }
    else if ((old_tile == tile_lava && new_tile == tile_lava) ||
	     (old_tile == tile_forcefield && new_tile == tile_forcefield) ||
	     see_new == false ||
	     prompt_yn(line) >= 1)
    {
      if (old_tile != tile_lava && new_tile == tile_lava)
      {
	if (see_new == false)
	  queue_msg("You step into lava!");
	else
	  queue_msg("You step into the lava!");
      }
      else if (old_tile != tile_forcefield && new_tile == tile_forcefield)
      {
	if (see_new == false)
	  queue_msg("You step into a forcefield!");
	else
	  queue_msg("You step into the forcefield!");
      }
      
      set_creature_coordinates(creature, new_y, new_x);
      creature->altitude = altitude_walking;

      if (new_tile == tile_lava)
	lava_bath(creature);
      else
	forcefield(creature);

      /*look_at_location(creature, level, new_y, new_x, false);*/
      draw_level();
    }
    else
      return false; /* Cancel; we don't want to step into lava. */
  }
  else if (level->map[new_y][new_x] == tile_capsule)
  {
    capsule(creature, new_y, new_x);
    
    return true;
  }
/*  else if (is_pushable(level, new_y, new_x) &&
	   push(creature, NULL, speed_to_dir(move_y, move_x)))
  {
    update_level_memory(level);
    draw_level();
    return true;
    }*/
  else if (level->map[new_y][new_x] == tile_door_closed ||
	   level->map[new_y][new_x] == tile_door_locked)
  {
    if (see_new)
      return open_door(creature, new_y, new_x);
    else
    {
      if (level->memory[new_y][new_x] == gent_door_closed)
      {
	return open_door(creature, new_y, new_x);
      }
      else
      {
	queue_msg("You find a door.");
	level->memory[new_y][new_x] = gent_door_closed;
	return true;
      }
    }
  }
  else if (level->map[new_y][new_x] == tile_door_secret_v ||
	   level->map[new_y][new_x] == tile_door_secret_h)
  {
    /*
      If there's a secret door in the way; reveal it.

      Finding a secret door while blind takes two tries. The player
      must first bump it to reveal "the obstacle" (in else clause),
      then bump it again to reveal the door (this time the gent won't
      be blank).
    */

    if (level->memory[new_y][new_x] != gent_blank)
    {
      queue_msg("You find a secret door!");

      /*
	This is a bit of a hack. I'm not all for making logic
	decisions in the UI code, but doing it elsewhere would require
	us to keep two types of tile_secret_locked_door_* as well.
      */
      maybe_locked_door(level, new_y, new_x);

      /* They will look the same either way */
      level->memory[new_y][new_x] = gent_door_closed;
    }
    else
    {
      if (is_floating(creature) == true)
	queue_msg("You cannot fly there.");
      else if (is_swimming(creature) == true)
	queue_msg("You cannot swim there.");
      else
	queue_msg("You cannot go there.");
      
      if (see_new == false && level->memory[new_y][new_x] == gent_blank)
      {
	level->memory[new_y][new_x] = gent_obstacle;
	update_level_memory(level);
	draw_level();
	return false;
      }
    }
    
    /* Easiest way to rebuild FOV. */
    /*set_creature_coordinates(creature, creature->y, creature->x);*/
    
    /* Update that we've seen the door. */
    draw_level();
    
    return true;
  }
  else if (is_walkable(creature->location, false, new_y, new_x) == false &&
	   obstacle == NULL)
  {
    /* There isn't anyone there - nothing more to do here */
    if (is_floating(creature) == true)
      queue_msg("You cannot fly there.");
    else if (is_swimming(creature) == true)
      queue_msg("You cannot swim there.");
    else
      queue_msg("You cannot go there.");
    
    /*
      Record an obstacle in level memory. Only do this if we currently
      "know" a blank tile, or a floor tile - it could be a generator
      that has been pushed there (also while blinded). This is getting
      complicated.
    */
    if (can_see(game->player, new_y, new_x) == false &&
	(level->memory[new_y][new_x] == gent_blank ||
	 level->memory[new_y][new_x] == gent_floor))
    {
      level->memory[new_y][new_x] = gent_obstacle;
      update_level_memory(level);
      draw_level();
      return false;
    }

    return false;
  }

  /* If there's any ally there that needs to be displaced... */
  if (displace)
  {
    set_creature_coordinates(obstacle, old_y, old_x);
  }

  /* Now that the way has been cleared, move. */
  set_creature_coordinates(creature, new_y, new_x);

  if ((level->map[new_y][new_x] != tile_water) &&
      (creature->altitude == altitude_swimming))
  {
    creature->altitude = altitude_walking;
  }

  target_gone = false;

  /* This only happens if the creature isn't floating. */
/*  if (is_floating(creature) == false)
    {*/
    /* Spring any traps we might have stepped on... */
    target_gone = activate_trap(find_trap(level, new_y, new_x));
/*  }*/

  /* Deal damage if we're wounded. */
  /* TODO: Should this happen if we're floating? */
  if (target_gone == false)
    target_gone = wound_damage(creature);
  
  /* Display what's on the ground. */
  if (target_gone == false)
    look_at_location(creature, level, creature->y, creature->x, false);

  recenter_if_needed();
 
  /* Redraw the map */
  draw_level();

  return true;
} /* try_to_move */



void display_summary(creature_t * creature)
{
/*  key_token_t input;*/
/*  action_t action;*/
  char aug_index;
/*  char facet_index;*/
  item_t * item;
/*  char temp[800];*/
/*  char line[100];*/
/*  blean_t has_any;*/
  item_t * list[30];
  menu_item_t ** menu_list;
  int count;
    
/*  queue_msg("This feature doesn't really work right now.");
  msgflush_wait();
  return;*/

/*list_stuff:*/
/*  st_erase();*/

  aug_index = 'a';
/*    facet_index = 'k';*/

/*  strcpy(temp, "");*/

  for (count = 0; count < 30; count++)
    list[count] = NULL;

  count = 0;

  item = game->first_facet->next_item;

  while (item)
  {
    item->letter = aug_index++;
    /*sprintf(line, "%c) %s", item->letter, item->single_id_name);
      strcat(temp, line);*/
    list[count] = item;
    count++;
    item = item->next_item;
  }

  if (count == 0)
  {
    queue_msg("You have no facets or augmentations.");
    return;
  }

  menu_list = make_item_menu(list, count);

  browse(menu_list, count, MENU_FACETS, NULL, NULL);

  del_menu(menu_list, count);

  return;
} /* display_summary */



void inspect_facet_aug(const item_t * item)
{
  if (item == NULL)
    return;

  st_erase();

  st_move(1, 1);

  st_addstr(item->single_id_name);

  constrained_wrap(3, 41, 38, item->description);

  st_flush();

  queue_msg("Inspecting...");

  msgflush_wait();

  clear_msgbar();

  return;
} /* inspect_facet_aug */



/*
  Builds a message of form "<A creature> <message>" and adds it to the
  message queue. The proper creature article will be used.

  This is in player.c so we won't pollute message.c with
  creature-specific code.
*/
void msg_one(creature_t * creature, const char * message)
{
  char temp[40];

  if ((creature == NULL) || (message == NULL))
    return;

  sprintf(temp, "%s", creature->name_one);
  msg_glue(temp, message);

  return;
} /* msg_one */



/*
  Same as msg_one(), but uses the definitive article instead.
*/
void msg_the(creature_t * creature, const char * message)
{
  char temp[40];

  if ((creature == NULL) || (message == NULL))
    return;

  sprintf(temp, "%s", creature->name_the);
  msg_glue(temp, message);

  return;
} /* msg_the */



void throw_or_fire(creature_t * creature, const dir_t dir)
{
  unsigned int range;
  item_t * launcher;
  item_t * missile;
  
  if (creature == NULL ||
      dir == dir_none)
    return;

  launcher = get_equipped(creature, item_type_r_weapon, 0);
  missile = get_item_from_stack(get_equipped(creature, item_type_ammo, 0));
  
  if (launcher != NULL)
    range = get_weapon_range(launcher);
  else
    range = attr_current(creature, attr_throw_range);
  
  /* RFE: Is it necessary to detach_item() here? Doesn't fire_missile() do this by itself? */
  fire_missile(creature, dir, launcher, detach_item(missile), false, range);

  /*
    If we run out of ammo, cycle to the next available. If there is
    none, quiver_cycle() will print "Out of ammo!". This only applies
    if we have a launcher, if we're throwing we'll equip _any_ ammo
    and we probably don't want that.
  */
  if (launcher && get_equipped(creature, item_type_ammo, 0) == NULL)
    quiver_cycle(creature);

  /* RFE: Should this happen if it's a silent weapon? */
  if (launcher && launcher->item_number == treasure_shotgun)
    draw_attention(creature->location, creature->y, creature->x, noise_loud);

  return;
} /* throw_or_fire */



/*
  Lists any status flags CREATURE has. The result will be stored in
  BUF so you better make sure it's big enough.

  flag_mode_stats: brief, for the statistics area
  flag_mode_summary: for the character summary
  flag_mode_morgue: for morgue.txt (past tense)
  (flag_mode_t in player.h)

  Note that even if no flags are listed, BUF will still be cleared.
*/

/*
  RFE: Some of these don't make any sense. "You are stunned" and "you
  are asleep" will never be shown in the summary, for example, since
  the player cannot access the summary while in those states.
*/
void status_flags(const creature_t * creature, char * buf, const flag_mode_t flag_mode)
{
  if (creature == NULL || buf == NULL)
    return;

  strcpy(buf, "");

  if (is_burdened(creature))
  {
    switch (flag_mode)
    {
      case flag_mode_stats:
	strcat(buf, "Burdened\n");
	break;

      case flag_mode_summary:
	strcat(buf, "You are burdened.\n");
	break;

      case flag_mode_morgue:
	strcat(buf, "You were burdened.\n");
	break;
    }
  }

  if (attr_current(creature, attr_poisoned) > 0)
  {
    switch (flag_mode)
    {
      case flag_mode_stats:
	strcat(buf, "Poisoned\n");
	break;

      case flag_mode_summary:
	strcat(buf, "You are poisoned.\n");
	break;

      case flag_mode_morgue:
	strcat(buf, "You were poisoned.\n");
	break;
    }
  }

/*
  if ()
  {
    switch (flag_mode)
    {
      case flag_mode_stats:
	strcat(buf, "\n");
	break;

      case flag_mode_summary:
	strcat(buf, "You are .\n");
	break;

      case flag_mode_morgue:
	strcat(buf, "You were .\n");
	break;
    }
  }
*/

  if (attr_current(creature, attr_wounded) > 0)
  {
    switch (flag_mode)
    {
      case flag_mode_stats:
	strcat(buf, "Wounded\n");
	break;

      case flag_mode_summary:
	strcat(buf, "You are wounded.\n");
	break;

      case flag_mode_morgue:
	strcat(buf, "You were wounded.\n");
	break;
    }
  }

  if (is_blinded(creature))
  {
    switch (flag_mode)
    {
      case flag_mode_stats:
	strcat(buf, "Blinded\n");
	break;

      case flag_mode_summary:
	strcat(buf, "You are blinded.\n");
	break;

      case flag_mode_morgue:
	strcat(buf, "You were blinded.\n");
	break;
    }
  }

  if (is_floating(creature) || attr_current(creature, attr_permafloat))
  {
    switch (flag_mode)
    {
      case flag_mode_stats:
	strcat(buf, "Floating\n");
	break;

      case flag_mode_summary:
	strcat(buf, "You float.\n");
	break;

      case flag_mode_morgue:
	strcat(buf, "You were floating.\n");
	break;
    }
  }

  if (get_effect_by_id(creature, effect_hide))
  {
    switch (flag_mode)
    {
      case flag_mode_stats:
	strcat(buf, "Hiding\n");
	break;

      case flag_mode_summary:
	strcat(buf, "You are hiding.\n");
	break;

      case flag_mode_morgue:
	strcat(buf, "You were hiding.\n");
	break;
    }
  }

  if (attr_known(creature, attr_unchanging))
  {
    switch (flag_mode)
    {
      case flag_mode_stats:
	strcat(buf, "Unchanging\n");
	break;

      case flag_mode_summary:
	strcat(buf, "You are unable to change form.\n");
	break;

      default:
	break;
    }
  }
  else if (flag_mode == flag_mode_morgue &&
	   attr_current(creature, attr_unchanging))
  {
    strcat(buf, "You were unable to change form.\n");
  } 

  if (attr_known(creature, attr_p_sleep))
  {
    switch (flag_mode)
    {
      case flag_mode_summary:
	strcat(buf, "You are immune to sleep.\n");
	break;

      case flag_mode_morgue:
	strcat(buf, "You were immune to sleep.\n");
	break;

      default:
	break;
    }
  }

  if (flag_mode == flag_mode_summary &&
      attr_known(creature, attr_wound_immunity))
  {
    strcat(buf, "You are protected from wounds.\n");
  }
  else if (flag_mode == flag_mode_morgue &&
	   attr_current(creature, attr_wound_immunity))
  {
    strcat(buf, "You were protected from wounds.\n");
  }

  if (flag_mode == flag_mode_summary &&
      attr_known(creature, attr_gas_immunity))
  {
    strcat(buf, "Your respiratory system is protected.\n");
  }
  else if (flag_mode == flag_mode_morgue &&
	   attr_current(creature, attr_gas_immunity))
  {
    strcat(buf, "Your respiratory system was protected.\n");
  }

  if (flag_mode == flag_mode_summary &&
      attr_known(creature, attr_nonbreathing))
  {
    strcat(buf, "You do not breathe.\n");
  }
  else if (flag_mode == flag_mode_morgue &&
	   attr_current(creature, attr_nonbreathing))
  {
    strcat(buf, "You were not breathing.\n");
  }
  else if (flag_mode == flag_mode_summary &&
	   attr_known(creature, attr_free_swim))
  {
    strcat(buf, "You can breathe under water.\n");
  }
  else if (flag_mode == flag_mode_morgue &&
	   attr_current(creature, attr_free_swim))
  {
    strcat(buf, "You could breathe under water.\n");
  }

  if (get_effect_by_id(creature, effect_sleep))
  {
    switch (flag_mode)
    {
      case flag_mode_stats:
	strcat(buf, "Sleeping\n");
	break;

      case flag_mode_summary:
	strcat(buf, "You are asleep.\n");
	break;

      case flag_mode_morgue:
	strcat(buf, "You were asleep.\n");
	break;
    }
  }

  if (get_effect_by_id(creature, effect_haste))
  {
    switch (flag_mode)
    {
      case flag_mode_stats:
	strcat(buf, "Haste\n");
	break;

      case flag_mode_summary:
	strcat(buf, "You are hastened.\n");
	break;

      case flag_mode_morgue:
	strcat(buf, "You were hastened.\n");
	break;
    }
  }

  if (get_effect_by_id(creature, effect_healing))
  {
    switch (flag_mode)
    {
      case flag_mode_stats:
	strcat(buf, "Healing\n");
	break;

      case flag_mode_summary:
	strcat(buf, "You are healing.\n");
	break;

      case flag_mode_morgue:
	strcat(buf, "You were healing.\n");
	break;
    }
  }

  if (get_effect_by_id(creature, effect_stun))
  {
    switch (flag_mode)
    {
      case flag_mode_stats:
	strcat(buf, "Stunned\n");
	break;

      case flag_mode_summary:
	strcat(buf, "You are stunned.\n");
	break;

      case flag_mode_morgue:
	strcat(buf, "You were stunned.\n");
	break;
    }
  }

  if (get_effect_by_id(creature, effect_web))
  {
    switch (flag_mode)
    {
      case flag_mode_stats:
	strcat(buf, "Web\n");
	break;

      case flag_mode_summary:
	strcat(buf, "You are stuck in a web.\n");
	break;

      case flag_mode_morgue:
	strcat(buf, "You were stuck in a web.\n");
	break;
    }
  }

  if (get_effect_by_id(creature, effect_slow))
  {
    switch (flag_mode)
    {
      case flag_mode_stats:
	strcat(buf, "Slowed\n");
	break;

      case flag_mode_summary:
	strcat(buf, "You are slowed.\n");
	break;

      case flag_mode_morgue:
	strcat(buf, "You were slowed.\n");
	break;
    }
  }

  if (get_effect_by_id(creature, effect_light))
  {
    switch (flag_mode)
    {
      case flag_mode_stats:
	strcat(buf, "Light\n");
	break;

      case flag_mode_summary:
	strcat(buf, "You are surrounded by light.\n");
	break;

      case flag_mode_morgue:
	strcat(buf, "You were surrounded by light.\n");
	break;
    }
  }


  if (get_effect_by_id(creature, effect_illiteracy))
  {
    switch (flag_mode)
    {
      case flag_mode_stats:
	strcat(buf, "Illiterate\n");
	break;

      case flag_mode_summary:
	strcat(buf, "You are illiterate.\n");
	break;

      case flag_mode_morgue:
	strcat(buf, "You were illiterate.\n");
	break;
    }
  }

  if (flag_mode == flag_mode_summary &&
      killed(creature) == false)
  {
    strcat(buf, "You are alive.\n");
  }
  else if (flag_mode == flag_mode_morgue &&
	   killed(creature))
  {
    strcat(buf, "You are dead.\n");
  }

  return;
} /* status_flags */



/*
  Returns true if CREATURE can benefit from waiting several turns. If
  it has no status effects that will time out, there's no point in
  resting and this will return false.

  If MESSAGE is false, no messages will be printed.
*/
blean_t needs_rest(creature_t * creature, const blean_t message)
{
  if (creature == NULL)
    return false;

  /* Can't rest while poisoned, this could prove lethal. */
  if (attr_current(creature, attr_poisoned))
  {
    if (message)
      queue_msg("You can not rest now. You have been poisoned!");

    return false;
  }

  /* If we're wounded or blinded... */
  if (attr_current(creature, attr_wounded) ||
      is_blinded(creature) ||
      get_effect_by_id(creature, effect_slow) ||
      get_effect_by_id(creature, effect_stun))
    return true;

  /* Else... */
  if (message)
    queue_msg("You do not need any rest.");

  return false;
} /* needs_rest */



void break_rest(creature_t * creature)
{
  if (creature &&
      is_player(creature) &&
      creature->multiaction == MULTIACTION_REST)
  {
    creature->multiaction = MULTIACTION_BREAK_REST;
  }

  return;
}


void list_item(const item_t * item)
{
  char * name;
  char line[100];

  if (item == NULL)
    return;

  if (item->stack != NULL)
  {
    name = get_inv_item_name(item->stack);
    sprintf(line, "(%c) %s.", item->stack->letter, name);
  }
  else
  {
    name = get_inv_item_name(item);
    sprintf(line, "(%c) %s.", item->letter, name);
  }
  
  free(name);
  name = NULL;
  
  queue_msg(line);

  return;
}



/*
  Checks if CREATURE should continue resting. Returns true if rest was
  broken, false if it should continue.
 */
blean_t try_to_rest(creature_t * creature)
{
  unsigned int i;
  creature_t * temp;
  level_t * level;

  if (creature == NULL ||
      (level = creature->location) == NULL)
    return false;
  
  /*
    The player is resting. This will continue until all bad statuses
    have disappeared (that can be removed by resting) or it exceeds
    100 turns.
  */
  if (needs_rest(creature, false) == false ||
      creature->multi_param == 100)
  {
    break_rest(creature);
    return true;
  }
  else
  {
    /* A hostile creature comes within view */

    for (i = 0; i < level->creatures; i++)
    {
      temp = level->creature[i];

      if (temp != NULL &&
	  temp != creature &&
	  can_see_creature(creature, temp))
      {
	if (creature->multi_param > 1)
	  msg_one(temp, "comes into view.");

	break_rest(creature);
	return true;
      }
    }

/*	   can_see_anyone() ||*/
  }

  /* Do nothing. Meditate, perhaps. */
  creature->multi_param++;

  return false;
} /* try_to_rest */



/*
  Prints a message when EFFECT expires. Some effects (like light
  radius) will communicate its expiry to the player in other ways, and
  for some it might not be desirable to let the player know at all
  that it has expired.
*/
void msg_expire(effect_t * effect)
{
  char line[100];
  creature_t * creature;
  
  if (effect == NULL)
    return;

  creature = effect->affecting;

  switch (effect->id)
  {
    case effect_slow:
      sprintf(line, "You feel yourself speeding up.");
      break;

    case effect_haste:
      sprintf(line, "You feel yourself slowing down.");
      break;

    case effect_rage:
      sprintf(line, "You don't feel quite as angry anymore.");
      break;
      
    case effect_healing:
      sprintf(line, "You no longer feel as warm.");
      break;

    case effect_poison:
      if (is_player(creature))
	sprintf(line, "You no longer poisoned.");
      else if (can_see(game->player, creature->y, creature->x))
	sprintf(line, "%s is no longer poisoned.", creature->name_the);
      break;

    case effect_stun:
      sprintf(line, "You can move again.");
      break;

    case effect_web:
      if (is_player(creature))
	sprintf(line, "You break free of the web.");
      else if (can_see(game->player, creature->y, creature->x))
	sprintf(line, "%s breaks free of a web.", creature->name_the);
      break;

    case effect_wound:
      if (is_player(creature))
	sprintf(line, "You are no longer bleeding.");
      else if (can_see(game->player, creature->y, creature->x))
	sprintf(line, "%s is no longer bleeding.", creature->name_the);
      break;

    case effect_hide:
      sprintf(line, "You are not hiding anymore.");
      break;

    case effect_blindness:
      sprintf(line, "You can see again.");
      break;

    case effect_illiteracy:
      sprintf(line, "You are no longer illiterate.");
      break;

    case effect_temp_weapon:
      switch (effect->param[EFFECT_VWEAPON_INDEX])
      {
	case virtual_hungry_book:
	  sprintf(line, "The book lets go of your hand.");
	  break;

	case virtual_flame_hands:
	  sprintf(line, "Your hands are no longer burning.");
	  break;

	default:
	  sprintf(line, "BUG: Missing temp_weapon expire message.");
	  break;
      }
      break;
      
    case effect_sleep:
      sprintf(line, "You wake up. BUG: This message should never be displayed.");

    default:
      /* No message */
      return;
  }

  upperfirst(line);
  queue_msg(line);

  return;
} /* msg_expire */
