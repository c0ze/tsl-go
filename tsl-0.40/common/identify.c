#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "main.h"
#include "stuff.h"
#include "game.h"
#include "item.h"
#include "message.h"
#include "stacks.h"
#include "equip.h"



/*
  Returns the identified status of ITEM. This also checks if the item
  is globally known (e.g. once a shortsword has been identified, all
  shortswords will give their name away).
*/
id_status_t identified(const item_t * item)
{
  id_status_t temp;

  if (item == NULL)
    return known_none;

  temp = item->id;

  if (game->item_known[item->item_number])
    temp |= known_name;

  return temp;
} /* identified */



/*
  Identifies ITEM completely. This also adds this items item number to
  the list of "globally identified" items.
*/
void identify(item_t * item)
{
  if (item == NULL)
    return;

  item->id = known_all;

  make_item_known(item);

  return;
} /* identify */



/*
  Marks ITEM as "globally known" (identifies the name of all similar
  items, but not details like charges).
*/
void make_item_known(const item_t * item)
{
  if (item == NULL)
    return;
  
  game->item_known[item->item_number] = true;

  return;
} /* make_item_number_known */



/*
  Marks all items with item number ID as "globally known".
*/
void make_number_known(const unsigned int number)
{
  /* This also prevents build_item_undefined (0) from becoming known! */
  /* Note to self: And why is this a good thing? */

  if (number <= 0 || number >= ITEMS)
    return;
  
  game->item_known[number] = true;

  return;
} /* make_number_known */



/*
  If the name of ITEM isn't known, identify it and display a message.
*/
void id_if_not_known(item_t * item)
{
  char line[80];

  if (item == NULL)
    return;

  if (almost_wand(item) && (identified(item) & known_charges) == 0)
    item->id |= known_charges;

  if ((identified(item) & known_name) == 0)
  {
    item_article_t art;

    make_item_known(item);

    art = get_item_article(item);

    sprintf(line, "This is %s%s%s.",
	    item_article_a[art],
	    item_article_of[art],
	    item->single_id_name);
    queue_msg(line);
  }

  return;
} /* id_if_not_known */



/*
  Displays what ITEM is if we don't know already and OWNER easily can
  identify it.
*/
void auto_id(creature_t * owner, item_t * item)
{
  char line[100];
  char * name;
  
  if (owner == NULL || item == NULL)
    return;

  if (item->auto_id)
  {
    if ((identified(item) & known_name) == 0)
    {
      identify(item);
      
      name = get_item_name(item);
      
      if (stack_size(item) > 1)
	sprintf(line, "Upon closer inspection, these are %s.", name);
      else
	sprintf(line, "Upon closer inspection, this is %s.", name);
      
      free(name);
      
      queue_msg(line);
    }
  }

  return;
} /* auto_id */



void try_to_id_weapon(creature_t * owner, item_t * weapon)
{
  int i;

  if (owner == NULL || weapon == NULL)
    return;

  for (i = 0; i < 2; i++)
  {
    if ((weapon = get_equipped(owner, item_type_m_weapon, i)) != NULL)
    {
      weapon->custom[WEAPON_ATKSEQ_ID + owner->attack_pos] = true;
    
      if (weapon->custom[WEAPON_ATKSEQ + owner->attack_pos + 1] == 0)
	id_if_not_known(weapon);
    }
  }

  return;
}
