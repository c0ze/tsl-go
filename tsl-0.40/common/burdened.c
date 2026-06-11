#include "burdened.h"
#include "message.h"
#include "player.h"
#include "inventory.h"
#include "stacks.h"



/*
  Prints a message if CREATURE has become unburdened by dropping
  WEIGHT. Put another way, WEIGHT is the straw that breaks creatures
  back and would put its total carried weight above its carrying
  capacity. This assumes that the item weighing WEIGHT has already
  been removed from the creatures inventory.
*/
void unburdened(const creature_t * creature, const unsigned int weight)
{
  unsigned int carried;
  unsigned int allow;
  
  if (creature == NULL ||
      is_player(creature) == false)
  {
    return;
  }

  carried = get_carried_weight(creature);
  allow = get_weight_allowance(creature);

  if (carried + weight > allow &&
      carried <= allow)
  {
    queue_msg("You are no longer burdened.");
  }
  
  return;
} /* unburdened */



/*
  Displays a message if CREATUREs current burden falls between current
  capacity and OLD_CAPACITY.
*/
void stagger(const creature_t * creature, const unsigned int old_capacity)
{
  unsigned int carried;
  unsigned int allow;
  
  if (creature == NULL ||
      is_player(creature) == false)
  {
    return;
  }

  carried = get_carried_weight(creature);
  allow = get_weight_allowance(creature);

  if (carried > allow &&
      old_capacity >= carried)
  {
    queue_msg("You stagger under your load!");
  }

  return;
} /* stagger */



/*
  Returns how much weight CREATURE is currently carrying.
*/
unsigned int get_carried_weight(const creature_t * creature)
{
  item_t * temp;
  unsigned int total;

  if (creature == NULL)
    return 0;

  total = 0;

  for (temp = creature->first_item; temp != NULL; temp = temp->next_item)
    total += stack_size(temp) * get_weight(temp);
  
  return total;
} /* get_carried_weight */



/*
  Returns how much CREATURE may carry.
*/
unsigned int get_weight_allowance(const creature_t * creature)
{
  if (creature == NULL)
    return 0;

  /* if (has_inventory(creature) == false)
     return 1;*/

  return attr_current(creature, attr_carrying_capacity);
} /* get_weight_allowance */



/*
  Returns true if CREATURE is carrying too much to be comfortable.
*/
blean_t is_burdened(const creature_t * creature)
{
  if (get_carried_weight(creature) > get_weight_allowance(creature))
    return true;
  else
    return false;
} /* is_burdened */
