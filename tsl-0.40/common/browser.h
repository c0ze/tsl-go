/*
  browser.h
*/

#ifndef _BROWSER_H_
#define _BROWSER_H_

#include "main.h"
#include "stuff.h"
#include "creature.h"
#include "menuitem.h"


#define MENU_USE       0
#define MENU_PICKUP    1
#define MENU_NOSELECT  2
#define MENU_FACETS    3
#define MENU_GENERIC   4
#define MENU_PICK      5
#define MENU_OPTIONS   6
#define MENU_ABILITIES 7
#define MENU_BESTIARY  8


#define BRCT_KEYS 5

struct browser_t
{
  int desc_h;
  int list_h;
  int start_row;
  int cur_pos;
  int total_items;
  blean_t have_gents;
  int keys[BRCT_KEYS];
};
typedef struct browser_t browser_t;


extern browser_t browser[10];



void init_browsers(void);

int browse(menu_item_t ** list, int totitms,
	   const int mode,
	   item_t * inspect_item, int * ret_action);
void browser_scroll(browser_t * b, menu_item_t ** list, int totitms);

void draw_browser(const int sep1, const int sep2);

#endif
