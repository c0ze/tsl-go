#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "main.h"
#include "stuff.h"
#include "stacks.h"



/*
  Returns true if ITEM_A and ITEM_B are identical enough to be
  allowed to stack.
*/
blean_t can_stack(const item_t * item_a, const item_t * item_b)
{
  if (item_a == NULL ||
      item_b == NULL ||
      item_a == item_b)
    return false;

  /* Most important, the two items must be of the same kind. */
  if (item_a->item_number != item_b->item_number)
    return false;

  if ((item_a->label != NULL) || (item_b->label != NULL))
    if (mycmp(item_a->label, item_b->label) == false)
      return false;
  
  /*
    Since they're the same item number, the same properties should
    apply to them, so we just have to check one of them here on.
  */

  /*
    Most things don't stack.
  */
  if (item_a->item_type == item_type_body ||
      item_a->item_type == item_type_feet ||
      item_a->item_type == item_type_head ||
      item_a->item_type == item_type_cloak ||
      item_a->item_type == item_type_m_weapon ||
      item_a->item_type == item_type_r_weapon ||
      item_a->item_type == item_type_wand ||
      item_a->item_type == item_type_book)
  {
    return false;
  }

  /*
    These will almost always differ in remaining burn time. We could
    have some kind of "auto-equip next item in stack when the current
    goes out", but I wish to retain the suspense (?) of the room going
    black.
  */
  if (item_a->item_type == item_type_light)
  {
    return false;
  }

  /*
    Sometimes we just don't *want* items to stack and have set this
    flag during item creation!
  */
  if (item_a->prohibit_stacking == true)
  {
    return false;
  }

  /*
    We won't stack anything with an invoked power, unless it's a scroll.
  */
  if ((item_a->invoke_power != 0) &&
      (item_a->item_type != item_type_scroll))
  {
    return false;
  }

  /* We've found no reason *not* to allow stacking... */
  return true;
} /* can_stack */



/*
  Starting at START, this will count (and return) the number of items
  found in the "next" direction of the linked list (including START
  itself) until a NULL is reached.
*/
unsigned int count_items_forward(item_t * start)
{
  unsigned int count;
  item_t * temp;

  if (start == NULL)
    return 0;

  count = 0;

  for (temp = start; temp != NULL; temp = temp->next_item)
    count++;

  return count;
} /* count_items_forward */



/*
  Returns the total number if items in the stack ITEM; that is, the
  "parent" item and all "child" items.
*/
unsigned int stack_size(const item_t * item)
{
  if (item == NULL)
    return 0;
  else
    return (1 + count_items_forward(item->child));
} /* stack_size */



/*
  Attaches NEW_CHILD to the stack PARENT. Returns NEW_CHILD on
  failure, otherwise NULL. No check is done whether NEW_CHILD *may* be
  stacked with PARENT (see identical_items()).
*/
item_t * attach_item_to_stack(item_t * parent, item_t * new_child)
{
  if ((parent == NULL) || (new_child == NULL))
    return new_child;

  /*
    This will reverse the stack order, but it doesn't matter.
  */
  while (new_child->child)
    attach_item_to_stack(parent, new_child->child);

  /* Remove the item from wherever it is right now. */
  detach_item(new_child);
  
  /*
    Insert the new item at the first location in the linked list of
    items. The order of items doesn't matter to stacks, so we'll just
    use the fastest way possible.
  */
  new_child->prev_item = NULL; /* This will be the new *first* item */
  new_child->next_item = parent->child; /* Point to the old 1st item as 2nd */
  parent->child = new_child; /* Set the new item as 1st */
  
  /* Is there a second item? */
  if (new_child->next_item != NULL)
    new_child->next_item->prev_item = new_child; /* Link the 2nd back to the new first */

  /* Make sure the item knows where it is. */
  new_child->stack = parent;

  /* Make sure the item knows where it NOT is. */
  /* TODO: These can probably be removed (3 of them); this is already done by detach_item... */
  new_child->inventory = NULL;
  new_child->location = NULL;
  new_child->equipped = false;

  return NULL;
} /* attach_item_to_stack */



/*
  Returns an item from STACK. Stacked item will be chosen first, then,
  when only the parent item remains, the parent item.
*/
item_t * get_item_from_stack(item_t * stack)
{
  if (stack == NULL)
    return NULL;

  if (stack->child != NULL)
    return stack->child;
  else
    return stack;
} /* get_item_from_stack */



/*
  Returns the last item in STACK, or STACK itself if it's a single
  item. The item returned will not have any other items stacked under
  it.
*/
item_t * bottom_of_stack(item_t * stack)
{
  item_t * temp;

  if (stack == NULL)
    return NULL;

  if (stack->stack == NULL)
    return stack;

  temp = stack;

  while (temp->next_item != NULL)
    temp = temp->next_item;

  return temp;
} /* bottom_of_stack */



/*
  Returns a pointer to a new item stack, which consists of
  NEW_STACK_SIZE elements from the already existing stack STACK.
*/
item_t * split_stack(item_t * stack, const unsigned int new_stack_size)
{
  item_t * new_stack;
  unsigned int i;

  if (stack == NULL)
    return NULL;

  /* We just won't do this */
  if (new_stack_size >= stack_size(stack))
    return NULL;

  /* Pick items off STACK and put them on the new stack */
  new_stack = detach_item(get_item_from_stack(stack));

  for (i = 0; i < new_stack_size - 1; i++)
    attach_item_to_stack(new_stack, get_item_from_stack(stack));

  return new_stack;
} /* split_stack */
