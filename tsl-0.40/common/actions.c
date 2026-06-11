#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>

#include "main.h"
#include "stuff.h"
#include "rules.h"
#include "altitude.h"
#include "actions.h"
#include "item.h"
#include "effect.h"
#include "ui.h"
#include "player.h"
#include "level.h"
#include "inventory.h"
#include "game.h"
#include "magic.h"
#include "monster.h"
#include "ai.h"
#include "places.h"
#include "shapeshf.h"
#include "fov.h"
#include "elements.h"
#include "traps.h"
#include "stacks.h"
#include "losegame.h"
#include "ability.h"
#include "poison.h"
#include "explode.h"
#include "potions.h"
#include "area.h"
#include "sleep.h"
#include "burdened.h"
#include "craft.h"
#include "gore.h"
#include "find.h"
#include "equip.h"
#include "missile.h"
#include "reading.h"
#include "options.h"
#include "wounds.h"
#include "doors.h"



/*
  Special events when OWNER equips certain ITEMs. OWNER must be the player.
*/
void equip_artifact(creature_t * owner, item_t * item)
{
  char * old_name;
  char * new_name;
  char line[100];
  unsigned int saved_id;
  int temp;
  effect_t * eff;

  if (owner == NULL || item == NULL)
    return;

  if (is_player(owner) == false)
    return;

  saved_id = item->id;
  old_name = get_item_name(item);
  item->id |= known_name;
  new_name = get_inv_item_name(item);
  item->id = saved_id;
  
  switch (item->item_number)
  {
    case artifact_starflame_mace:
      if ((identified(item) & known_name) == 0)
      {
	if (is_blinded(owner) == false)
	{
	  sprintf(line, "%s lights up!", old_name);
	  upperfirst(line);
	  queue_msg(line);
	}

	sprintf(line, "This must be the fabled %s!", new_name);
	queue_msg(line);

	make_item_known(item);
      }
      
      break;

    case artifact_crown_of_thorns:
      temp = wound_creature(owner, CROWN_OF_THORNS_WOUNDING);

      if (attr_current(owner, attr_m_first_aid) == 0 && temp)
      {
	sprintf(line, "%s cuts deep into your skin!", old_name);
	upperfirst(line);
	queue_msg(line);
	
	if ((identified(item) & known_name) == 0)
	{
	  sprintf(line, "This must be the %s!", new_name);
	  queue_msg(line);
	  make_item_known(item);
	}
      }
      
      break;

    case artifact_axe_trollish:
      eff = get_effect_by_id(owner, effect_illiteracy);
      
      if (attr_current(owner, attr_p_read) == 0)
      {
	if (maybe())
	  queue_msg("You feel like you grew up in a less privileged environment.");
	else
	  queue_msg("You feel really stupid now.");

	if ((identified(item) & known_name) == 0)
	{
	  sprintf(line, "This must be the %s!", new_name);
	  queue_msg(line);
	  make_item_known(item);
	}
      }
      
      effect_expire(eff);
      prolong_effect(owner, effect_illiteracy, TROLLISH_ILLITERACY);
	
      break;
      
    default:
      /* Nothing */
      break;
  }

  free(old_name);
  free(new_name);

  return;
} /* equip_artifact */



/*
  TODO: What does this DO?
 */
void instant_reveal(creature_t * creature)
{
  creature_t * other;
  level_t * level;
  unsigned int i;

  if (creature == NULL)
    return;

  level = creature->location;

  for (i = 0; i < level->creatures; i++)
  {
    other = level->creature[i];

    if ((other == NULL) || (other == creature))
      continue;

    if (can_see_creature(other, creature))
    {
      aggravate(other);
    }
  }
  
  return;
} /* instant_reveal */



/*
  Uses ITEM on CREATURE. ITEMs that are disposed after use will be
  deleted, others will remain where they are.

  Returns true if the action consumed a turn.
*/
blean_t use_item(creature_t * creature, item_t * item)
{
/*  char line[100];
    char * temp_str;*/
  use_func_t action;

  if (creature == NULL || item == NULL)
    return false;
  
  if ((action = use_func(item)) != NULL)
  {
    return action(creature, item);
  }
  else if (item->invoke_power != 0)
  {
    /* Magical devices */
    
    if (item->charges == 0)
    {
      /* There are no charges left */
      
      if (is_player(creature))
      {
	queue_msg("All charges seem to have been used up.");
	item->id |= known_charges; /* Let the player know there are 0 charges left */
      }

      /*
	TODO: This should probably return true, since this way we can
	check for free if there is one more charge by zapping and then
	cancelling.
      */
      return false;
    }
    else
    {
      /* There are charges remaining */
      unsigned int ability;
      
      ability = item->invoke_power;
      
      if ((ability >= ATTRIBUTES) ||
	  (attr_info[ability] == NULL))
      {
	queue_msg("BUG: Unknown invoke_power.");
	return false;
      }
      
      if (invoke_ability(creature, ability, item))
      {
	/* We activated the device; one charge used up */
	spend_charge(item);

	return true;
      }
      else
      {
	/* We didn't fulfill the activation */
	return false;
      }
    }
    
    return false;
  }  /* charges remaining */
  else
  {
    /* Something undefined  */
    queue_msg("You can't use that!");
    return false;
  }

  return true;
} /* use_item */



/*
  Returns a pointer to the function where ITEM is applied, or NULL if
  the item can't be applied in any way.
*/
use_func_t use_func(const item_t * item)
{
  if (item == NULL)
    return NULL;
  
  if (item->item_type == item_type_food)
    return &eat;

  if (item->item_type == item_type_potion)
    return &drink_potion;

  if (item->item_type == item_type_book)
    return &read_book;

  if (item->item_type == item_type_scroll)
    return &read_scroll;

  if (item->item_number == treasure_dataprobe)
    return &dataprobe;

  if (item->item_number == treasure_key)
    return &key_open;

  return NULL;
} /* use_func */



/*
  Recharges ITEM.
 */
void recharge(creature_t * recharger, item_t * item, item_t * source)
{
  char line[80];
  char * name;
  signed int failure_prob;

  if ((item == NULL))
    return;

  /* Whatever happens, the number of remaining charges will have changed. */
  item->id &= ~known_charges;

  name = get_item_name(item);

  failure_prob = 5 + item->charges;

  if (tslrnd() % 100 < failure_prob)
  {
    /* Recharging failed; item explodes... */
    sprintf(line, "%s explodes!", name);
    upperfirst(line);
    queue_msg(line);

    explosion(recharger->location, recharger, source,
	      recharger->y, recharger->x, expl_wand, 0, NULL, NULL);
    
    detach_item(item);
    unburdened(recharger, item->weight);
    del_item(item);
  }
  else
  {
    /* The recharging succeeded */
    sprintf(line, "%s was recharged.", name);
    upperfirst(line);
    queue_msg(line);
    item->charges += roll(1, 6);
    item->id &= ~known_charges;
  }

  free(name);
  name = NULL;

  return;
} /* recharge */



/*
  Drops ITEM on the floor. The item will be on the floor under the
  creature holding it. If the item isn't in an inventory, or the
  creature holding it doesn't have a proper location set, or it
  couldn't be attached to the level, it will be deleted.

  This function will also call floor_effect() to make "special" things
  happen depending on where the item lands (pentagrams, water, lava,
  etc).
*/
void drop_item(creature_t * owner, item_t * item, const blean_t silent)
{
  level_t * level;
  char * temp;
  char line[80];
  char name[80];
  unsigned int new_y;
  unsigned int new_x;
  unsigned int total_weight;
  
  if (item == NULL)
    return;

  if (owner == NULL)
  {
    del_item(item);
    return;
  }

  level = owner->location;

  new_y = owner->y;
  new_x = owner->x;
  
  if (NULL == level)
  {
    del_item(item);
    return;
  }
  
  total_weight = item->weight * stack_size(item);

  /* Get item name */
  if (item->stack)
    temp = get_item_name(item->stack);
  else
    temp = get_item_name(item);
  
  strcpy(name, temp);

  /* No need to worry about this anymore */
  free(temp);

  /* Place item on the ground */
  detach_item(item);
  
  if (attach_item_to_level(level, item) != NULL)
  {
    del_item(item);
    return;
  }

  place_item(item, new_y, new_x);

  /* The item is at its new location */

  /* Display what happens? */
  if (silent == false)
  {
    if (owner == game->player)
    {
      sprintf(line, "You drop %s.", name);
      queue_msg(line);
    }
    else if (can_see(game->player, new_y, new_x))
    {
      sprintf(line, "drops %s.", name);
      msg_one(owner, line);
    }
  }

  unburdened(owner, total_weight);

  /* What should happen now that the item is on the floor? */
  floor_effect(item);

  return;
} /* drop_item */



/*
  Applies "floor effects" on ITEM. ITEM must be properly attached to a
  level and positioned on a tile before this function is
  called. Depending on the floor tile, different things might happen.
*/
void floor_effect(item_t * item)
{
  level_t * level;
  unsigned int y;
  unsigned int x;
  char * name;
  char line[80];
  unsigned int items;
  unsigned int i;
  item_t ** stuff;
  creature_t * owner;

  if (item == NULL)
    return;

  level = item->location;
  y = item->y;
  x = item->x;

  if (on_map(level, y, x) == false)
    return;

  if (level->map[y][x] == tile_pentagram)
  {
  }

  if (level->map[y][x] == tile_water ||
      level->map[y][x] == tile_lava)      
  {
    /* Items will be destroyed when dropped into lava and water. */

    /* TODO: Floating items. maybe? */

    if (item->indestructible)
    {
      return;
    }
    
    if (can_see(game->player, y, x) == true)
    {
      /* If we see it disappear, print a message.*/

      name = get_item_name(item);
      
      switch (level->map[y][x])
      {
	case tile_water:
	  sprintf(line, "%s disappears into the depths!", name);
	  upperfirst(line);
	  break;
	  
	case tile_lava:
	  sprintf(line, "%s is consumed by the lava!", name);
	  upperfirst(line);
	  break;
	  
	default: break;
      }

      free(name);
      name = NULL;

      queue_msg(line);
    }

    owner = item->inventory;
    detach_item(item);
    unburdened(owner, item->weight);
    del_item(item);

    /* The item is gone; we'd better quit */
    return;
  }
  
  /* If there are more stackable items lying here, merge them into one pile */
  items = find_items(level, y, x, &stuff);
  
  for (i = 0; i < items; i++)
  {
    if (can_stack(item, stuff[i]))
    {
      attach_item_to_stack(stuff[i], item);
      break;
    }
  }

  free(stuff);
  
  return;
} /* floor_effect */



/*
  Clones CREATURE and places the clone on an adjacent square. If there
  is no room for a clone, none is created.
  See also: clone_creature()
*/
void split_creature(creature_t * creature)
{
  creature_t * clone;

  signed int dirs[4][2];
  unsigned int count;

  if (creature == NULL)
    return;

  /* Find an adjacent square where the clone can appear */
  count = 0;

  /* RFE: This can be done in a better way, I think. */
  if (is_walkable(creature->location, false, creature->y - 1, creature->x))
  {
    dirs[count][0] = creature->y - 1;
    dirs[count][1] = creature->x;
    count++;
  }
  
  if (is_walkable(creature->location, false, creature->y + 1, creature->x))
  {
    dirs[count][0] = creature->y + 1;
    dirs[count][1] = creature->x;
    count++;
  }
  
  if (is_walkable(creature->location, false, creature->y, creature->x - 1))
  {
    dirs[count][0] = creature->y;
    dirs[count][1] = creature->x - 1;
    count++;
  }
  
  if (is_walkable(creature->location, false, creature->y, creature->x + 1))
  {
    dirs[count][0] = creature->y;
    dirs[count][1] = creature->x + 1;
    count++;
  }
  
  if (count == 0)
    return; /* No space for a clone; abort */

  count = tslrnd() % count;

  clone = clone_creature(creature);

  del_creature(attach_creature(creature->location, clone));
  set_creature_coordinates(clone, dirs[count][0], dirs[count][1]);

  return;
} /* split_creature */



blean_t yell(const creature_t * creature)
{
  blean_t seen;
  char line[80];
  char name[40];
  
  if (creature == NULL ||
      creature->location == NULL)
    return false;

  if (can_see(game->player, creature->y, creature->x))
    seen = true;
  else
    seen = false;

  if (is_player(creature))
  {
    switch (creature->id)
    {
      case monster_silver_wolf:
	queue_msg("You howl!");
	break;
	  
      case monster_ghoul:
      case monster_drowned_one:
	queue_msg("You moan!");
	break;

      case monster_imp:
	queue_msg("You shriek!");
	break;

      case monster_graveling:
	queue_msg("You grunt!");
	break;

      case monster_slime:
      case monster_severed_hand:
      case monster_crypt_vermin:
      case monster_floating_brain:
	queue_msg("You cannot make any sound!");
	break;

      default:
      case monster_gnoblin:
	queue_msg("You let out a loud war cry!");
	break;
    }
  }
  else
  {
    if (attr_current(game->player, attr_perception) > 3)
      sprintf(name, "%s", creature->name_one);
    else
      strcpy(name, "something");
    
    switch (creature->id)
    {
      case monster_gnoblin:
	switch (roll(1, 4))
	{
	  case 1:
	    if (seen)
	      sprintf(line, "%s curses loudly!", name);
	    else
	      sprintf(line, "You hear %s cursing loudly!", name);
	    break;
	    
	  default:
	    if (seen)
	      sprintf(line, "%s yells!", name);
	    else
	      sprintf(line, "You hear %s yelling!", name);
	    break;
	}
	break;
	
      case monster_graveling:
	switch (roll(1, 4))
	{
	  case 1:
	    if (seen)
	      sprintf(line, "%s grunts.", name);
	    else
	      sprintf(line, "You hear %s grunting.", name);
	    break;

	  default:
	    if (seen)
	      sprintf(line, "%s snarls.", name);
	    else
	      sprintf(line, "You hear %s snarling.", name);
	    break;
	}
	break;
	    
      case monster_ghoul:
	if (seen)
	  sprintf(line, "%s moans!", name);
	else
	  sprintf(line, "You hear a moan!");
	break;
	
      case monster_imp:
	switch (roll(1, 4))
	{
	  default:
	    if (seen)
	      sprintf(line, "%s shrieks!", name);
	    else
	      sprintf(line, "You hear %s shrieking!", name);
	    break;
	}
	break;
	
      default:
	return false;
    }
    
    upperfirst(line);
    queue_msg(line);
  }
  
  draw_attention(creature->location, creature->y, creature->x, noise_loud);

  return true;
} /* yell */



void draw_attention(level_t * level,
		    const unsigned int source_y,
		    const unsigned int source_x,
		    const noise_level_t noise)
{
  unsigned int amount;
  unsigned int y;
  unsigned int x;
  area_t * area;
  area_t * temp_area;
  creature_t * creature;

  if (level == NULL)
    return;
  
  switch (noise)
  {
    case noise_faint:
      amount = 11;
      break;

    case noise_loud:
      amount = 60;
      break;

    case noise_terrible:
      amount = 150;
      break;

    default:
      amount = 0;
      break;
  }

  area = new_area(source_y, source_x);
  area_cloud(area, level, amount);

  for (temp_area = area; temp_area != NULL; temp_area = temp_area->next)
  {
    y = temp_area->y;
    x = temp_area->x;

    creature = find_creature(level, y, x);

    if (creature != NULL &&
	is_player(creature) == false &&
	creature->ai_state != ai_mimic)
    {
      if (is_awake(creature) == false)
      {
	/* Wake them up */
	creature_sleep(creature, false);
      }
      
      set_attr(creature, attr_perception, attr_base(creature, attr_perception) + 2);
/*      set_attr(creature, attr_vision, 5); */
      aggravate(creature);
      set_enemy_target(creature, source_y, source_x);
    }
  }

  return;
} /* draw_attention */



blean_t reveal_mimic(creature_t * mimic)
{
  if (mimic == NULL)
    return false;

  if (mimic->ai_state == ai_mimic)
  {
    if (can_see_creature(game->player, mimic))
    {
      queue_msg("Wait! That is a small mimic!");
    }

    add_anim(anim_type_attention, mimic->uid, 0, mimic->y, mimic->x);

    set_creature_name(mimic, "a mimic", "the mimic", "mimic");
    mimic->gent = gent_mimic;
    mimic->ai_state = ai_offensive;
/*    mimic->aggravated = true;*/

    return true;
  }

  return false;
} /* reveal_mimic */



void aggravate(creature_t * creature)
{
  if (creature == NULL ||
      is_player(creature) ||
      creature->ai_state == ai_retreat ||
      creature->ai_state == ai_mimic)
  {
    return;
  }

  creature->ai_state = ai_offensive;
  find_target(creature);

  return;
} /* aggravate */



/*
  Sets a coordinate pair {Y, X} that CREATURE will try to move to.
*/
void set_enemy_target(creature_t * creature,
		      const unsigned int y, const unsigned int x)
{
  if (creature == NULL ||
      on_map(creature->location, y, x) == false ||
      creature->ai_state == ai_mimic)
  {
    return;
  }

  creature->ai_state = ai_target;

  creature->target_y = y;
  creature->target_x = x;

  return;
} /* set_enemy_target */



/* Makes player interact with CREATURE. */
blean_t interact(creature_t * creature)
{
/*  char line[80];*/
/*  item_t * item;*/

  if (creature == NULL)
    return false;

  if (get_effect_by_id(creature, effect_sleep))
  {
    msg_one(creature, "appears to be asleep.");
    return false;
  }
  
  /* Creature is generic */
  switch (creature->id)
  {
    case unique_king_of_worms:
      queue_msg("He keeps the wisdom that you need, the password that you want.");
      break;
      
    case monster_gnoblin:
      msg_one(creature, "snarls at you!");
      break;

    case monster_ghoul:
      msg_one(creature, "appears to desire your brain.");
      break;
      
    case monster_imp:
      msg_one(creature, "giggles impishly!");
      break;
      
    case monster_slime:
      msg_one(creature, "oozes around, indifferent to your inquiries.");
      break;

    default:
      queue_msg("BUG: Undefined generic interaction!");
      return false;
  }
  
  return true;
} /* interact */



void traverse_branch(const unsigned int type)
{
  level_t * level;
  unsigned int y;
  unsigned int x;
  unsigned int i;
  tile_t wanted_tile;
  unsigned int new_level_index;
    
  if (game->player == NULL)
    return;

  level = get_current_level();

  if (level == NULL)
    return;
  
  new_level_index = level->link[type - tile_stair0];

  for (i = 0; i < STAIRS; i++)
  {
    if (game->level_list[new_level_index]->link[i] == level->level_index)
    {
      wanted_tile = tile_stair0 + i;
      goto traverse;
    }
  }

  queue_msg("BUG: bad level link!");
  return;

traverse:
  change_level(new_level_index);
  
  /* RFE: find_spot()! */
  for (y = 0; y < get_current_level()->size_y; y++)
  {
    for (x = 0; x < get_current_level()->size_x; x++)
    {
      if (get_current_level()->map[y][x] == wanted_tile)
      {
	set_creature_coordinates(game->player, y, x);
      }
    }
  } 
  
  return;
} /* traverse_branch */



blean_t reveal_creature(creature_t * creature)
{
  if (creature == NULL ||
      creature->detected)
    return false;
  
  if (on_map(creature->location, creature->y, creature->x) &&
      can_see(game->player, creature->y, creature->x) &&
      creature->detected == false)
  {
    creature->detected = true;
    draw_level();
    msg_one(creature, "is revealed!");
    return true;
  }

  return false;
} /* reveal_creature */



/*
  Tries to make CREATURE pick up ITEM. If ITEM is null, prompt the
  player for an item at the creatures feet to pick up (possibly
  none). Returns TRUE if the action was completed, FALSE if it was
  cancelled or invalid.
*/
blean_t pickup(creature_t * creature, item_t * item, const blean_t first_item)
{
  char line[80];
  unsigned int items;
  item_t ** item_list;
  char * name;
  item_t * temp_item;

  if (creature == NULL)
    return 0;

  if (is_player(creature) == false)
  {
    queue_msg("BUG: pickup() called for someone else than the player");
    return false;
  }

  /* Do we already know what item we want to pick up? */

  if (item == NULL)
  {
    /* Give the player a choice */

    items = find_items(creature->location,
				  creature->y, creature->x, &item_list);
    
    if (items > 0 && is_floating(creature))
    {
      queue_msg("You cannot reach it!");
      return false;
    }
    
    if (items == 0)
    {
      /* Suppress this message if we were going to pick up the first item. */
      if (first_item == false)
	queue_msg("Nothing here.");
      
      /* item_list is NULL; no need to free it. */
      return false;
    }
    else if (items == 1 || first_item)
    {
      /*
	If there's only one item or we're supposed to pick up the
        first item, we don't need to give the player a choice.
      */
      item = item_list[0];
    }
    else
    {
      /* There are more than one item; let the player select one. */
/*      queue_msg("Pick up which item?");
      msgflush_nowait();
      
      item = select_item(item_list, items, NULL, false, NULL, true);*/
    }

    /* We no longer need the list. */
    free(item_list);

    if (item == NULL)
    {
      /* User cancelled. */
      clear_msgbar();
      return false;
    }
  }
  
  /* item should now point to the item we want to pick up. */

  if (get_carried_weight(creature)
      + stack_size(item) * get_weight(item)
      > get_weight_allowance(creature) * 2)
  {
    queue_msg("Too heavy!");
    return false;
  }

  if (get_carried_weight(creature)
      + stack_size(item) * get_weight(item)
      > get_weight_allowance(creature))
  {
    if (item->stack != NULL)
      name = get_item_name(item->stack);
    else
      name = get_item_name(item);
    
    sprintf(line, "You have trouble lifting %s.", name);
    free(name);
    name = NULL;
    
    queue_msg(line);
  }

  /*
    This is done before we attach it to the creatures inventory since
    the stack size could change and mess up the grammar.
  */
  auto_id(creature, item);
  
  if (attach_item_to_creature(creature, item) != NULL)
  {
    /*
      We couldn't carry it - try to reattach it to the level, and
      delete the item if it couldn't be reattached - this will seem
      VERY strange to the player if it actually happens...
    */

    del_item(attach_item_to_level(creature->location, item));
    queue_msg("BUG: Couldn't carry, couldn't reattach.");
  }
  
  if (is_player(creature))
  {
    /*
      If we stacked the new item(s), display the total number of items
      now in the new stack.
    */
    
    /* RFE: Should this be under an else { } from the code above? */

    list_item(item);
  }

  if (options.autoequip == false)
    return true;

  if (item->item_type == item_type_ammo)
  {
    /*
      New item is ammo. If we have no ranged attack at all, ready the
      ammo.  If we have a ranged weapon equipped but no ammo for it
      reload with the new ammo.
    */

    temp_item = get_equipped(creature, item_type_r_weapon, 0);

    if (equipped_ranged(creature) == NULL)
    {
      quiver_cycle(creature);
    }
    else if (temp_item &&
	     get_equipped(creature, item_type_ammo, 0) == NULL &&
	     temp_item->custom[WEAPON_AMMO_TYPE] == item->custom[CUSTOM_AMMO_ATYPE])
    {
      quiver_cycle(creature);
    }
  }
  else if (get_equipped(creature, item->item_type, 0) == NULL)
  {
    if (item->item_type == item_type_r_weapon)
    {
      /*
	New item is a ranged weapon. If we have no ammo, equip the
	weapon. If we have matching ammo equipped, equip the weapon as
	well. If we have some other kind of ammo, do nothing.
      */
      temp_item = get_equipped(creature, item_type_ammo, 0);
      
      if (temp_item)
      {
	if (temp_item->custom[CUSTOM_AMMO_ATYPE] == item->custom[WEAPON_AMMO_TYPE])
	  try_to_equip(item);
      }
      else if (equipped_ranged(creature) == NULL)
      {
	try_to_equip(item);
      }
    }
    else if (almost_wand(item))
    {
      if (equipped_ranged(creature) == NULL)
	try_to_equip(item);
    }
    else
    {
      try_to_equip(item);

      if (item->item_type == item_type_m_weapon && item->equipped)
      {
	creature->attack_pos = -1;
	creature->next_attack = 0;
      }
    }
  }

  return true;
} /* pickup */



/*
  Makes CREATURE traverse the stairs it is standing on. Returns TRUE
  if the action was completed, FALSE if it was invalid.
*/
blean_t stairs(creature_t * creature, const blean_t message)
{
  unsigned int y;
  unsigned int x;
  level_t * old_level;

  if (creature == NULL)
    return false;

  /* TODO: NPCs should be allowed to do this as well. */
  if (is_player(creature) == false)
  {
    queue_msg("Only the player can do that!");
    return false;
  }
  
  y = creature->y;
  x = creature->x;
  
  old_level = creature->location;

  if (old_level->map[y][x] != tile_stair0 &&
      old_level->map[y][x] != tile_stair1 &&
      old_level->map[y][x] != tile_stair2 &&
      old_level->map[y][x] != tile_stair3)
  {
    if (message)
      queue_msg("There are no stairs here.");

    return false;
  }

  queue_msg("You climb...");
  
  map_erase();
  map_flush();
  msgflush_wait();
  clear_msgbar();
  
  traverse_branch(old_level->map[y][x]);
  
  move_everyone(old_level, y, x,
		creature->location, creature->y, creature->x);

  /* Hopefully, we've now arrived on the new level... */

  center_view_y(get_current_level(), game->player->y);
  center_view_x(get_current_level(), game->player->x);

  look_at_location(game->player, get_current_level(), game->player->y, game->player->x, false);

  draw_level();
  
  return true;
} /* stairs */



/*
  Makes CREATURE use whatever is assigned to its shift slot, in
  direction DIR. Return true if the action succeeded (and cost a
  turn), false if it was aborted for some reason.
*/
blean_t shift_action(creature_t * creature, const dir_t dir)
{
  item_t * item;

  if (creature == NULL)
    return false;

  item = get_equipped(creature, item_type_wand, 0);

  if (item == NULL)
    item = get_equipped(creature, item_type_tool, 0);
  
  if (item)
  {
    /* Creature has a wand (or tool); activate it.. */
    
    if (item->charges == 0)
    {
      queue_msg("All charges seem to have been used up.");

      /*
	If we didn't know we had run out of charges, revealing this
        information will cost a turn.
      */
      if ((item->id & known_charges) == 0)
      {
	item->id |= known_charges; /* Let the player know there are 0 charges left */
	return true;
      }

      /* If we already knew there were no charges left no turn is consumed. */
      return false;
    }

    /*
      Pass the selected direction as magic param, targeted spells (all
      wands) will use this instead of prompting for a direction.
    */

    if (attr_info[item->invoke_power]->invoke(creature, item, dir))
    {
      spend_charge(item);
      return true;
    }
    
    return false;
  }

  if (get_equipped(creature, item_type_r_weapon, 0) == NULL &&
      get_equipped(creature, item_type_ammo, 0) == NULL)
  {
    queue_msg("No ranged attack selected!");
    return false;
  }
  
  /* Fire the equipped ranged weapon or throw a missile. */
  if (can_fire_weapon(creature, false))
  {
    throw_or_fire(creature, dir);
    return true;
  }

  return false;
} /* shift_action */



/*
  Returns true if CREATURE has a weapon with ammo and is capable of
  firing it. If not, pushes an error message into the message queue
  and returns false.
*/
blean_t can_fire_weapon(creature_t * creature, const blean_t prohibit_throw)
{
  if (creature == NULL)
  {
    queue_msg("BUG: can_fire_weapon()");
    return false;
  }
  
  if (is_swimming(creature) == true)
  {
    queue_msg("You cannot fight under water!");
    return false;
  }

  /*
    We can't use the explicit fire command if we don't have any
    ranged weapon - use throw instead.
  */
  if (prohibit_throw && get_equipped(creature, item_type_r_weapon, 0) == NULL)
  {
    queue_msg("You don't have any ranged weapon equipped.");
    return false;
  }
  
  if (get_equipped(creature, item_type_ammo, 0) == NULL)
  {
    queue_msg("Out of ammo!");
    return false;
  }

  return true;
} /* can_fire_weapon */



/*
  Takes a charge off ITEM. If item runs out of charges and is a
  non-rechargeable type it will be destroyed. If this happens, returns
  true. Usually returns false.
 */
blean_t spend_charge(item_t * item)
{
/*  char line[100];*/

  if (item == NULL)
    return false;

  if (item->charges > 0)
  {
    item->charges--;
    
    if (item->charges == 0 &&
	item->rechargeable == false)
    {
/*      char * name;
	item_t * temp;*/
      
      /* Only destroy one item, in case there are many. */
/*      temp = get_item_from_stack(item);*/
      
      /*
	Remove the [0] from the name for the "crumbles" message
	- we already know it is spent.
      */
/*      temp->id &= ~known_charges;
      
      name = get_item_name(temp);
      sprintf(line, "%s crumbles to dust.", name);
      free(name);
      upperfirst(line);
      queue_msg(line);

      del_item(temp);

      return true;*/
    }
  }

  return false;
}



blean_t throw_prompt(creature_t * creature, item_t * item)
{
  item_t * missile;
  char * item_name;
  char line[100];
  dir_t dir;
  
  /* Try to throw the item */
  
  /* Items currently in use can't be thrown; they have to be unequipped first */
/*  if (item->equipped &&
      item->item_type != item_type_ammo &&
      item->item_type != item_type_weapon)
  {
    queue_msg("You cannot throw something you're using.");
    break;
    }*/

  missile = get_item_from_stack(item);

/*  unequip_item(missile, false);*/
  
  item_name = get_item_name(missile);
  sprintf(line, "Throw %s in which direction?", item_name);
  free(item_name);
  
  queue_msg(line);
  msgflush_nowait();
  
  dir = get_direction();
  
  if (dir == dir_none)
  {
    clear_msgbar();
    return false;
  }

  /* */
  try_to_unequip(missile, false);
  
  fire_missile(creature, dir,
	       NULL, detach_item(missile),
	       false, attr_current(creature, attr_throw_range));

  return true;
} /* throw_prompt */



/*
  Lets the player enter a label for ITEM.
*/
blean_t label_prompt(item_t * item)
{
  char new_label[20];
  char line[100];
  char * name;
  item_t * temp_item;

  if (item == NULL)
    return false;

  if (attr_current(game->player, attr_p_read))
  {
    queue_msg("Unfortunately, you can not read (or write).");
    return false;
  }

  name = get_item_name(item);
  sprintf(line, "Label %s what?", name);
  free(name);

  read_string(line, new_label, 19);

  if (strlen(new_label) == 0)
    label_item(item, NULL); /* The empty string removes the label*/
  else
    label_item(item, new_label);
  
  /*
    Try to restack this item, if it can now be merged with
    another stack.
  */
  /* TODO: This should become restack() or something. */
  if (item->inventory)
  {
    temp_item = item->inventory->first_item;
    
    while (temp_item != NULL)
    {
      if (temp_item != item && can_stack(item, temp_item))
      {
	while (item->child)
	  attach_item_to_stack(temp_item, item->child);
	
	attach_item_to_stack(temp_item, item);
	break;
      }
      
      temp_item = temp_item->next_item;
    }
  }
  
  clear_msgbar();

  return false;
} /* label_prompt */
