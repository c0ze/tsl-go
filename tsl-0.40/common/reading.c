#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "main.h"
#include "stuff.h"
#include "reading.h"
#include "item.h"
#include "creature.h"
#include "game.h"
#include "message.h"
#include "ability.h"
#include "effect.h"
#include "monster.h"
#include "player.h"
#include "ui.h"
#include "stacks.h"
#include "burdened.h"
#include "effect.h"



/*
  Returns false if CREATURE can not read after printing a message
  why. Returns true and is silent if it can.
*/
blean_t cannot_read(const creature_t * creature)
{
  if (creature == NULL)
    return true;

  if (is_blinded(creature))
  {
    if (is_player(creature))
      queue_msg("You cannot see!");
    
    return true;
  }
  
  if (attr_current(creature, attr_p_read))
  {
    if (is_player(creature))
      queue_msg("You cannot read!");
    
    return true;
  }

  return false;
} /* cannot_read */



/*
  We don't care if scrolls have charges; we will just activate it once
  and then destroy it.
*/
blean_t read_scroll(creature_t * creature, item_t * item)
{
  unsigned int ability;
  item_t * temp;

  if (creature == NULL || item == NULL || cannot_read(creature))
    return false;

  /* Take the scroll out of the inventory */
  temp = detach_item(get_item_from_stack(item));
  
  ability = item->invoke_power;
    
  if ((ability >= ATTRIBUTES) ||
      (attr_info[ability] == NULL))
  {
    /*
      This should never happen, so we won't bother to put the scroll
      back where it belongs.
    */
    queue_msg("BUG: Unknown invoke_power for scroll.");
    del_item(temp);
    return false;
  }
  
  invoke_ability(creature, ability, item);
  
  /* We've read the scroll; destroy it */
  unburdened(creature, temp->weight);
  del_item(temp);
  
  return true;
} /* read_scroll */



/* */
blean_t read_book(creature_t * creature, item_t * item)
{
  unsigned int ability;
  char line[100];

  if (creature == NULL || item == NULL || cannot_read(creature))
    return false;
  
  ability = item->custom[BOOK_ABILITY];
  
  id_if_not_known(item);
  msgflush_wait();
  
  if (ability && attr_base(game->native, ability))
  {
    queue_msg("You already know this.");
    return false;
  }
  else if (maybe())
  {
    queue_msg("This book is difficult to understand!");
    msgflush_wait();
    bad_book(creature, item);
    return true;
  }
  else
  {
/*    if (maybe())
    {
      queue_msg("This book is difficult to understand!");
      msgflush_wait();
      }*/
    
    if (item->item_number == treasure_b_pharmacy)
    {
      make_number_known(treasure_elixir);
      make_number_known(treasure_p_healing);
      make_number_known(treasure_p_instant_healing);
      make_number_known(treasure_p_pain);
      make_number_known(treasure_p_energy);
      make_number_known(treasure_p_speed);
      make_number_known(treasure_p_poison);
      make_number_known(treasure_p_slowing);
      make_number_known(treasure_p_polymorph);
      make_number_known(treasure_p_levitation);
      make_number_known(treasure_p_sleep);
      make_number_known(treasure_p_yuck);
      make_number_known(treasure_p_blindness);
      
      queue_msg("You learn how to identify all potions.");
    }
/*    else if (item->item_number == treasure_b_camouflage)
    {
      queue_msg("You ");
      game->first_facet->attr_mod[attr_stealth] += 3;
      }*/
    else if (ability)
    {
      sprintf(line, "You learn %s.", attr_info[ability]->name);
      game->first_facet->attr_mod[ability] = 1;
      game->first_facet->attr_mod[attr_ep_max] += 1;
      game->first_facet->attr_mod[attr_magic] += 1;
      unmapped_abilities(NULL);
    }
    
    detach_item(item);
    unburdened(creature, item->weight);
    del_item(item);

    return true;
  }
  
} /* read_book */



void bad_book(creature_t * reader, item_t * book)
{
/*  item_t * item;*/
  creature_t * monster;

  if (reader == NULL || book == NULL)
    return;

  switch (roll(1, 3))
  {
    case 1:
      queue_msg("The book bites into your hand!");
/*      item = build_item(treasure_hungry_book);
      item->lifetime = MAGIC_WEAPON_LIFETIME;
      set_magic_weapon(reader, item);*/

      set_temp_weapon(reader, virtual_hungry_book, HUNGRY_BOOK_LIFETIME);
/*      detach_item(item);
      unburdened(reader, item->weight);
      del_item(book);*/
      break;

    case 2:
      if (is_blinded(reader) == false)
      {
	queue_msg("Darkness falls around you!");
	prolong_effect(reader, effect_blindness, DEFAULT_BLIND_TIME);
	set_creature_coordinates(reader, reader->y, reader->x);
	draw_level();
	msgflush_wait();
	break;
      }
      /* Else continue to the next case. */

    case 3:
      if (book->item_number == treasure_b_breathe_fire)
	monster = build_monster(monster_flame_spirit);
      else if (book->item_number == treasure_b_frost_ray)
	monster = build_monster(monster_frostling);
      else if (maybe() && maybe())
	monster = build_monster(monster_nameless_horror);
      else
	monster = build_monster(monster_imp);

      monster->lifetime = DEFAULT_SUMMON_LIFETIME;
      msg_one(monster, "escapes from the book!");

      if (attach_creature(reader->location, monster) == NULL)
	find_nearest_free_spot(monster, reader->y, reader->x);
      else
	del_creature(monster);

      break;
  }

  draw_level();
  msgflush_wait();
  clear_msgbar();
  
  return;
} /* bad_book */
