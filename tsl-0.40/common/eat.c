#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <ctype.h>

#include "main.h"
#include "stuff.h"
#include "eat.h"
#include "item.h"
#include "inventory.h"
#include "creature.h"
#include "input.h"
#include "ui.h"
#include "stacks.h"
#include "equip.h"
#include "poison.h"
#include "losegame.h"
#include "burdened.h"


/*
  Makes CREATURE eat ITEM. ITEM will be destroyed. Returns true if the
  item was eaten and the action consumed a turn. Returns false if it
  just couldn't be eaten for some reason.
*/
/* RFE: Make this general for any creature. Make enemies use it. */
blean_t eat(creature_t * creature, item_t * item)
{
  char line[100];
  char name[50];
  char * temp_name;

  if (creature == NULL || item == NULL)
    return false;

  if (attr_current(creature, attr_p_eat))
  {
    queue_msg(MSG_UNABLE_TO_EAT);
    return false;
  }

  if (stack_size(item) > 1)
    item = split_stack(item, 1);

  temp_name = get_item_name(item);
  strcpy(name, temp_name);
  free(temp_name);
  temp_name = NULL;

  try_to_unequip(item, false);

  if (item->gent == gent_sword)
  {
    EAT_MSG;
    queue_msg("You cut yourself open.");
    del_item(item);
    set_attr(creature, attr_killed, 1);
    sprintf(line, "tried to swallow %s", name);
    check_for_player_death(line);
  }
  else if (item->item_type == item_type_potion)
  {
    EAT_MSG;
    queue_msg("Let's hope it emerges in one piece.");
  }
  else if (item->item_number == treasure_s_buckshot)
  {
    EAT_MSG;
    queue_msg("You die of lead poisoning.");
    del_item(item);
    set_attr(creature, attr_killed, 1);
    sprintf(line, "tried to swallow %s", name);
    check_for_player_death(line);
  }
  else if (item->item_number == treasure_seaweed_cloak)
  {
    EAT_MSG;
    id_if_not_known(item);
    queue_msg("You make some excellent nori.");
    heal(creature, 1);
  }	
  else if (item->item_type == item_type_scroll ||
	   item->item_type == item_type_feet)
  { 
    EAT_MSG;
    if (item->item_type == item_type_scroll)
      queue_msg("It is a little dry."); /* oddmunds insists this was his idea */
    else if (item->item_type == item_type_feet)
      queue_msg("They are... chewy.");
      
    if (maybe())
    {
      queue_msg("You choke!");

      if (attr_current(creature, attr_nonbreathing))
      {
	queue_msg("Fortunately, you don't need to breathe.");
	msgflush_wait();
	queue_msg("Finally, you get all of it down.");
      }
      else
      {
	del_item(item);
	set_attr(creature, attr_killed, 1);
	sprintf(line, "choked on %s", name);
	check_for_player_death(line);
      }
    }

    heal(creature, 1);
  }
  else if (item->item_type == item_type_food)
  {
    EAT_MSG;
    
    switch (item->item_number)
    {
      case treasure_ogre_corpse:
      case treasure_corpse:
      case treasure_carcass:
      case treasure_decapitated_head:
      case treasure_severed_hand:
	heal(creature, 1);
	break;
	
      case treasure_chickpeas:
	queue_msg("They are a little dry.");
	heal(creature, 1);
	break;
	
      case treasure_bread:
      case treasure_meat:
      case treasure_fish:
      case treasure_cheese:
      case treasure_falafel:
      case treasure_sausage:
	heal(creature, 3);
	break;
	
      case treasure_r_mushroom:
	heal(creature, 4);
	
	if (poison_creature(creature, DEFAULT_POISON_TIME))
	  queue_msg("You feel very ill.");
	break;
    }
  }
  else
  {
    queue_msg("You can't eat that!");
    return false;
  }

  if (item->inventory == creature)
  {
    detach_item(item);
    unburdened(creature, item->weight);
  }

  del_item(item);

  return true;
} /* eat */
