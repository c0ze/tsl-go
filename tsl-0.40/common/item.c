#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "main.h"
#include "stuff.h"
#include "item.h"
#include "inventory.h"
#include "creature.h"
#include "level.h"
#include "player.h"
#include "stacks.h"
#include "game.h"
#include "find.h"
#include "craft.h"

char * item_article_that[] =
{
  "that",
  "that",
  "those",
  "that",
  "that",
  "those",
  "that",
  "that",
  "those",
  "that"
};

char * item_article_it[] =
{
  "it",
  "it",
  "they",
  "it",
  "it",
  "they",
  "it",
  "it",
  "they",
  "it"
};

char * item_article_has[] =
{
  "has",
  "has",
  "have",
  "has",
  "has",
  "have",
  "has",
  "has",
  "have",
  "has"
};

char * item_article_is[] =
{
  "is",
  "is",
  "is",
  "is",
  "is",
  "are",
  "is",
  "is",
  "are",
  "is"
};

char * item_article_a[] =
{
  "a ",
  "an ",
  "a ",
  "a ",
  "some ", /* "some is" */
  "some ", /* "some are" */
  "",  /* none */
  "a ",
  "a ",
  "the "
};

char * item_article_of[] =
{
  "",
  "",
  "pair of ",
  "set of ",
  "",
  "",
  "",
  "pile of ",
  "",
  ""
};



/*
  Allocates an item_t structure. The structure returned will have its
  member variables set to neutral values (such that the game shouldn't
  crash if they are used - I hope).
*/
item_t * alloc_item()
{
  int i;
  item_t * local;

  local = malloc(sizeof(item_t));
  if (local == NULL) out_of_memory();
  mem_alloc.items++;

  local->id = known_none;

  local->item_number = 0;
  local->artifact = false;
  local->no_wish = false;

  local->gent = gent_undefined;

  local->letter = '%';

  local->y = 0;
  local->x = 0;

  local->ricochets = false;

  local->lifetime = 0;

  local->label = NULL;

  local->single_unid_name = NULL;
  local->single_id_name = NULL;
  local->plural_unid_name = NULL;
  local->plural_id_name = NULL;
  local->description = NULL;

  local->single_unid_article = article_i_a;
  local->single_id_article = article_i_a;
  local->plural_unid_article = article_i_a_n_x;
  local->plural_id_article = article_i_a_n_x;

  local->indestructible = false;
  local->edible = false;
  local->prohibit_stacking = false;
  local->auto_id = false;

  local->location = NULL;
  local->inventory = NULL;
  local->stack = NULL;
 
  local->prev_item = NULL;
  local->next_item = NULL;
  local->child = NULL;

  local->rechargeable = false;
  local->invoke_power = false;
  local->charges = 0;

  local->weight = 1;

  local->item_type = item_type_none;
  local->sort_type = item_type_none;

  for (i = 0; i < CUSTOM_FIELDS; i++)
    local->custom[i] = 0;

  for (i = 0; i < ATTRIBUTES; i++)
    local->attr_mod[i] = 0;

  return local;
} /* alloc_item */



/*
  Frees ITEM. This will also remove any stacked items *under* this
  one, but it won't touch the parent object of its stack.
*/
void del_item(item_t * item)
{
  if (item == NULL)
    return;

  detach_item(item);

  /* Remove any stacked items */
  while (item->child != NULL)
    del_item(item->child);

  if (item->single_unid_name != NULL)
  {
    mem_alloc.chars -= strlen(item->single_unid_name) + 1;
    free(item->single_unid_name);
    item->single_unid_name = NULL;
  }
  
  if (item->single_id_name != NULL)
  {
    mem_alloc.chars -= strlen(item->single_id_name) + 1;
    free(item->single_id_name);
    item->single_id_name = NULL;
  }

  if (item->plural_unid_name != NULL)
  {
    mem_alloc.chars -= strlen(item->plural_unid_name) + 1;
    free(item->plural_unid_name);
    item->plural_unid_name = NULL;
  }
  
  if (item->plural_id_name != NULL)
  {
    mem_alloc.chars -= strlen(item->plural_id_name) + 1;
    free(item->plural_id_name);
    item->plural_id_name = NULL;
  }

  if (item->description != NULL)
  {
    mem_alloc.chars -= strlen(item->description) + 1;
    free(item->description);
    item->description = NULL;
  }

  label_item(item, NULL);

  free(item);
  mem_alloc.items--;

  return;
} /* del_item */



/*
  Removes ITEM from any level it is on or creature it is carried by.
*/
item_t * detach_item(item_t * item)
{
  item_t * temp;
  
  if (item == NULL)
    return NULL;

  if (item->location != NULL)
  {
    /* Loop through the item list, look for the item... */
    for (temp = item->location->first_item; temp != NULL; temp = temp->next_item)
    {
      if (temp == item)
      {
	/* Found it! */
	/* Repoint next item's previous pointer to this ones previous. */
	if (item->next_item != NULL)
	  item->next_item->prev_item = item->prev_item;
	
	/* Repoint previous item's next pointer to this ones next. */
	if (item->prev_item != NULL)
	  item->prev_item->next_item = item->next_item;
	
	/* If this is the first item, we need to change the levels pointer as well.*/
	if (item == item->location->first_item)
	  item->location->first_item = item->next_item;
      } /* if item */
    } /* for */
  } /* if location */
  
  if (item->stack != NULL)
  {
    /* Loop through stack, look for the item... */
    for (temp = item->stack->child; temp != NULL; temp = temp->next_item)
    {
      if (temp == item)
      {
	/* Found it! */
	/* Repoint next item's previous pointer to this ones previous. */
	if (item->next_item != NULL)
	  item->next_item->prev_item = item->prev_item;
	
	/* Repoint previous item's next pointer to this ones next. */
	if (item->prev_item != NULL)
	  item->prev_item->next_item = item->next_item;
	
	/* If this is the first item, we need to change the stack heads child pointer as well.*/
	if (item == item->stack->child)
	  item->stack->child = item->next_item;
      } /* if item */
    } /* for */
  } /* if location */
  
  if (item->inventory != NULL)
  {
    /* Loop through the item list, look for the item... */
    for (temp = item->inventory->first_item; temp; temp = temp->next_item)
    {
      if (temp == item)
      {
	/* Har har, found it! */
	/* Repoint next item's previous pointer to this ones previous. */
	if (item->next_item != NULL)
	  item->next_item->prev_item = item->prev_item;
	
	/* Repoint previous item's next pointer to this ones next. */
	if (item->prev_item != NULL)
	  item->prev_item->next_item = item->next_item;
	
	/* If this is the first item in the list, we need to
	   change the creatures pointer as well.*/
	if (item == item->inventory->first_item)
	  item->inventory->first_item = item->next_item;
      } /* if item */
    } /* for */
  } /* if inventory */
  
  /* Since the item is now "nowhere", these should be NULLed. */
  item->next_item = NULL;
  item->prev_item = NULL;

  item->location = NULL;
  item->inventory = NULL;
  item->stack = NULL;

  item->equipped = false;
  
  return item;
} /* detach_item */



/* 
   Inserts ITEM on LEVEL. On failure, ITEM is returned, otherwise
   NULL. The caller is responsible for setting sane coordinates for the
   item.
*/
item_t * attach_item_to_level(level_t * level, item_t * item)
{
  if ((level == NULL) || (item == NULL))
    return item;

  /* Remove the item from wherever it is right now. */
  detach_item(item);
  
  /* Insert the new item at the first location in the linked list of
     items. The order of items doesn't matter to rooms, so we'll just
     use the fastest way possible. */
  item->prev_item = NULL; /* This will be the new *first* item */
  item->next_item = level->first_item; /* Point to the old 1st item as 2nd */
  level->first_item = item; /* Set the new item as 1st */
  
  /* Is there a second item? */
  if (item->next_item != NULL)
    item->next_item->prev_item = item; /* Link the 2nd back to the new first */

  /* Make sure the item knows where it is. */
  item->location = level;

  /* Make sure the item knows where it NOT is. */
  item->inventory = NULL;
  item->equipped = false;

  return NULL;
} /* attach_item_to_level */



/*
  Sets the coordinates of ITEM to {Y, X}. If the specified coordinates
  are outside the edge of the map, nothing happens. No check is
  performed whether the coordinates are walkable or not (e.g. it is
  perfectly legal to put it inside a wall or similar). If ITEM
  attached to a level and any identical item are present at the
  coordinates, they will stack.
*/
void place_item(item_t * item, const unsigned int y, const unsigned int x)
{
  item_t ** list;
  unsigned int i;
  unsigned int items;

  if (item == NULL)
    return;

  item->y = y;
  item->x = x;

  /* Check if there is anything else there... */
  items = find_items(item->location, y, x, &list);
  
  if (items > 0)
  {
    /* There is; loop through each item and see if we should stack them. */
    for (i = 0; i < items; i++)
    {
      if ((list[i] != item) && (can_stack(list[i], item) == true))
      {
	attach_item_to_stack(list[i], item);
	break;
      }
    }
    
    free(list);
    list = NULL;
  }
  
  return;
} /* set_item_coordinates */



/*
  Places ITEM at some random coordinates (usually a floor). The item
  must be attached to a level for this to work.
*/
void find_random_item_spot(item_t * item)
{
  int y;
  int x;

  if (item == NULL ||
      item->location == NULL)
  {
    return;
  }

  if (find_spot(item->location, &y, &x, find_3x3) == false)
  {
    if (find_spot(item->location, &y, &x, find_noitems) == false)
    {
      if (find_spot(item->location, &y, &x, find_unoccupied) == false)
      {
	y = 0;
	x = 0;
      }
    }
  }

  place_item(item, y, x);

  return;
} /* find_random_item_spot */



/*
  Returns ITEMs ATTR_INDEX modifier.
*/
signed int get_item_mod(const item_t * item, const attr_index_t attr_index)
{
  if (item == NULL ||
      attr_index < 0 ||
      attr_index >= ATTRIBUTES ||
      attr_info[attr_index] == NULL)
  {
    return 0;
  }

  return item->attr_mod[attr_index];
} /* get_item_mod */



/*
  Duplicates ORIGINAL. A new item_t will be returned, containing the
  data found in ORIGINAL. This is a deep copy, i.e. the name strings
  will be duplicated as well. Information about an items whereabouts,
  though, would not make sense to be duplicated, and will be set to
  neutral values. Note that this does NOT copy an entire stack, only
  the first item (the copy returned has its stack set to NULL)
*/
item_t * clone_item(const item_t * original)
{
  item_t * new_item;

  if (original == NULL)
    return NULL;

  new_item = alloc_item();

  /* Deallocate any name strings that are currently set for the *new* item. */
  if (new_item->single_unid_name != NULL)
  {
    mem_alloc.chars -= strlen(new_item->single_unid_name) + 1;
    free(new_item->single_unid_name);
    new_item->single_unid_name = NULL;
  }
  
  if (new_item->single_id_name != NULL)
  {
    mem_alloc.chars -= strlen(new_item->single_id_name) + 1;
    free(new_item->single_id_name);
    new_item->single_id_name = NULL;
  }

  if (new_item->plural_unid_name != NULL)
  {
    mem_alloc.chars -= strlen(new_item->plural_unid_name) + 1;
    free(new_item->plural_unid_name);
    new_item->plural_unid_name = NULL;
  }
  
  if (new_item->plural_id_name != NULL)
  {
    mem_alloc.chars -= strlen(new_item->plural_id_name) + 1;
    free(new_item->plural_id_name);
    new_item->plural_id_name = NULL;
  }

  if (new_item->description != NULL)
  {
    mem_alloc.chars -= strlen(new_item->description) + 1;
    free(new_item->description);
    new_item->description = NULL;
  }

  memcpy(new_item, original, sizeof(item_t));

  /* NULL these so set_item_*_name() won't free them below (they are still in use by the original item!) */
  new_item->plural_id_name = NULL;
  new_item->plural_unid_name = NULL;
  new_item->single_id_name = NULL;
  new_item->single_unid_name = NULL;
  new_item->description = NULL;

  set_item_single_name(new_item, original->single_unid_name, original->single_id_name);
  set_item_plural_name(new_item, original->plural_unid_name, original->plural_id_name);
  set_item_description(new_item, original->description);

  /*
    The clone will just be floating in the void. Someone better catch
    it before it drifts away and causes cosmic disturbances.
  */
  new_item->stack = NULL;
  new_item->location = NULL;
  new_item->inventory = NULL;
  new_item->prev_item = NULL;
  new_item->next_item = NULL;

  return new_item;
} /* clone_item */



unsigned int get_min_damage(const item_t * item)
{
  if (item == NULL)
    return 0;
  
  return item->custom[WEAPON_MIN_DAMAGE];
} /* get_min_damage */



unsigned int get_max_damage(const item_t * item)
{
  if (item == NULL)
    return 0;
  
  return item->custom[WEAPON_MAX_DAMAGE];
} /* get_max_damage */



/*
  Returns a random damage from WEAPON. If MISSILE in non-null, its
  minimum damage will be added as well. If CREATURE is non-null ...
*/
unsigned int random_damage(const creature_t * creature, const item_t * weapon, const item_t * missile)
{
  unsigned int ret;
  unsigned int min;
  unsigned int max;

  ret = 0;

  if (missile != NULL)
  {
    if (explosive(missile))
    {
      min = get_min_damage(missile);
      max = get_max_damage(missile);
      
      ret += MAX(0, min + tslrnd() % MAX(1, max - min + 1));
    }
    else
    {
      ret += get_min_damage(missile);
    }
  }

  if (weapon)
  {
    min = get_min_damage(weapon);
    max = get_max_damage(weapon);
    
    ret += MAX(0, min + tslrnd() % MAX(1, max - min + 1));
  }

/*  if (creature != NULL)
    ret += get_attr_current(creature, attr_damage_bonus);*/

  return ret;
} /* random_damage */



/*
  Returns the damage type of ITEM. If ITEM is NULL or isn't a melee
  weapon or ammo, "general" will be returned.
 */
damage_type_t get_damage_type(const item_t * item)
{
  if (item == NULL)
    return damage_general;

  /*
    We'll keep these as separate cases in case (no pun intended) the
    CUSTOM_* constants change.
  */
  if (item->item_type == item_type_m_weapon)
  {
    return item->custom[WEAPON_DAMAGE_TYPE];
  }
  else if (item->item_type == item_type_ammo)
  {
    return item->custom[CUSTOM_AMMO_DTYPE];
  }
  else
  {
    return damage_general;
  }
} /* get_damage_type */



unsigned int get_weight(const item_t * item)
{
  if (item == NULL)
    return 0;

  return item->weight;
} /* get_weight */



blean_t put_item_or_destroy(level_t * level, item_t * item,
			    const unsigned int y,
			    const unsigned int x)
{
  if (item == NULL)
    return false;

  if ((on_map(level, y, x) == false) ||
      (attach_item_to_level(level, item) != NULL))
  {
    del_item(item);
    return false;
  }

  find_nearest_item_spot(item, y, x);
  
  return true;
} /* put_item_or_destroy */



void find_nearest_item_spot(item_t * item,
			    const unsigned int intended_y,
			    const unsigned int intended_x)
{
  unsigned int radius;
  level_t * level;
  signed int y;
  signed int x;

  if (item == NULL)
  {
    return;
  }

  level = item->location;

  if (level == NULL)
  {
    return;
  }

  for (radius = 0; radius <= 5; radius++)
  {
    for (y = intended_y - radius; y <= intended_y + radius; y++)
    {
      for (x = intended_x - radius; x <= intended_x + radius; x++)
      {
	if (on_map(level, y, x) == false)
	  continue;

	if ((level->map[y][x] == tile_floor) &&
	    (find_items(level, y, x, NULL) == 0))
	{
	  place_item(item, y, x);
	  return;
	}
      }
    }
  }

  find_random_item_spot(item);

  return;
} /* find_nearest_item_spot */



/*
  Returns the maximum firing range of WEAPON, or zero if WEAPON doesn't have any.
*/
unsigned int get_weapon_range(const item_t * weapon)
{
  if ((weapon == NULL) ||
      (weapon->item_type != item_type_r_weapon))
  {
    return 0;
  }

  return weapon->custom[WEAPON_RANGE];
} /* get_weapon_range */



signed int weapon_seq(const item_t * item, const unsigned int pos)
{
  unsigned int npos;

  if (item == NULL)
    return 0;

  npos = pos + WEAPON_ATKSEQ;

  if (npos > CUSTOM_FIELDS)
    return 0;

  return item->custom[npos];
} /* weapon_seq */
