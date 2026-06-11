#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <ctype.h>

#include "main.h"
#include "stuff.h"
#include "dwiminv.h"
#include "select.h"
#include "item.h"
#include "inventory.h"
#include "creature.h"
#include "input.h"
#include "ui.h"
#include "actions.h"
#include "equip.h"
#include "find.h"
#include "stacks.h"
#include "craft.h"

#ifdef TSL_CONSOLE
#include "glyph.h"
#endif



/*
  Opens a submenu for ITEM.
*/
int browser_item_submenu(creature_t * creature, item_t * item)
{
  menu_item_t ** submenu;
  int count;
  int selection;
  int i;
  int lact;
  blean_t turn_spent;
  char tmpstr[50];
  item_t * extra_item;
  int temp;

  if (creature == NULL || item == NULL)
    return 0;

  turn_spent = false;

  submenu = malloc(sizeof(menu_item_t *) * SUBMENU_SIZE);

  if (submenu == NULL)
    out_of_memory();

  for (i = 0; i < SUBMENU_SIZE; i++)
    submenu[i] = alloc_menu_item();

  count = 0;

  /* Build the submenu. The default action for the item goes first. */
  submenu[count]->label = mydup(get_default_verb(item));
  upperfirst(submenu[count]->label);
  submenu[count]->act = MENUACT_USE;
  count++;

  if (almost_wand(item))
  {
    if (item->equipped)
      submenu[count]->label = mydup("Put away");
    else
      submenu[count]->label = mydup("Ready");

    submenu[count]->act = MENUACT_EQUIP;
    count++;
  }

  /*
    If the item hasn't been identified and the owner has the means to
    identify it, add an Identify option.
  */
  if (attr_current(creature, attr_blindness) == 0 &&
      attr_current(creature, attr_p_read) == 0 &&
      ((identified(item) & known_name) == 0 ||
       (almost_wand(item) && (identified(item) & known_charges) == 0)) &&
      (extra_item = find_known_item(creature, treasure_s_identify)) != NULL)
  {
    temp = stack_size(extra_item);

    if (temp > 1)
      sprintf(tmpstr, "Identify (%d scrolls)", temp);
    else
      strcpy(tmpstr, "Identify (scroll)");

    submenu[count]->label = mydup(tmpstr);
    submenu[count]->act = MENUACT_IDENTIFY;
    count++;
  }

  /*
    If the item can be recharged and the owner has a recharge scroll.
  */
  if (is_rechargeable(item) &&
      (extra_item = find_known_item(creature, treasure_s_recharge)) != NULL)
  {
    temp = stack_size(extra_item);

    if (temp > 1)
      sprintf(tmpstr, "Recharge (%d scrolls)", temp);
    else
      strcpy(tmpstr, "Recharge (scroll)");

    submenu[count]->label = mydup(tmpstr);
    submenu[count]->act = MENUACT_RECHARGE;
    count++;
  }

  /* These are standard for all items */
  submenu[count]->label = mydup("Drop");
  submenu[count]->act = MENUACT_DROP;
  count++;

  if (apply_func(item))
  {
    submenu[count]->label = mydup("Apply");
    submenu[count]->act = MENUACT_APPLY;
    count++;
  }

  if (item->item_type != item_type_food)
  {
    submenu[count]->label = mydup("Eat");
    submenu[count]->act = MENUACT_EAT;
    count++;
  }

  submenu[count]->label = mydup("Label");
  submenu[count]->act = MENUACT_LABEL;
  count++;

  while (1)
  {
    selection = browse(submenu, count, MENU_GENERIC, item, NULL);

    if (selection == -1)
      goto cleanup;

    lact = submenu[selection]->act;
    del_menu(submenu, SUBMENU_SIZE);
    return lact;
  }

cleanup:
  
  del_menu(submenu, SUBMENU_SIZE);
  
  if (turn_spent)
    return 1;
      
  return 0;
}


int dwim_pickup(creature_t * creature)
{
  item_t ** item_list;
  menu_item_t ** menu_list;
  int total_items;
  blean_t turn_spent;
  int selection;
  int i;
  browser_t * b;

  total_items = find_items(creature->location,
			   creature->y, creature->x, &item_list);

  b = &browser[MENU_PICKUP];

  /* These reset every time for pickup */
  b->cur_pos = 0;
  b->start_row = 0;

  turn_spent = false;

  if (total_items == 0)
  {
    queue_msg("Nothing here.");
    return false;
  }
  else if (total_items == 1)
  {
    if (pickup(creature, item_list[0], false))
    {
      free(item_list);
      return true;
    }
    else
    {
      free(item_list);
      return false;
    }
  }
  else
  {
    sort_items(item_list, total_items, NULL, false);
    menu_list = make_item_menu(item_list, total_items);

    for (i = 0; i < total_items; i++)
      menu_item_explain_pickup(menu_list[i]);

    while (1)
    {
      selection = browse(menu_list, total_items, MENU_PICKUP, NULL, NULL);

      if (selection == -1)
	goto cleanup;

      if (pickup(creature, menu_list[selection]->item, false))
      {
	browser_remove_item(b, menu_list, selection);
	total_items--;
      }

      if (b->total_items == 0)
	goto cleanup;
      
      msgflush_nowait();
    }
  }

cleanup:
  free(item_list);
  del_menu(menu_list, total_items);
  
  if (turn_spent)
    return true;

  return false;
}



/*
  Lets the player select an items from CREATUREs inventory.
  Returns the item selected or NULL if cancelled.
*/
item_t * dwim_select(creature_t * creature, blean_t (* criteria) (const item_t *), char * order)
{
  item_t ** item_list;
  menu_item_t ** menu_list;
  item_t * temp;
  int total_items;
  int selection;
  browser_t * b;
  item_t * ret;
  int count;

  ret = NULL;

  /* These are reset */
  b = &browser[MENU_PICK];
  b->cur_pos = 0;
  b->start_row = 0;

  total_items = count_inventory(creature, criteria);

  if (total_items == 0)
    return NULL;

  /* Allocate an array of item_t *pointers* */
  item_list = malloc(sizeof(item_t *) * total_items);

  if (item_list == NULL)
    out_of_memory();

  count = 0;

  for (temp = creature->first_item; temp != NULL; temp = temp->next_item)
  {
    if (criteria == NULL || criteria(temp))
    {
      item_list[count] = temp;
      count++;
    }
  }

  sort_items(item_list, count, order, false);
  menu_list = make_item_menu(item_list, count);
  
  while (1)
  {
    selection = browse(menu_list, total_items, MENU_PICK, NULL, NULL);
    
    if (selection == -1)
      goto cleanup;

    ret = menu_list[selection]->item;
    goto cleanup;
  }

cleanup:
  free(item_list);
  del_menu(menu_list, total_items);
  
  return ret;
} /* dwim_select */



int dwim_inventory(creature_t * creature, blean_t (* criteria) (const item_t *), char * order)
{
  int total_items;
  int count;
  int shit;
  item_t ** list;
  item_t * temp;
  item_t * t2;
  menu_item_t ** second_list;
  blean_t turn_spent;
  int action;
  browser_t * b;
  int selection;

  if (creature == NULL)
    return 0;

  turn_spent = false;
  b = &browser[MENU_USE];

  total_items = count_inventory(creature, criteria);

  if (total_items == 0)
  {
    queue_msg("You do not have any items.");
    return 0;
  }

  /* Allocate an array of item_t *pointers* */
  list = malloc(sizeof(item_t *) * total_items);

  if (list == NULL)
    out_of_memory();

  count = 0;

  for (temp = creature->first_item; temp != NULL; temp = temp->next_item)
  {
    if (criteria == NULL || criteria(temp))
    {
      list[count] = temp;
      count++;
    }
  }

  sort_items(list, count, order, false);
  second_list = make_item_menu(list, count);

  while(1)
  {
    selection = browse(second_list, count, MENU_USE, NULL, &action);

    if (selection == -1)
      goto cleanup;
    
    if (action == action_select)
    {
    use_item:
      if (dwim_item(creature, second_list[selection]->item))
      {
	turn_spent = true;
	goto cleanup;
      }

      temp = second_list[selection]->item;
      del_menu_item(second_list[selection]);
      second_list[selection] = item_to_menu_item(temp);
      menu_item_add_explanation(second_list[selection]);
	
      msgflush_nowait();
    }
    else if (action == action_flip)
    {
      shit = browser_item_submenu(creature, second_list[selection]->item);

      temp = second_list[selection]->item;
      del_menu_item(second_list[selection]);
      second_list[selection] = item_to_menu_item(temp);
      menu_item_add_explanation(second_list[selection]);

      switch (shit)
      {
	case MENUACT_DROP:
	  goto drop_item;
	  
	case MENUACT_USE:
	  goto use_item;

	case MENUACT_EQUIP:
	  if (temp->equipped)
	    try_to_unequip(temp, true);
	  else
	    try_to_equip(temp);
	  
	  msgflush_nowait();
	  break;

	case MENUACT_IDENTIFY:
	  del_item(get_item_from_stack(find_known_item(creature, treasure_s_identify)));
	  id_if_not_known(temp);
	  turn_spent = true;
	  goto cleanup;

	case MENUACT_RECHARGE:
	  t2 = get_item_from_stack(find_known_item(creature, treasure_s_recharge));
	  detach_item(t2);
	  recharge(creature, temp, t2);
	  del_item(t2);
	  turn_spent = true;
	  goto cleanup;

	case MENUACT_LABEL:
	  if (attr_current(creature, attr_p_read))
	  {
	    queue_msg("Unfortunately, you can not read (or write).");
	    msgflush_nowait();
	    break;
	  }
	  else
	  {
	    label_prompt(temp);
	    del_menu_item(second_list[selection]);
	    second_list[selection] = item_to_menu_item(temp);
	    menu_item_add_explanation(second_list[selection]);
	    break; //goto cleanup;
	  }
	  break;

	case MENUACT_EAT:
	  turn_spent = eat(creature, temp);
	  goto cleanup;

	case MENUACT_APPLY:
	  turn_spent = apply(creature, temp);
	  goto cleanup;

	case 0:
	  break;

	default:
	  turn_spent = true;
	  goto cleanup;
	  //	  break;
      }
    }
    else if (action == action_drop)
    {
      int stuff;
      int quantity;

    drop_item:
	
      temp = second_list[selection]->item;

      quantity = stack_size(temp);
      
      if (quantity == 1)
      {
	try_to_unequip(temp, false);
	drop_item(creature, temp, false);
	stuff = 1;
	turn_spent = true;
      }
      else
      {
	stuff = drop_stack(creature, temp);
	turn_spent = true;
      }
      
      del_menu_item(second_list[selection]);
      second_list[selection] = item_to_menu_item(temp);
      menu_item_add_explanation(second_list[selection]);
      
      /* Remove item from list, but only if the whole stack was removed.  */
      if (stuff == quantity)
      {
	browser_remove_item(b, second_list, selection);
	count--;

	if (b->total_items == 0)
	  goto cleanup;
      }
      
      msgflush_nowait();
    }
  }

cleanup:
  del_menu(second_list, count);
  free(list);

  if (turn_spent)
    return 1;
    
  return 0;
} /* dwim_inventory */






/*
  Draws a browser screen with the vertical separators at SEP1 and SEP2
  rows from the top.
*/
void draw_browser(const int sep1, const int sep2)
{
  int i;

  for (i = 1; i < sep2; i++)
  {
    st_move(i, 0);
    st_special(ST_VLINE);
    st_move(i, 38);
    st_special(ST_VLINE);
  }
  
  for (i = 1; i < 38; i++)
  {
    st_move(0, i);
    st_special(ST_HLINE);
    st_move(sep1, i);
    st_special(ST_HLINE);
    st_move(sep2, i);
    st_special(ST_HLINE);
  }
  
  st_move(0, 0);
  st_special(ST_SE);
  
  st_move(0, 37);
  st_special(ST_SW);
  st_move(1, 38);
  st_addch(' ');
  st_move(1, 37);
  st_special(ST_NE);
  st_move(1, 38);
  st_special(ST_SW);
  
  st_move(sep1, 0);
  st_special(ST_LTEE);
  st_move(sep1, 38);
  st_special(ST_RTEE);
  
  st_move(sep2, 0);
  st_special(ST_NE);
  st_move(sep2, 38);
  st_special(ST_NW);

  return;
} /* draw_browser */



int drop_stack(creature_t * creature, item_t * item)
{
  item_t * new_stack;
  item_type_t type_temp;
  int number;
  int quantity;

  if (creature == NULL || item == NULL)
    return 0;
  
  quantity = stack_size(item);
  number = read_number("How many?");
  
  if (number < 0)
  {
    queue_msg("!?");
    return 0;
  }
  else if (number == 0)
  {
    queue_msg("Never mind, then.");
    return 0;
  }
  else if (number > quantity)
  {
    queue_msg("You don't have that many!");
    return 0;
  }
  
  if (number == stack_size(item))
  {
    try_to_unequip(item, false);
    drop_item(creature, item, false);
    return number;
  }

  new_stack = split_stack(item, number);
  
  if (new_stack == NULL)
  {
    queue_msg("BUG: Error splitting stack, sorry.");
    return 0;
  }
  
  if (stack_size(new_stack) != number)
  {
    queue_msg("BUG: split_stack() returned wrong size, sorry.");
    return 0;
  }
  
  /*
    Ugly hack; change the item type of the original
    stack temporarily, so that we don't combine the
    stacks again when attaching the new stack.
  */
  type_temp = item->item_type;
  item->item_type = 999;
  
//  attach_item_to_creature(creature, new_stack);
  
  item->item_type = type_temp;
  
//  try_to_unequip(new_stack, false);
  drop_item(creature, new_stack, false);

//  del_item(new_stack);
  
  return number;
}
  
  
  
/*
  Returns 1 if the action consumed a turn.
*/
int dwim_item(creature_t * who, item_t * what)
{
  if (who == NULL || what == NULL)
    return 0;

  if (almost_wand(what))
  {
    if (use_item(who, what))
      return 1;
    else
      return 0;
  }

  if (is_equipable(what))
  {
    if (what->equipped)
      try_to_unequip(what, true);
    else
      try_to_equip(what);

    return 0;
  }

  switch (what->item_type)
  {
    case item_type_potion:
    case item_type_book:
    case item_type_scroll:
    case item_type_tool:
    case item_type_food:

      if (what->item_type == item_type_potion &&
	  attr_current(who, attr_p_drink))
      {
	queue_msg(MSG_UNABLE_TO_DRINK);
	return 0;
      }

      if (what->item_type == item_type_food &&
	  attr_current(who, attr_p_eat))
      {
	queue_msg(MSG_UNABLE_TO_EAT);
	return 0;
      }
      
      if (use_item(who, what))
	return 1;
      else
	return 0;

    default:
      queue_msg(what->single_id_name);
      break;
  }

  return 0;
} /* dwim_item */



void browser_remove_item(browser_t * b, menu_item_t ** list, int it)
{
  int i;

  del_menu_item(list[it]);

  for (i = it + 1; i < b->total_items; i++)
    list[i - 1] = list[i];
  
  b->total_items--;
  list[b->total_items] = NULL;
  
  if (b->cur_pos >= b->total_items)
    b->cur_pos = b->total_items - 1;
  
  return;
}
