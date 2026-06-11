#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "main.h"
#include "stuff.h"
#include "item.h"
#include "equip.h"
#include "message.h"
#include "fov.h"
#include "ui.h"
#include "game.h"
#include "actions.h"
#include "player.h"
#include "burdened.h"
#include "memory.h"
#include "altitude.h"
#include "ability.h"



/*
  Makes the creature carrying ITEM equip it.

  Returns: true if the item was equipped, false if it failed (but it shouldn't).
*/
blean_t equip_item(item_t * item)
{
  creature_t * owner;
  blean_t player;
  item_t * temp;
  
  if (item == NULL)
    return false;

  owner = item->inventory;

  if (owner == NULL)
    return false;

  player = is_player(owner);

  /* Make sure it is equippable. */
  if (is_equipable(item) == 0)
    return false; /* It isn't. */
  
  /* Is it already equipped? */
  if (item->equipped)
    return false; /* It is... */
  
  /*
    If we're equipping ammo, remove any launcher present if it isn't
    compatible. Ammo takes precedence over weapon, for once. We do
    this before checking for our own type to get a better message
    order.
  */
  if (item->item_type == item_type_ammo)
  {
    temp = get_equipped(owner, item_type_r_weapon, 0);
    
    if (temp && temp->custom[WEAPON_AMMO_TYPE] != item->custom[CUSTOM_AMMO_ATYPE])
    {
      unequip_item(temp, true);
    }

    /* Equipping ammo is usually silent. */
    unequip_item(get_equipped(owner, item_type_ammo, 0), false);
  }

  if (item->item_type == item_type_m_weapon)
  {
    temp = get_equipped(owner, item->item_type, 1);

    if (temp)
    {
      if (player)
	queue_msg("You are already using two weapons.");

      return false;
    }

    temp = get_equipped(owner, item->item_type, 0);

    if (temp)
    {
      if (attr_current(owner, attr_dualwield) == 0)
      {
	if (player)
	  queue_msg("You can only use one weapon.");

	return false;
      }
      else
      {
	if (temp && temp->custom[WEAPON_TWOHANDED])
	{
	  if (player)
	    queue_msg("You are already using a two-handed weapon.");

	  return false;
	}
	else if (temp && item->custom[WEAPON_TWOHANDED])
	{
	  if (player)
	    queue_msg("That requires both hands.");

	  return false;
	}
      }
    }
  }
  else
  {
    /* Make sure no item of the same type is equipped already */
    temp = get_equipped(owner, item->item_type, 0);
    unequip_item(temp, true);
  }

  /* If we're equipping a ranged weapon, remove any ammo present unless it's compatible. */
  if (item->item_type == item_type_r_weapon)
  {
    temp = get_equipped(owner, item_type_ammo, 0);
    
    if (temp && temp->custom[CUSTOM_AMMO_ATYPE] != item->custom[WEAPON_AMMO_TYPE])
    {
      unequip_item(temp, true);
    }
  }
  
  /* We can't equip any ranged weapon, missiles or tools while holding a wand. */
  if (item->item_type == item_type_r_weapon ||
      item->item_type == item_type_ammo)
  {
    unequip_item(get_equipped(owner, item_type_wand, 0), true);
    unequip_item(get_equipped(owner, item_type_tool, 0), true);
  }

  /* If we're equipping a wand, remove ranged weapons, ammo and tools. */
  if (item->item_type == item_type_wand || item->item_type == item_type_tool)
  {
    unequip_item(get_equipped(owner, item_type_r_weapon, 0), true);
    unequip_item(get_equipped(owner, item_type_ammo, 0), true);
    unequip_item(get_equipped(owner, item_type_tool, 0), true);
    unequip_item(get_equipped(owner, item_type_wand, 0), true);
  }

  /* Equip the item */
  item->equipped = true;

  if (item->item_type == item_type_m_weapon)
  {
    reset_seq(owner);
  }

  equip_msg(owner, item);

  if (player)
    equip_artifact(owner, item);

  return true;
} /* equip_item */



/*
  Displays a messages for CREATURE equipping ITEM.
*/
void equip_msg(creature_t * creature, item_t * item)
{
  char line[100];
  char * item_name;

  if (creature == NULL || item == NULL)
    return;

  /* This needs to be overwritten at some point */
  strcpy(line, "There is a bug in after_equip()!");

  /*
    What item are we dealing with? We temporarily switch the item to
    not equipped, to avoid messages like "you light a lit torch".
  */
  item->equipped = false;
  item_name = get_item_name(item);
  item->equipped = true;

  /*
    Report that the creature has equipped an item. Different messages
    for player and monsters.
  */
  if (is_player(creature))
  {
    switch (item->item_type)
    {
      case item_type_m_weapon:
	sprintf(line, "You wield (%c) %s.", item->letter, item_name);
	queue_msg(line);
	break;

      case item_type_r_weapon:
      case item_type_wand:
      case item_type_tool:
	sprintf(line, "You ready (%c) %s.", item->letter, item_name);
	queue_msg(line);
	break;

      case item_type_light:
	sprintf(line, "You light (%c) %s.", item->letter, item_name);
	queue_msg(line);
	break;

      case item_type_ammo:
	sprintf(line, "You ready (%c) %s.", item->letter, item_name);
	queue_msg(line);
	break;

      default:
	sprintf(line, "You put on (%c) %s.", item->letter, item_name);
	queue_msg(line);
	break;
    }
  }
  else if (can_see_creature(game->player, creature))
  {
    switch (item->item_type)
    {
      case item_type_m_weapon:
	sprintf(line, "%s wields %s!", creature->name_one, item_name);
	break;

      case item_type_r_weapon:
      case item_type_wand:
      case item_type_tool:
	sprintf(line, "%s readies %s!",	creature->name_one, item_name);
	break;

      case item_type_light:
	sprintf(line, "%s lights %s.", creature->name_one, item_name);
	break;

      default:
	sprintf(line, "%s puts on %s.", creature->name_one, item_name);
	break;
    }

    upperfirst(line);
    queue_msg(line);
  }

  free(item_name);

  return;
} /* equip_msg */



/*
  Unequips ITEM.

  Returns: false if the item could not be unequipped (usually because
  it's cursed), true on success.

  If MESSAGE is false, messages are suppressed.
*/
blean_t unequip_item(item_t * item, blean_t message)
{
  creature_t * owner;

  if (item == NULL)
    return false;

  if (item->equipped == false)
    return false;

  owner = item->inventory;

  if (is_player(owner) == false)
    message = false;

  item->equipped = false;

  if (item->item_type == item_type_m_weapon)
  {
    owner->attack_pos = -1;
    owner->next_attack = 0;
    advance_pos(owner);
  }
  
  /* Make sure we don't get a current EP that is over the maximum. */
  set_attr(item->inventory, attr_ep_current,
	   MIN(attr_current(item->inventory, attr_ep_max),
	       attr_base(item->inventory, attr_ep_current)));

  if (message)
    unequip_msg(owner, item);

  /* In almost all cases we want to remove the ammo as well */
  if (item->item_type == item_type_r_weapon)
  {
    unequip_item(get_equipped(owner, item_type_ammo, 0), false);
  }

  return true;
} /* unequip_item */



/*
  Displays a message when OWNER has unequipped ITEM.
 */
void unequip_msg(creature_t * owner, item_t * item)
{
  char line[100];
  char * name;

  if (owner == NULL || item == NULL)
    return;
  
  name = get_item_name(item);

  if (is_player(owner))
  {
    switch (item->item_type)
    {
      case item_type_light:
	sprintf(line, "You put out (%c) %s.", item->letter, name);
	break;
	
      case item_type_body:
      case item_type_cloak:
	sprintf(line, "You take off (%c) %s.", item->letter, name);
	break;
	
      case item_type_m_weapon:
      case item_type_r_weapon:
      case item_type_ammo:
      case item_type_wand:
      case item_type_tool:
	sprintf(line, "You put (%c) %s away.", item->letter, name);
	break;
	
      default:
	sprintf(line, "You remove (%c) %s.", item->letter, name);
	break;
    }
  }
  
  free(name);
  queue_msg(line);

  return;
} /* unequip_item */



/*
  Makes CREATURE switch to the next compatible ammunition (any
  ammunition if it doesn't have any ranged weapon) or print a message if
  it can't.
*/
void quiver_cycle(creature_t * creature)
{
  item_t * launcher;
  item_t * temp;
  item_t * original;

  if (creature == NULL)
    return;

  launcher = get_equipped(creature, item_type_r_weapon, 0);

  if (creature->first_item == NULL)
  {
    queue_msg("You don't have any items!");
    return;
  }

  /*
    Cycle around the creatures inventory and look for compatible
    ammo. Start from the currently equipped ammo, or the first item in
    inventory if none could be found.
  */
  original = get_equipped(creature, item_type_ammo, 0);
  
  if (original == NULL)
    temp = creature->first_item;
  else
    temp = original->next_item;

  while (1)
  {
    if (temp == NULL)
    {
      /*
	We're at the end of the inventory. If original is null it
        means we started from the first item so there are no more
        items to check. Otherwise, start over from the beginning.
      */

      if (original != NULL)
	temp = creature->first_item;
      else
	break;
    }

    if (temp == original)
    {
      /* We're back at where we started; there are no other ammo types. */
      queue_msg("You don't have any other ammunition.");
      return;
    }

    if (temp->item_type == item_type_ammo)
    {
      /*
	If we have no launcher anything goes, if we have one the ammo
	must be compatible.
      */
      if (launcher == NULL ||
	  temp->custom[CUSTOM_AMMO_ATYPE] == launcher->custom[WEAPON_AMMO_TYPE])
      {
	/* This ammo is compatible; switch to it. */
	unequip_item(original, false);
	equip_item(temp);

	return;
      }
    }

    temp = temp->next_item;
  }
  
  /* There was no suitable ammo. */
  queue_msg("Out of ammo!");

  return;
} /* quiver_cycle */



/*
  Returns the Nth item of type ITEM_TYPE that OWNER is using.
*/
item_t * get_equipped(const creature_t * owner,
		      const item_type_t item_type,
		      const unsigned int n)
{
  unsigned int count;
  item_t * temp;

  if (owner == NULL)
    return NULL;

  count = 0;

  for (temp = owner->first_item; temp != NULL; temp = temp->next_item)
  {
    if ((temp->item_type == item_type) &&
	(temp->equipped == true))
    {
      if (count == n)
	return temp;
      else
	count++;
    }
  } /* for temp */
  
  return NULL;
} /* get_equipped */



/*
  Counts how many items of type ITEM_TYPE OWNER has. If ONLY_EQUIPPED
  is true it will only count items that are equipped.
*/
unsigned int count_item_type(const creature_t * owner,
			     const item_type_t item_type,
			     const blean_t only_equipped)
{
  unsigned int count;
  item_t * temp;
  
  if (owner == NULL)
    return 0;
  
  count = 0;
  
  for (temp = owner->first_item; temp != NULL; temp = temp->next_item)
  {
    if (item_type == item_type_nofilter || temp->item_type == item_type)
    {
      if ((only_equipped == false) || (temp->equipped == true))
	count++;
    }
  }
  
  return count;
} /* count_item_type */



/*
  Returns any ranged weapon, ammo or wand (in that order) CREATURE has
  equipped. Note that if it returns a ranged weapon, it can also have
  ammo equipped. If it returns ammo or wand, that's all it has.
*/
item_t * equipped_ranged(const creature_t * creature)
{
  item_t * temp;

  temp = get_equipped(creature, item_type_r_weapon, 0);
  if (temp) return temp;

  temp = get_equipped(creature, item_type_ammo, 0);
  if (temp) return temp;

  temp = get_equipped(creature, item_type_wand, 0);
  if (temp) return temp;

  return NULL;
} /* equipped_ranged */



/*
  Player frontend for equipping. Equips ITEM and performs a few useful
  checks. It's intended for the player only, if the item is in someone
  elses inventory strange things can happen.
*/
blean_t try_to_equip(item_t * item)
{
  creature_t * creature;
  
  if (item == NULL || (creature = item->inventory) == NULL)
    return false;

  if (item->equipped)
  {
    queue_msg("You are already using that.");
    return false;
  }
	  
  /* Try to equip the item */
  if (equip_item(item) == false)
    return false;

  /*
    If it was a missile weapon, pick the first available ammo if we
    don't already have any equipped.
  */ 
  if (item->item_type == item_type_r_weapon &&
      get_equipped(creature, item_type_ammo, 0) == NULL)
  {
    quiver_cycle(creature);
  }

  /* If it modifies our carrying capacity... */
  if (get_item_mod(item, attr_carrying_capacity))
    unburdened(creature, get_item_mod(item, attr_carrying_capacity));
    
  /* If it modifies our sight... */
  if (get_item_mod(item, attr_vision) ||
      get_item_mod(item, attr_blindness))
  {
    build_fov(creature);
    update_level_memory(creature->location);
    draw_level();
  }
  
  /*
    If it makes us levitate, and we are not receiving it
    from another source.
  */
  if (get_item_mod(item, attr_levitate) > 0 ||
      attr_current(creature, attr_levitate) == 1)
  {
    change_altitude(creature);
  }
  
  unmapped_abilities(item);
  
  return true;
} /* try_to_equip */



/*
  Player frontend for unequipping. Unequips ITEM and performs some
  useful checks. If MESSAGE is false, messages will be suppressed.
*/
blean_t try_to_unequip(item_t * item, blean_t message)
{
  creature_t * creature;
  signed int temp;

  if (item == NULL || (creature = item->inventory) == NULL)
  {
    return false;
  }

  if (item->equipped == false)
  {
    if (message)
      queue_msg("You are not using that.");

    return false;
  }
	  
  if (unequip_item(item, message) == false)
  {
    queue_msg("You can't remove that.");
    return false;
  }

  /* If we got here, it means unequip_item() succeeded */
  clear_msgbar();
  
  /* If it modifies our carrying capacity... */
  temp = get_item_mod(item, attr_carrying_capacity);
  
  if (temp)
  {
    stagger(creature,
	    attr_current(creature, attr_carrying_capacity) +
	    temp);
  }
  
  /* If it modifies our sight... */
  if (get_item_mod(item, attr_vision) ||
      get_item_mod(item, attr_blindness))
  {
    build_fov(creature);
    update_level_memory(creature->location);
    draw_level();
  }
  
  /*
    If it made us levitate, and we were not receiving it
    from any other source.
  */
  if (get_item_mod(item, attr_levitate) ||
      attr_current(creature, attr_levitate) == 0)
  {
    change_altitude(creature);
  }
  
  clear_msgbar();

  return false;
} /* try_to_unequip */



