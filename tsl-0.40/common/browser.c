#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "browser.h"
#include "input.h"
#include "ui.h"


browser_t browser[10];


/*
  Initializes the browser contexts we need.
*/
void init_browsers(void)
{
  int i;
  int j;

  for (i = 0; i < 10; i++)
    for (j = 0; j < BRCT_KEYS; j++)
      browser[i].keys[j] = action_select;

  browser[MENU_USE].desc_h = 7;
  browser[MENU_USE].list_h = 13;
  browser[MENU_USE].cur_pos = 0;
  browser[MENU_USE].start_row = 0;
  browser[MENU_USE].have_gents = true;
  browser[MENU_USE].keys[0] = action_flip;
  browser[MENU_USE].keys[1] = action_drop;

  browser[MENU_PICK].desc_h = 7;
  browser[MENU_PICK].list_h = 13;
  browser[MENU_PICK].cur_pos = 0;
  browser[MENU_PICK].start_row = 0;
  browser[MENU_PICK].have_gents = true;

  browser[MENU_GENERIC].desc_h = 7;
  browser[MENU_GENERIC].list_h = 13;
  browser[MENU_GENERIC].cur_pos = 0;
  browser[MENU_GENERIC].start_row = 0;
  browser[MENU_GENERIC].have_gents = false;

  browser[MENU_PICKUP].desc_h = 7;
  browser[MENU_PICKUP].list_h = 13;
  browser[MENU_PICKUP].cur_pos = 0;
  browser[MENU_PICKUP].start_row = 0;
  browser[MENU_PICKUP].have_gents = true;
  browser[MENU_PICKUP].keys[1] = action_pickup;

  browser[MENU_FACETS].desc_h = 15;
  browser[MENU_FACETS].list_h = 5;
  browser[MENU_FACETS].cur_pos = 0;
  browser[MENU_FACETS].start_row = 0;
  browser[MENU_FACETS].have_gents = false;

  browser[MENU_OPTIONS].desc_h = 15;
  browser[MENU_OPTIONS].list_h = 5;
  browser[MENU_OPTIONS].cur_pos = 0;
  browser[MENU_OPTIONS].start_row = 0;
  browser[MENU_OPTIONS].have_gents = false;

  browser[MENU_ABILITIES].desc_h = 7;
  browser[MENU_ABILITIES].list_h = 13;
  browser[MENU_ABILITIES].cur_pos = 0;
  browser[MENU_ABILITIES].start_row = 0;
  browser[MENU_ABILITIES].have_gents = false;

  browser[MENU_BESTIARY].desc_h = 12;
  browser[MENU_BESTIARY].list_h = 8;
  browser[MENU_BESTIARY].cur_pos = 0;
  browser[MENU_BESTIARY].start_row = 0;
  browser[MENU_BESTIARY].have_gents = false;

  return;
} /* init_item_browser */



/*
  Lets the player browse LIST (with TOTITMS elements) in browser
  context MODE.
*/
int browse(menu_item_t ** list, int totitms, const int mode,
	   item_t * inspect_item, int * ret_action)
{
  browser_t * b;
  item_t * tmpitem;
  char gent_slot[5];
  int sep1;
  int sep2;
  int item_i;
  int rows_spent;
  char * last_category;
  char line[1000];
  char itemname[100];
  char * tmpstr;
  char * strp;
  key_token_t input;
  action_t action;
  int i;
  blean_t more_above;
  blean_t more_below;
  char in_use[10];

#ifdef TSL_CONSOLE
  strcpy(gent_slot, "  ");
#else
  strcpy(gent_slot, "   ");
#endif

  b = &browser[mode];

  b->total_items = totitms;

  if (b->cur_pos >= b->total_items)
    b->cur_pos = b->total_items - 1;

  if (b->cur_pos < 0)
    b->cur_pos = 0;

  browser_scroll(b, list, b->total_items);

  while (true)
  {
  redraw_list:

    st_erase();
    st_move(0, 0);

    sep1 = 2 + b->desc_h;
    sep2 = 3 + b->desc_h + b->list_h;

    draw_browser(sep1, sep2);

    if (inspect_item)
      tmpitem = inspect_item;
    else if (list[b->cur_pos]->item)
      tmpitem = list[b->cur_pos]->item;
    else
      tmpitem = NULL;

    /* Display information about the selected item  */
    if (tmpitem)
    {
      tmpstr = get_inv_item_name(tmpitem);
      sprintf(line, "%s%s", gent_slot, tmpstr);
      free(tmpstr);

      st_move(1, 1);
      st_addstr(line);

      st_move(1, 1);
      st_gent(tmpitem->gent);
      
      if (identified(tmpitem) & known_name ||
	  tmpitem->item_type == item_type_m_weapon)
      {
	tmpstr = get_item_data(tmpitem);
      }
      else
	tmpstr = mydup("(not identified)");

      constrained_wrap(2, 41, 38, tmpstr);
      free(tmpstr);
    }
    else if (list[b->cur_pos]->inspect != NULL)
    {
      constrained_wrap(2, 41, 38, list[b->cur_pos]->inspect);
    }

    rows_spent = 0;
    last_category = NULL;
    more_below = false;

    if (b->start_row > 0)
      more_above = true;
    else
      more_above = false;

    for (i = 0; i < b->list_h + 1; i++)
    {
      item_i = b->start_row + i;
      
      if (item_i >= b->total_items)
	break;
      
      if (rows_spent >= b->list_h)
      {
	more_below = true;
	break;
      }

      tmpitem = list[item_i]->item;

      if (list[item_i]->category != NULL &&
	  mycmp(list[item_i]->category, last_category) == false)
      {
	last_category = list[item_i]->category;
	sprintf(line, "(%s)", last_category);

	//st_move(sep1 + 1 + rows_spent, 38 - strlen(line));
	st_move(sep1 + 1 + rows_spent, 1);

	st_dim(true);
	st_addstr(line);
	st_dim(false);

	rows_spent++;
      }
      
      if (rows_spent >= b->list_h)
      {
	more_below = true;
	break;
      }

      strcpy(itemname, list[item_i]->label);

      sprintf(line, "%c) ", list[item_i]->letter);
//      strcpy(line, "");
      
/*      if (b->have_gents)
      {
	strcat(line, gent_slot);
	}*/
      
      if (tmpitem)
      {
	strp = line + strlen(line);

	in_use_str(in_use, tmpitem);

	if (tmpitem->equipped == true)
	{
	  strncat(strp, itemname, 31 - strlen(in_use));
	  strcat(strp, " ");
	  strcat(strp, in_use);
	  //sprintf(strp, "%.29s %s", itemname, in_use);
	}
	else
	  sprintf(strp, "%.31s", itemname);
      }
      else
      {
	strcpy(line, list[item_i]->label);
      }
      
      if (item_i == b->cur_pos)
      {
	st_move(sep1 + 1 + rows_spent, 1);
	st_rev(true);
	st_addstr("                                     ");
	st_move(sep1 + 1 + rows_spent, 2);
	st_addstr(line);

	if (b->have_gents && tmpitem)
	{
	  st_move(sep1 + 1 + rows_spent, 36);
	  st_gent(tmpitem->gent);
	}

	st_rev(false);
	
	if (list[item_i]->explanation)
	{
	  st_move(sep2, 2);
	  st_addstr(list[item_i]->explanation);
	}
      }
      else
      {
	st_move(sep1 + 1 + rows_spent, 2);
	st_addstr(line);

	if (b->have_gents)
	{
	  st_move(sep1 + 1 + rows_spent, 36);
	  st_gent(tmpitem->gent);
	}
      }

      rows_spent++;
    }
       
    if (more_above)
    {
      st_move(3 + b->desc_h, 38);
      st_special(ST_CROSS);
    }

    if (more_below)
    {
      st_move(2 + b->desc_h + b->list_h, 38);
      st_special(ST_CROSS);
    }

    /* Done printing. */
    st_flush();

    /* Get input */
    while (1)
    {
      input = get_keypress();
        
      action = key_to_action(input);

      if (input == kt_dir_up ||
	  input == kt_joy_n
	  /*action == action_n*/)
      {
	if (b->cur_pos > 0)
	{
	  b->cur_pos--;
	  browser_scroll(b, list, totitms);
	  break;
	}
      }
      else if (input == kt_dir_down ||
	       input == kt_joy_s /*action == action_s*/)
      {
	if (b->cur_pos < b->total_items - 1)
	{
	  b->cur_pos++;
	  browser_scroll(b, list, totitms);
	  break;
	}
      }
      else if (input == kt_page_down)
      {
	b->cur_pos = MIN(b->cur_pos + (b->list_h - 2), totitms - 1);
	browser_scroll(b, list, totitms);
	break;
      }
      else if (input == kt_page_up)
      {
	b->cur_pos = MAX(b->cur_pos - (b->list_h - 2), 0);
	browser_scroll(b, list, totitms);
	break;
      }
      else if (action == action_cancel)
      {
	return -1;
      }

      /* Check if user pressed one of the keys accepted by this browser context */
      for (i = 0; i < BRCT_KEYS; i++)
      {
	if (action == b->keys[i])
	{
	  if (ret_action != NULL)
	  {
	    *ret_action = action;
	  }
	  
	  return b->cur_pos;
	}
      }

      /* Check if user pressed an index letter */
      for (i = 0; i < totitms; i++)
      {
	if (input == list[i]->letter)
	{
	  b->cur_pos = i;
	  /*return i;*/
	  /*break;*/

	  browser_scroll(b, list, b->total_items);
	  goto redraw_list;
	}
      }

    } /* keypress */
  } /* selection */
} /* browse */



/*
  Checks if browser B (displaying LIST with TOTITMS elements) needs to
  scroll. Scrolling will place the cursor as the second selectable row
  at the top or bottom.
*/
void browser_scroll(browser_t * b, menu_item_t ** list, int totitms)
{
  int i;
  int rows_spent;
  int item_i;
  char * last_category;
  blean_t overshoot;
  int dir;
  int top_stop;
  int bottom_stop;
  int bottom_stop2;
  int screen_pos;
  int current_w;
  
  if (b == NULL)
    return;
  
  current_w = (b->cur_pos - 1) / (b->list_h - 2);
  
  if (totitms > 2 &&
      b->cur_pos == totitms - 1 &&
      b->cur_pos % (b->list_h - 2) == 1)
  {
    current_w -= 1;
  }

  b->start_row = current_w * (b->list_h - 2);

  if (current_w < 0)
  {
    queue_msg("BUG: bad scrolling, sorry.");
    b->start_row = 0;
    return;
  }
  
  return;


  dir = 0;
 
  /*
    Each pass through this loop will calculate which screen row the
    menu cursor is at. If it's too far in either direction, we will
    move the browser start row one step up/down, recalculate, and
    repeat until it gets to the desired position.
  */
  while (true)
  {
    rows_spent = 0;
    last_category = NULL;
    bottom_stop = 2;
    bottom_stop2 = 2;
    top_stop = 0;
    overshoot = false;
    screen_pos = 999;
    
    for (i = 0; i < b->list_h; i++)
    {
      item_i = b->start_row + i;
      
      /* If we're past the last item */
      if (item_i >= b->total_items)
      {
	overshoot = true;
	break;
      }
      
      /* If we're past the last row, but still have items to display. */
      if (rows_spent >= b->list_h)
	break;

      /*
	Check if the category of this menu item differs from the
	category of the last menu item (which will be NULL if this is
	the first item).
      */
      if (list[item_i]->category != NULL &&
	  mycmp(list[item_i]->category, last_category) == false)
      {
	last_category = list[item_i]->category;
	rows_spent++;
      }

      /* Again, if we're past the last row. */
      if (rows_spent >= b->list_h)
	break;

      /* If this is the second _item_, save its on-screen position. */
      if (i == 1)
	top_stop = rows_spent;

      /* Save the desired bottom position - this always drags 1 behind */
      if (i >= 0)
      {
	/*
	  If the list matches exactly what will fit on screen make the
	  bottom scroll point unreachable, to avoid triggering a
	  scroll on the last line when it isn't necessary.
	*/
	if (i == totitms - 1)
	{
	  bottom_stop = b->list_h + 3;
	  bottom_stop2 = b->list_h + 3;
	}
	else
	{
	  bottom_stop = bottom_stop2;
	  bottom_stop2 = rows_spent;
	}
      }
      
      /* Is this the current item? */
      if (item_i == b->cur_pos)
      {
	screen_pos = rows_spent;
      }
      
      rows_spent++;
    }

    /*
      If we haven't decided on a direction to scroll yet, do it
      now. We do this after recalculating screen_pos so we always have
      fresh numbers.
    */
    if (dir == 0)
    {
      if (b->cur_pos <= b->start_row)
      {
	b->start_row = MAX(0, b->cur_pos - (b->list_h - 2));
	dir = -1;
      }
      else if (screen_pos < top_stop)
	dir = -1;
      else if (!overshoot && screen_pos > bottom_stop)
	dir = +1;
      else
	return;
    }

    /* Scroll if needed */
    if (dir < 0 && screen_pos < bottom_stop)
    {
      b->start_row--;
      
      if (b->start_row <= 0)
      {
	b->start_row = 0;
	return;
      }
    }
    else if (dir > 0 && screen_pos > top_stop)
    {
      b->start_row++;
    }
    else
      return;
  }
  
  return;
} /* browser_scroll */
