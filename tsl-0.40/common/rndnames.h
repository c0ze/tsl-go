/*
  rndnames.h - random appearances for wands and potions.
*/

#ifndef _RNDNAMES_H_
#define _RNDNAMES_H_

#include "item.h"

/*
  RFE: This could be improved to handle *any* item type, e.g. with
  functions like register_random_name(item_type, article, name).
*/

/*
  Potions
*/
#define POTION_APPEARANCES 20
extern gent_t potion_gent[];
extern char * potion_name[];
extern item_article_t potion_article[];
unsigned int potion_map[POTION_APPEARANCES];

char * get_potion_name(const unsigned int item);
item_article_t get_potion_article(const unsigned int item);
gent_t get_potion_gent(const unsigned int item);
signed int randomize_potion(const unsigned int item);

/*
  Wands
*/
#define WAND_APPEARANCES 14
extern char * wand_name[];
extern item_article_t wand_article[];
unsigned int wand_map[WAND_APPEARANCES];

char * get_wand_name(const unsigned int item);
item_article_t get_wand_article(const unsigned int item);
signed int randomize_wand(const unsigned int item);

/*
  Scrolls
*/
#define SCROLL_APPEARANCES 20
extern char * scroll_name[];
extern char * scroll_name_plural[];
extern item_article_t scroll_article[];
unsigned int scroll_map[WAND_APPEARANCES];

char * get_scroll_name(const unsigned int item);
char * get_scroll_name_plural(const unsigned int item);
item_article_t get_scroll_article(const unsigned int item);
signed int randomize_scroll(const unsigned int item);

/* Books */
char * get_book_name(const unsigned int item);

#endif
