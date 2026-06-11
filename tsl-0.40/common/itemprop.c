#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "main.h"
#include "stuff.h"
#include "item.h"
#include "stacks.h"
#include "craft.h"
#include "treasure.h"
#include "actions.h"



/*
  See item.h for prototypes.
*/



/*
  Checks if ITEM is of a type that can be equipped. Returns true if
  the item type can be equipped, otherwise false.
*/
blean_t is_equipable(const item_t * item)
{
  if (item->item_type == item_type_m_weapon ||
      item->item_type == item_type_r_weapon ||
      item->item_type == item_type_body ||
      item->item_type == item_type_light ||
      item->item_type == item_type_feet ||
      item->item_type == item_type_head ||
      item->item_type == item_type_cloak ||
      item->item_type == item_type_ammo ||
      item->item_type == item_type_wand ||
      almost_wand(item))
  {
    return true;
  }
  
  return false;
} /* is_equipable */



/*
  Returns true if ITEM can be recharged, otherwise false.
*/
blean_t is_rechargeable(const item_t * item)
{
  if (item == NULL)
    return false;

  return item->rechargeable;
} /* is_rechargeable */



/*
  Returns whether ITEM is edible or not.
 */
blean_t is_edible(const item_t * item)
{
  if (item == NULL)
    return false;

  return item->edible;
} /* is_edible */



/*
  Returns whether ITEM can be applied. This is just intended as a
  wrapper for apply_func, so we can pass it as item criteria.
*/
blean_t can_apply(const item_t * item)
{
  if (item == NULL)
    return false;

  if (apply_func(item))
    return true;

  return false;
} /* can_apply */



/*
  Returns true if ITEM can be read, otherwise false.
*/
blean_t is_readable(const item_t * item)
{
  if (item == NULL)
    return false;

  if (item->item_type == item_type_scroll ||
      item->item_type == item_type_book)
    return true;

  return false;
} /* is_readable */



/*
  Returns true if ITEM is equipped (this also requires that the item
  *can* be equipped), otherwise false.
*/
blean_t is_equipped(const item_t * item)
{
  if (item == NULL)
    return false;

  if (is_equipable(item) == false)
    return false;

  return item->equipped;
} /* is_equipped */



/*
  Returns true if ITEM can be drunk, otherwise false.
*/
blean_t is_drinkable(const item_t * item)
{
  if (item == NULL)
    return false;

  if (item->item_type == item_type_potion)
    return true;
  else
    return false;
} /* is_drinkable */



/*
  Returns true if ITEM is a tool that can be 'a'pplied, otherwise false.
*/
blean_t can_activate(const item_t * item)
{
  if (item == NULL)
    return false;

  if (use_func(item))
    return true;

  if (almost_wand(item))
    return true;

  return false;
} /* can_activate */



blean_t almost_wand(const item_t * item)
{
  if (item == NULL)
    return false;

  if (item->item_type == item_type_scroll)
    return false;

  if (item->invoke_power != 0)
  {
    if (item->item_type == item_type_wand)
      return true;

    if (identified(item) & known_name)
      return true;
  }

  return false;
} /* almost_wand */



/*
  Returns true if ITEM can be eaten.
*/
blean_t is_food(const item_t * item)
{
  if (item == NULL)
    return false;

  if (item->item_type == item_type_food)
    return true;
  else
    return false;
} /* is_food */



/*
  Returns if ITEM is explosive. Explosive items should have a min and max damage set just like a weapon.
*/
blean_t explosive(const item_t * item)
{
  if (item == NULL)
    return false;

  switch (item->item_number)
  {
    case treasure_grenade:
    case treasure_lantern:
    case artifact_everlasting_lantern:
    case treasure_fireball:
      return true;

    default:
      return false;
  }
} /* explosive */
