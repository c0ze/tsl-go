#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <ctype.h>

#include "main.h"
#include "stuff.h"
#include "select.h"
#include "item.h"
#include "inventory.h"
#include "creature.h"
#include "input.h"
#include "ui.h"

#ifdef TSL_CONSOLE
#include "glyph.h"
#endif



/*
  Sorts the items in LIST according to item type. ITEMS must be the
  number of items in the list.
  
  If ORDER is non-null, it will override the default order items are
  listed.  If RESET_LETTERS is true, items will be re-enumerated with
  item_letters _after_ they have been sorted into categories.
*/
void sort_items(item_t ** list, const unsigned int items, const char * order, const blean_t reset_letters)
{
  blean_t changed;
  unsigned int i;
  item_t * temp;
  unsigned int priority[50];

  /*
    Each index of priority represents an item type. The ones with
    lowest value will go first.  By default it will use the
    item_type_t enum order (the 50+ values). Any single type that has
    a lower value will end up above all others.
  */

  for (i = 0; i < 50; i++)
    priority[i] = 50 + i;

  if (order)
  {
    /*
      We wish to override the item order. Scan the order string for
      certain letters, if it matches an item type make this types
      priority the same as its index in the string.
    */

    i = 0;

    while (order[i] != '\0')
    {
      switch (order[i])
      {
	case 'a': priority[item_type_ammo] = i; break;
	case 'm': priority[item_type_m_weapon] = i; break;
	case 'r': priority[item_type_r_weapon] = i; break;
	case 'w': priority[item_type_wand] = i; break;
	case 'p': priority[item_type_potion] = i; break;
	case 's': priority[item_type_scroll] = i; break;
	case 'b': priority[item_type_book] = i; break;
	case 'l': priority[item_type_light] = i; break;
	case 't': priority[item_type_tool] = i; break;
	case 'B': priority[sort_type_bow] = i; break;
	case 'A': priority[sort_type_arrow] = i; break;
	case 'S': priority[sort_type_shotgun] = i; break;
	case 'H': priority[sort_type_shell] = i; break;
	case 'C': priority[sort_type_crossbow] = i; break;
	case 'D': priority[sort_type_dart] = i; break;
      }

      i++;
    }
  }

  /* Bubble sort. */
  do
  {
    changed = false;

    for (i = 1; i < items; i++)
    {
      if (priority[list[i - 1]->sort_type] > priority[list[i]->sort_type])// ||
//	  compare_items(list[i - 1], list[i]))
      {
	temp = list[i - 1];
	list[i - 1] = list[i];
	list[i] = temp;
	changed = true;
      }
    }
  } while (changed);

  if (reset_letters)
  {
    for (i = 0; i < items; i++)
      list[i]->letter = item_letters[i];
  }

  return;
} /* sort_items */





/*
blean_t compare_items(item_t * a, item_t * b)
{
  if (a == NULL || b == NULL)
    return;

  if (a->item_type == item_type_ranged && a->item_type == item_type_ammo)
  {
    if (a->item_number == treasure_shotgun)
    {
      if (b->item_number == treasure_s_)
      {
	return true;
      }
    }
  }

  return false;
}
*/
