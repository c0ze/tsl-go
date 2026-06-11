/*
  select.h - for selecting items from lists.
*/

#ifndef _SELECT_H_
#define _SELECT_H_

#include "main.h"
#include "item.h"

//#define INVENTORY_PAGE_SIZE 18

void sort_items(item_t ** list,
		const unsigned int items,
		const char * order,
		const blean_t reset_letters);

#endif
