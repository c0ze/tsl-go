/*
  MENUITEM.h
*/

#ifndef _MENUITEM_H_
#define _MENUITEM_H_

#include "main.h"
#include "item.h"


struct menu_item_t
{
  char * category;
  char * label;
  char * explanation;
  char * inspect;
  char letter;
  gent_t gent;
  int act;
  item_t * item;
};
typedef struct menu_item_t menu_item_t;


/* Setting up and freeing a menu */
menu_item_t * alloc_menu_item(void);
void del_menu_item(menu_item_t * m);
void del_menu(menu_item_t ** list, int total_items);

/* For converting item_t to menu_item_t */
menu_item_t ** make_item_menu(item_t ** first_list, int total_items);
menu_item_t * item_to_menu_item(item_t * item);
void menu_item_add_explanation(menu_item_t * m);
void menu_item_explain_pickup(menu_item_t * m);

#endif
