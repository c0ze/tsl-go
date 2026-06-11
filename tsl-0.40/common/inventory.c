#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "main.h"
#include "stuff.h"
#include "inventory.h"
#include "creature.h"
#include "level.h"
#include "player.h"
#include "stacks.h"
#include "actions.h"
#include "equip.h"



/*
  Adds ITEM to CREATUREs inventory.
  Returns: NULL on success, otherwise ITEM.
*/
item_t * attach_item_to_creature(creature_t * creature, item_t * item)
{
  item_t * temp;
  unsigned int letters;
  unsigned int i;

  if ((creature == NULL) || (item == NULL))
    return item;
  
  /* Remove the item from wherever it is right now. */
  detach_item(item);

  /*
    See if there is any item in the creatures inventory that we can
    stack the new item with.
  */
  for (temp = creature->first_item; temp != NULL; temp = temp->next_item)
  {
    if (can_stack(temp, item))
    {
      /*
	"Ok everyone, move over to this other item here..."
	
	Move the items one at a time so there's no danger of creating
        a stack within a stack.
      */
      while (item->child != NULL)
	attach_item_to_stack(temp, item->child);

      attach_item_to_stack(temp, item);
      return NULL;
    }
  }

  /* There was no suitable stack; it needs a slot of its own. */

  /* Find the first free letter and assign it to the item */
  letters = strlen(item_letters);
  
  for (i = 0; i <= letters; i++)
  {
    if (get_item_by_letter(creature, item_letters[i]) == NULL)
    {
      item->letter = item_letters[i];
      break;
    }
  }
  
  /*
    Insert the new item at the beginning of the linked list of
    items.
  */
  item->prev_item = NULL;
  item->next_item = creature->first_item;
  creature->first_item = item;
  
  /*
    If there already was one or more items in the inventory, repoint
    the first items "previous item" pointer to the new first item.
  */
  if (item->next_item != NULL)
    item->next_item->prev_item = item;
  
  item->inventory = creature;
  item->location = NULL;
  item->equipped = false;
  
  return NULL;
} /* attach_item_to_creature */



/*
  Returns the item in CREATUREs inventory that has letter
  SOUGHT_LETTER, or NULL if none is found.
*/
item_t * get_item_by_letter(const creature_t * creature,
			    const char sought_letter)
{
  item_t * item;

  if (creature == NULL)
    return NULL;

  for (item = creature->first_item; item != NULL; item = item->next_item)
  {
    if (item->letter == sought_letter)
      return item;
  }

  return NULL;
} /* get_item_by_letter */



/*
  Counts the number of items present in CREATUREs inventory. Note that
  this only counts the number of item *stacks* (containing 1 or more
  items); stack sizes aren't included.
*/
int count_inventory(const creature_t * owner,  blean_t (* criteria) (const item_t *))
{
  int count;
  item_t * temp;
  
  if (owner == NULL)
    return 0;

  count = 0;

  for (temp = owner->first_item; temp != NULL; temp = temp->next_item)
  {
    if (criteria == NULL || criteria(temp))
      count++;
  }

  return count;
} /* count_inventory */



/*
  Tries to equip ITEM on CREATURE. If the creature for any reason
  can't use the item, the item is destroyed. This is intended for
  outfitting creatures at their creation and we want them to only
  carry one item of this type. Returns true if the item was equipped
  properly, false if it was destroyed (pointer is invalid!).
*/
blean_t use_or_destroy(creature_t * creature, item_t * item)
{
  if ((creature == NULL) || (item == NULL))
    return false;

  if (attach_item_to_creature(creature, item) != NULL)
  {
    del_item(item);
    return false;
  }

  if (equip_item(item) == false)
  {
    del_item(item);
    return false;
  }

  return true;
} /* use_of_destroy */



/*
  Moves all items B has to As inventory, and from A to B.
*/
void switch_inventory(creature_t * a, creature_t * b)
{
  item_t * temp;

  if ((a == NULL) || (b == NULL))
    return;

  temp = b->first_item;

  b->first_item = a->first_item;
  a->first_item = temp;

  /*
    Inventories switches; now we need to loop through each item and
    update their owner.
  */

  temp = a->first_item;

  while (temp != NULL)
  {
    temp->inventory = a;
    temp = temp->next_item;
  }

  temp = b->first_item;

  while (temp != NULL)
  {
    temp->inventory = b;
    temp = temp->next_item;
  }

  return;
} /* move_all_items */



/*
  Finds the first occurence of an item with number ITEM_ID in OWNERs inventory.
*/
item_t * find_item_in_inventory(const creature_t * owner,
				const unsigned int number)
{
  item_t * temp;

  if (owner == NULL)
    return NULL;

  for (temp = owner->first_item; temp != NULL; temp = temp->next_item)
  {
    if (temp->item_number == number)
      return temp;
  }

  return NULL;
} /* find_item_in_inventory */



/*
*/
item_t * find_known_item(creature_t * owner, const int treasure)
{
  item_t * temp;

  temp = find_item_in_inventory(owner, treasure);

  if (temp != NULL &&
      identified(temp) & known_name)
  {
    return temp;
  }

  return NULL;
} /* find_item */
