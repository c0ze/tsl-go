/*
  stacks.h

  By stack we mean a stack of items. Not really an ADT stack, since
  they're implemented as a single-linked list.
*/

#ifndef _STACKS_H_
#define _STACKS_H_

#include "item.h"

blean_t can_stack(const item_t * item_a, const item_t * item_b);
unsigned int count_items_forward(item_t * start);
unsigned int stack_size(const item_t * item);
item_t * split_stack(item_t * stack, const unsigned int new_stack_size);
item_t * get_item_from_stack(item_t * stack);
item_t * bottom_of_stack(item_t * stack);

#endif
