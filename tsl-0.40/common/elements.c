#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "main.h"
#include "stuff.h"
#include "rules.h"
#include "elements.h"
#include "ui.h"
#include "player.h"
#include "game.h"
#include "fov.h"
#include "stacks.h"
#include "losegame.h"
#include "burdened.h"
#include "altitude.h"
#include "combat.h"
#include "find.h"
#include "equip.h"



/*
  Exposes CREATURE (and all items carried) to the effects of lava.
  Returns true if this results in CREATURE getting killed. The
  creature must be on a lava tile (not floating).
*/
blean_t lava_bath(creature_t * creature)
{
  signed int dam;
  item_t * item;
  char line[100];

  if (creature == NULL ||
      creature->location == NULL ||
      get_tile(creature->location, creature->y, creature->x) != tile_lava ||
      is_floating(creature))
  {
    return false;
  }

  dam = sroll(LAVA_DAMAGE);
  dam = damage_armor(creature, dam, damage_fire); /* won't actually damage armor */
  damage(creature, dam);

  if (is_player(creature))
  {
    queue_msg("You get burned by lava!");
    
    if (killed(creature))
    {
      /* We won't return after this. */
      queue_msg("You die...");
      check_for_player_death("melted in a pool of lava");
    }
  }
  else if (killed(creature))
  {
    if (can_see_creature(game->player, creature))
      msg_one(creature, "disappears into the lava!");
    
    kill_creature(creature, false);
    return true;
  }
  else if (can_see_creature(game->player, creature))
    msg_one(creature, "gets burned by lava!");

  /*
    RFE: This could be in its own function, but we don't want it in
    expose_creature_to_fire() or it would happen for every fire (not
    only lava).
  */
  /* Sometimes destroy boots. */
  if (roll(1, 8) == 1 && is_player(creature))
  {
    item = get_equipped(creature, item_type_feet, 0);

    if (item && item->id != artifact_fire_drake_boots)
    {
      sprintf(line, "Your %s have been destroyed!",
	      identified(item) & known_name ? item->single_id_name : item->single_unid_name);
      queue_msg(line);
      del_item(item);
    }
  }
  
  expose_creature_to_fire(creature);

  return false;
} /* lava_bath */



/*
  Exposes CREATURE (and all items carried) to the effects of a
  forcefield.  Returns true if this results in CREATURE getting
  killed. The creature must be on a forcefield tile.
*/
blean_t forcefield(creature_t * creature)
{
  signed int dam;
  unsigned int dam_to_deal;

  if (creature == NULL ||
      creature->location == NULL ||
      get_tile(creature->location, creature->y, creature->x) != tile_forcefield)
  {
    return false;
  }

  dam_to_deal = sroll(FORCEFIELD_DAMAGE);

  /* Monsters take more damage! */
  if (is_player(creature) == false)
    dam_to_deal *= 2;

  dam_to_deal = damage_armor(creature, dam_to_deal, damage_general);
  dam = damage(creature, dam_to_deal);

  if (dam > 0)
  {
    if (is_player(creature))
    {
      queue_msg("You get burned by the forcefield!");
      
      if (killed(creature))
      {
	/* We won't return after this. */
	queue_msg("You die...");
	check_for_player_death("were disintegrated by a forcefield");
      }
    }
    else
    {
      if (killed(creature))
      {
	if (can_see_creature(game->player, creature))
	  msg_one(creature, "is disintegrated by the forcefield!");

	kill_creature(creature, false);
	return true;
      }
      else if (can_see_creature(game->player, creature))
	msg_one(creature, "gets burned by a forcefield!");
    }
  } /* dam */
  
  return false;
} /* forcefield */



/*
  Exposes ITEM to fire. This is _not_ for applying a torch, only
  unintentional environmental effects.
  
  Returns: true if the item was destroyed, false if it was unaffected.
*/
blean_t expose_item_to_fire(item_t * item, blean_t force_burn)
{
  creature_t * owner;
  char * name;
  char temp[80];
  unsigned int item_y;
  unsigned int item_x;
  unsigned int probability;
    
  if (item == NULL)
    return false;

  /* Is this something we can burn? */
  if (item->item_type != item_type_scroll &&
      item->item_type != item_type_book &&
      item->item_type != item_type_potion)
  {
    return false;
  }

  /*
    Check where the item is on the map (we need to determine if we
    can see what happens or not); if it's carried by a creature, use
    that creatures coordinates, otherwise the coordinates of the item
    itself.
  */
  owner = item->inventory;

  if (owner != NULL)
  {
    item_y = owner->y;
    item_x = owner->x;
  }
  else
  {
    item_y = item->y;
    item_x = item->x;
  }

  /* Set the probability of an item burning */
  probability = 5;

  if (force_burn || tslrnd() % 100 < probability)
  {
    /*
      Can we see what happens? If we're carrying the item ourselves,
      we always can (feel it).
    */
    /* RFE: Maybe this should display "in your pack" for items in the inventory. */
    if (can_see(game->player, item_y, item_x) ||
	(item->inventory == game->player))
    {
      name = get_item_name(item);
      
      switch (item->item_type)
      {
	case item_type_scroll:
	case item_type_book:
	  sprintf(temp, "%s catches fire!", name);
	  break;
	  
	case item_type_potion:
	  sprintf(temp, "%s boils and explodes!", name);
	  break;

	default:
	  sprintf(temp, "%s burns!", name);
	  break;
      }
      
      upperfirst(temp);

      free(name);
      name = NULL;
      
      queue_msg(temp);
    }
    
    /*
      The item was destroyed. It's a good thing we saved ->next_item
      in "skip" back in expose_creature_to_fire()! (if that was where
      we came from...)
    */

    detach_item(item);
    unburdened(owner, item->weight);
    del_item(item);
    
    return true;
  }
  
  return false;
} /* expose_item_to_fire */



/*
  Exposes all creatures and loose items at {Y, X} on LEVEL to fire.
  Returns: Same as expose_item_to_fire()
  See also: expose_item_to_fire()
*/
blean_t expose_tile_to_fire(level_t * level,
			    const unsigned int y, const unsigned int x)
{
  unsigned int items;
  item_t ** item_p;
  unsigned int i;
  blean_t message;

  message = false;

  if ((level == NULL)
      || (y >= level->size_y)
      || (x >= level->size_x))
  {
    return message;
  }

  /* Expose whatever creature is standing there */
  if (expose_creature_to_fire(find_creature(level, y, x)) == true)
    message = true;

  /* Find all items lying here and expose those as well */
  items = find_items(level, y, x, &item_p);

  for (i = 0; i < items; i++)
  {
    if (expose_item_to_fire(item_p[i], false) == true)
      message = true;
  }

  free(item_p);

  return message;
} /* expose_tile_to_fire */



/*
  Exposes all creatures and loose items at {Y, X} on LEVEL to cold.
  Returns: Same as expose_item_to_fire()
  See also: expose_item_to_fire()
*/
blean_t expose_tile_to_cold(level_t * level,
			    const unsigned int y, const unsigned int x)
{
  blean_t message;
  tile_t tile;

  message = false;

  if (on_map(level, y, x) == false)
    return message;

  tile = get_tile(level, y, x);

  /*
    If it's a lava square, the lava will turn into solid floor.
  */
  if (tile == tile_lava)
  {
    level->map[y][x] = tile_floor;
    
    if (can_see(game->player, y, x))
    {
      queue_msg("Some lava hardens!");
      message = true;
    }
  }

  /*
    Water shouldn't freeze if someone is in it.
  */
  if (tile == tile_water && find_creature(level, y, x) == NULL)
  {
    level->map[y][x] = tile_ice;
    
    if (can_see(game->player, y, x))
    {
      queue_msg("Some water freezes!");
      message = true;
    }
  }

  return message;
} /* expose_tile_to_cold */



/*
  Exposes CREATURE (and all items it's carrying) to fire.
  Returns: Same as expose_item_to_fire()
  See also: expose_item_to_fire()
 */
blean_t expose_creature_to_fire(creature_t * creature)
{
  item_t * item;
  item_t * skip;
  blean_t message;

  message = false;

  if (creature == NULL)
    return message;

  /*
    Loop through the creatures items and expose each of them. First
    save the next item in the inventory in "skip" - if "item" is
    deleted, we won't be able to find the next one!
  */
  item = creature->first_item;

  while (item != NULL)
  {
    skip = item->next_item;

    if (expose_item_to_fire(get_item_from_stack(item), false) == true)
      message = true;

    item = skip;
  }

  return message;
} /* expose_creature_to_fire */



blean_t hazard(creature_t * creature)
{
  tile_t tile;

  if (creature == NULL)
    return false;

  tile = get_tile(creature->location, creature->y, creature->x);
  
  if (tile == tile_lava)
    return lava_bath(creature);
  else if (tile == tile_forcefield)
    return forcefield(creature);

  return false;
} /* hazard */
