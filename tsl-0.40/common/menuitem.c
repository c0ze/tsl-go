#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <ctype.h>

#include "stuff.h"
#include "menuitem.h"
#include "input.h"
#include "options.h"



/*
  Allocates an empty menu_item_t.
*/
menu_item_t * alloc_menu_item()
{
  menu_item_t * ret;
  
  ret = malloc(sizeof(menu_item_t));
  
  if (ret == NULL)
    out_of_memory();
  
  ret->category = NULL;
  ret->label = NULL;
  ret->item = NULL;
  ret->explanation = NULL;
  ret->inspect = NULL;
  ret->letter = '?';
  ret->gent = gent_undefined;

  return ret;
} /* menu_item_t */



/*
  Frees M. Strings attached to the menu_item_t are freed, but any
  item_t is left untouched.
*/
void del_menu_item(menu_item_t * m)
{
  if (m == NULL)
    return;

  free(m->category);
  free(m->explanation);
  free(m->label);
  free(m->inspect);
  free(m);
  
  return;
} /* del_menu_item */



/*
  Frees TOTAL_ITEMS of LIST.
*/
void del_menu(menu_item_t ** list, int total_items)
{
  int i;

  if (list == NULL)
    return;

  for (i = 0; i < total_items; i++)
    del_menu_item(list[i]);

  free(list);

  return;
} /* del_menu */



/*
  Converts TOTAL_ITEMS of FIRST_LIST to menu_item_t and returns a pointer to the array.
*/
menu_item_t ** make_item_menu(item_t ** first_list, int total_items)
{
  menu_item_t ** ret;
  int i;
  
  if (first_list == NULL)
    return NULL;
  
  ret = malloc(sizeof(menu_item_t) * total_items);
  
  if (ret == NULL)
    out_of_memory();
  
  for (i = 0; i < total_items; i++)
  {
    ret[i] = item_to_menu_item(first_list[i]);
    menu_item_add_explanation(ret[i]);
    ret[i]->act = i;
  }
  
  return ret;
} /* make_item_menu */



/*
  Returns a menu_item_t corresponding to ITEM.
*/
menu_item_t * item_to_menu_item(item_t * item)
{
  menu_item_t * ret;
  
  if (item == NULL)
    return NULL;
  
  ret = alloc_menu_item();
  ret->item = item;
  ret->letter = item->letter;
  ret->gent = item->gent;
  ret->label = get_inv_item_name(item);
  
/*  if (options.item_categories)*/
/*  {
    switch(item->item_type)
    {
      case item_type_body:     ret->category = mydup("armor"); break;
      case item_type_feet:     ret->category = mydup("boots"); break;
      case item_type_cloak:    ret->category = mydup("cloaks"); break;
      case item_type_head:     ret->category = mydup("headgear"); break;
      case item_type_m_weapon: ret->category = mydup("weapons/melee)"); break;
      case item_type_tool:     ret->category = mydup("tools"); break;
      case item_type_light:    ret->category = mydup("light"); break;
      case item_type_wand:     ret->category = mydup("wands"); break;
      case item_type_scroll:   ret->category = mydup("scrolls"); break;
      case item_type_book:     ret->category = mydup("books"); break;
      case item_type_potion:   ret->category = mydup("potions"); break;
      case item_type_r_weapon: ret->category = mydup("weapons/ranged"); break;
      case item_type_ammo:     ret->category = mydup("ammo"); break;
      case item_type_misc:     ret->category = mydup("misc"); break;
      case item_type_food:     ret->category = mydup("food"); break;
      default:                 ret->category = NULL; break;
    }
    }*/
    
  return ret;
} /* item_to_menu_item */



/*
  Adds an explanation to M of the default action (usually "Enter to ...").
*/
void menu_item_add_explanation(menu_item_t * m)
{
  char tmp[50];

  if (m == NULL || m->item == NULL)
    return;

  key_help(tmp, false, action_select);

  free(m->explanation);

  strcat(tmp, " to ");
  strcat(tmp, get_default_verb(m->item));
  m->explanation = mydup(tmp);
  
  return;
} /* menu_item_add_explanation */



/*
  Adds an explanation to M how to pick up.
*/
void menu_item_explain_pickup(menu_item_t * m)
{
  char key1[10];
  char key2[10];
  char tmp[100];

  if (m == NULL || m->item == NULL)
    return;

  free(m->explanation);

  key_help(key1, false, action_select);
  key_help(key2, false, action_pickup);

  sprintf(tmp, "%s or %s to pick up", key1, key2);
  m->explanation = mydup(tmp);
  
  return;
} /* menu_item_explain_pickup */


