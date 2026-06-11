/*
  dwiminv.h
*/

#ifndef _DWIMINV_H_
#define _DWIMINV_H_

#include "main.h"
#include "item.h"
#include "browser.h"

#define MENUACT_USE      1
#define MENUACT_DROP     2
#define MENUACT_LABEL    3
#define MENUACT_THROW    4
#define MENUACT_IDENTIFY 5
#define MENUACT_EQUIP    6
#define MENUACT_EAT      7
#define MENUACT_APPLY    8
#define MENUACT_RECHARGE 9

#define SUBMENU_SIZE 8

int dwim_inventory(creature_t * creature, blean_t (* criteria) (const item_t *), char * order);
int dwim_pickup(creature_t * creature);
item_t * dwim_select(creature_t * creature, blean_t (* criteria) (const item_t *), char * order);
int dwim_item(creature_t * who, item_t * what);
int drop_stack(creature_t * creature, item_t * item);
void browser_remove_item(browser_t * b, menu_item_t ** list, int it);

blean_t del_id_scroll(creature_t * owner);

int browser_item_submenu(creature_t * creature, item_t * item);

#endif
