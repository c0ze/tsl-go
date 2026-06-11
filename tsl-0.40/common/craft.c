#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "main.h"
#include "stuff.h"
#include "rules.h"
#include "craft.h"
#include "item.h"
#include "elements.h"
#include "stacks.h"
#include "ui.h"
#include "player.h"
#include "inventory.h"
#include "burdened.h"
#include "traps.h"
#include "find.h"
#include "dwiminv.h"



/*
  Makes CREATURE apply ITEM.
*/
blean_t apply(creature_t * creature, item_t * item)
{
  use_func_t f;

  if (creature == NULL || item == NULL)
    return false;

  f = apply_func(item);

  return f(creature, item);
} /* apply */



/* */
use_func_t apply_func(const item_t * item)
{
  if (item == NULL)
    return NULL;

  /* Torches */
  if (item->item_number == treasure_torch)
      /*item->equipped*/
  {
    return &torch;
  }
  
  /* Potions of poison, but only if we already know what they are. */
/*  if (item->item_number == treasure_p_poison &&
      identified(item) & known_name)
      return &poison_item;*/

  /* Set traps */
  if (item->item_number == treasure_d_small ||
      item->item_number == treasure_grenade)
    return &set_trap;

  /* Butchering */
  if (item->item_number == treasure_cleaver ||
      item->item_number == treasure_big_ugly_knife)
    return &butcher;

  return NULL;
} /* apply_func */



/*
  Prompts CREATURE to burn something with ITEM (should be a torch).

  Returns true if the action consumed a turn, false if it was
  cancelled for any reason.
*/
blean_t torch(creature_t * creature, item_t * item)
{
  item_t * item_to_burn;
  /*char line[80];*/

/*  treasure_t new_item_id;
  item_t * new_item;
  item_t * new_stack;
  char * item_name_old;
  char * item_name_new;*/

  if (creature == NULL)
    return false;

  // TODO: Could use a criteria
  if (count_inventory(creature, NULL) == 0)
  {
    queue_msg("Unfortunately, you don't have anything to burn.");
    return false;
  }

  queue_msg("What would you like to burn?");
  
  msgflush_nowait();
  
  /* Let the user select what item to grind */
  item_to_burn = dwim_select(creature, NULL, NULL);
  
  if (item_to_burn == NULL)
  {
    clear_msgbar();
    return false;
  }
  
  if (item_to_burn->item_number == treasure_filthy_rags)
  {
    queue_msg("You wrap the rags around your torch, prolonging its life.");
    item->custom[CUSTOM_LIGHT_MAX_TICKS] += 100;
    item->custom[CUSTOM_LIGHT_TICKS] = item->custom[CUSTOM_LIGHT_MAX_TICKS] - 1;
    detach_item(item_to_burn);
    unburdened(creature, item_to_burn->weight);
    del_item(item_to_burn);
    return true;
  }
  else
  {
    if (expose_item_to_fire(item_to_burn, true) == false)
      queue_msg("Nothing happens.");
    
    /* Action performed */
    return true;
  }

  return true;
} /* torch */



/*
*/
blean_t dataprobe(creature_t * creature, item_t * item)
{
  if (creature == NULL)
    return false;

  return datajack(creature, item, 0);
} /* dataprobe */



/*
  Returns true if ITEM can be poisoned (with a potion of poison, etc).
*/
blean_t can_be_poisoned(const item_t * item)
{
  if (item == NULL)
    return false;

  if (item->item_number == treasure_d_small)
    return true;

  return false;
} /* can_be_poisoned */



/*
  Prompts CREATURE for an item to poison with ITEM (should be a potion
  of poison). Returns true if the action consumed a turn, false if it
  was cancelled for any reason.
*/
blean_t poison_item(creature_t * creature, item_t * item)
{
  item_t * item_to_poison;

  if (creature == NULL)
    return false;

  if (count_inventory(creature, &can_be_poisoned) == 0)
  {
    queue_msg("Unfortunately, you don't have anything to poison.");
    return false;
  }
  
  queue_msg("Poison what?");
  
  msgflush_nowait();
  
  /* Let the user select what to poison. */
  item_to_poison = dwim_select(creature, &can_be_poisoned, NULL);
  
  if (item_to_poison == NULL)
    return false;
  
  switch (item_to_poison->item_number)
  {
    case treasure_d_small:
      upgrade_ammo(item_to_poison);
      break;

    default:
      queue_msg("BUG: poison_item(), we shouldn't get here.");
      return false;
  }

  /* Remove the potion. */
  unburdened(creature, item->weight);
  del_item(item);
 
  return true;
} /* poison_item */



/*
  Prompts CREATURE to set a trap from ITEM.
*/
blean_t set_trap(creature_t * creature, item_t * item)
{
  signed int how_many;
  unsigned int total;
  trap_t * trap;

  if (creature == NULL || item == NULL)
    return false;

  if (find_trap(creature->location, creature->y, creature->x))
  {
    queue_msg("There is already a trap here.");
    return false;
  }
  
  total = stack_size(item);

  how_many = 1;

  if (item->item_number == treasure_d_small && total > 1)
  {
    how_many = read_number("How many?");
    
    if (how_many == -1)
    {
      queue_msg("!?");
      return false;
    }
    else if (how_many == 0)
    {
      queue_msg("Very well, then.");
      return false;
    }
    else if (how_many > total)
    {
      queue_msg("You don't have that many!");
      return false;
    }
  }

  /*
    Withdraw from the trap item stack as many as we wish to use for
    the trap.
  */
  if (how_many < total)
    item = split_stack(item, how_many);
  
  if (item->item_number == treasure_grenade)
    trap = alloc_trap(trap_booby);
  else if (item->item_number == treasure_d_small)
    trap = alloc_trap(trap_dart);
  else
  {
    queue_msg("BUG: unknown item for trap, sorry.");
    return false;
  }

  /* Delete the trap item(s). */
  detach_item(item);
  unburdened(creature, item->weight);
  del_item(item);

  /*
    Set up the new trap the way we want it. It's revealed since we set
    it ourselves.
  */
  trap->revealed = true;
  trap->activations_remaining = how_many;

  if (attach_trap(creature->location, trap))
  {
    del_trap(trap);
    queue_msg("BUG: couldn't attach trap, sorry.");
  }
  else
  {
    place_trap(trap, creature->y, creature->x);
  }

  queue_msg("You set a trap.");
 
  return true;
} /* set_trap */



/*
  Converts a type of ammo to a better type, if one is available. ITEM
  must be a stack of one or more missiles and be in someones inventory
  for this to work.
*/
void upgrade_ammo(item_t * item)
{
  char line[100];
  char * item_name_new;
  creature_t * owner;
  item_t * new_stack;
  unsigned int how_many;
  treasure_t new_item_id;

  if (item == NULL)
    return;
  
  owner = item->inventory;

  if (owner == NULL)
    return;

  /* Figure out what we want to build. */
  switch (item->item_number)
  {
    case treasure_b_bullet:
      new_item_id = treasure_b_hollow;
      break;

    case treasure_d_small:
      new_item_id = treasure_d_poison;
      break;

    default:
      queue_msg("BUG: upgrade_ammo() doesn't know what to do, sorry.");
      return;
  }

  /*
    Remove old item(s) so they won't take up space in inventory. We
    won't delete them just yet.
  */
  detach_item(item);

  /* Build the new stack of better ammo. */
  how_many = stack_size(item);

  new_stack = build_item(new_item_id);

  while (--how_many)
    attach_item_to_stack(new_stack, build_item(new_item_id));

  /* We know right away what the new items are. */
  identify(new_stack);

  /* Put the new stack in inventory */
  attach_item_to_creature(owner, new_stack);

  /*
    We won't call unburdened here. We assume all missiles are of the
    same weight.
  */

  /* New delete the old items! */
  del_item(item);

  /* Print a message*/
  item_name_new = get_item_name(new_stack);
  sprintf(line, "You produce %s.", item_name_new);
  queue_msg(line);
  free(item_name_new);
  item_name_new = NULL;

  /* Where did they end up? */
  list_item(new_stack);
  
  return;
}



/*
*/
blean_t butcher(creature_t * creature, item_t * item)
{
  char line[100];
  char * old_name;
  char * new_name;
  item_t * temp_item;
  treasure_t new_item;

  if (creature == NULL || creature->location == NULL)
    return false;

  /*
    Loop through every item on the level, look for things we're
    standing on that can be butchered. When we find one, we will
    determine what to make of it (in new_item) and jump straight to
    the butchering process at do_it.
  */
  for (temp_item = creature->location->first_item;
       temp_item != NULL;
       temp_item = temp_item->next_item)
  {
    if (temp_item->y == creature->y &&
	temp_item->x == creature->x)
    {
      switch (temp_item->item_number)
      {
	case treasure_corpse:
	  new_item = treasure_decapitated_head;
	  goto do_it;

	case treasure_carcass:
	  new_item = treasure_meat;
	  goto do_it;

	case treasure_ogre_corpse:
	  new_item = treasure_decapitated_head;
	  goto do_it;
      }
    }
  }

  queue_msg("There is nothing here to butcher.");
  return false;

do_it:
  /* This only gets run if we have found an item to butcher. */

  /* Make sure we only affect one item, if it happens to be a stack. */
  temp_item = get_item_from_stack(temp_item);

  /* What is it we're butchering? */
  old_name = get_item_name(temp_item);

  /* Delete _one_ item from the stack (possibly all). */
  del_item(temp_item);

  /* We put the new item in temp_item, since it's now unoccupied. */
  temp_item = build_item(new_item);

  /* What did it become? */
  new_name = get_item_name(temp_item);

  sprintf(line, "You butcher %s into %s.", old_name, new_name);
  queue_msg(line);
  
  /* */
  attach_item_to_level(creature->location, temp_item);
  place_item(temp_item, creature->y, creature->x);
  pickup(creature, temp_item, false);

  /* Remember to clean up. */
  free(new_name);
  free(old_name);
  
  return false;
} /* butcher */
