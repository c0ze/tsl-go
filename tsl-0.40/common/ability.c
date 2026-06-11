#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "ability.h"
#include "ui.h"
#include "game.h"
#include "browser.h"



/*
  Returns the number of abilities available to CREATURE (any attribute
  that has an invoke function). This does not take into account if the
  creature can _use_ it (has enough EP, etc).
*/
int count_abilities(const creature_t * creature)
{
  int i;
  int c;

  if (creature == NULL)
    return 0;

  c = 0;
  
  for (i = 0; i < ATTRIBUTES; i++)
  {
    if (attr_info[i] &&
	attr_info[i]->invoke &&
	attr_current(creature, i))
    {
      c++;
    }
  }

  return c;
} /* count_abilities */



/*
  Activates ability SEL for USER. If sel is -1 display a prompt and
  let the player select an ability first.
*/
blean_t ability_menu(creature_t * user, signed int sel)
{
  unsigned int cost;

  if (user == NULL)
    return false;

  if (count_abilities(user) == 0)
  {
    queue_msg("You have no abilities.");
    return false;
  }
  
  if (sel == -1)
  {
    queue_msg("Use which ability?");
    sel = select_ability(user);
  }
  
  if (sel == -1)
  {
    clear_msgbar();
    return false; /* Cancelled */
  }

  cost = get_ability_cost(user, sel);
  
  if (cost > attr_current(user, attr_ep_current))
  {
    queue_msg("You don't have enough energy!");
    return false;
  }
  
  if (invoke_ability(user, sel, NULL))
  {
    return true;
  }

  return false;
} /* ability_menu */



/*
  Lists all abilities USER has at its disposal and lets the player
  choose one of them.
*/
int select_ability(creature_t * user)
{
  menu_item_t ** menu_list;
  int total_items;
  int selection;
  int ret;
  int i;
  int k;
  char line[100];  
  char temp[1000];
  char select_key[20];

  if (user == NULL)
    return 0;

  key_help(select_key, false, action_select);

  total_items = count_abilities(user);

  if (total_items == 0)
  {
    queue_msg("You have no abilities.");
    return -1;
  }

  menu_list = malloc(sizeof(menu_item_t *) * total_items);

  if (menu_list == NULL)
    out_of_memory();

  k = 0;

  for (i = 0; i < ATTRIBUTES; i++)
  {
    if (attr_info[i] &&
	attr_current(user, i) &&
	attr_info[i]->key != ' ')
    {
      menu_list[k] = alloc_menu_item();

      sprintf(line, "%-25s (cost %2d)",
	      attr_info[i]->name,
	      get_ability_cost(user, i));

      menu_list[k]->label = mydup(line);
      menu_list[k]->letter = attr_info[i]->key;
      menu_list[k]->act = i;

      explain_ability(temp, i);
      menu_list[k]->inspect = mydup(temp);

      k++;
    }
  }
  
  selection = browse(menu_list, total_items, MENU_ABILITIES, NULL, NULL);

  if (selection == -1)
    ret = selection;
  else
    ret = menu_list[selection]->act;

//cleanup:
  del_menu(menu_list, total_items);
  
  return ret;
} /* select_ability */



/*
  Lists the players ability shortcut slots and lets the player choose one of them.
*/
int select_ability_slot()
{
  menu_item_t ** menu_list;
  int total_items;
  int selection;
  int i;
  int t;
  char line[100];
  char select_key[20];
  char label[10];

  key_help(select_key, false, action_select);

  total_items = 10;

  menu_list = malloc(sizeof(menu_item_t *) * total_items);

  if (menu_list == NULL)
    out_of_memory();

  for (i = 0; i < 10; i++)
  {
    menu_list[i] = alloc_menu_item();
    
    key_help(label, false, action_ability_1 + i);
    
    t = game->ability_slot[i];

    sprintf(line, "[%s] ", label);

    if (t != -1)
      strcat(line, attr_info[t]->name);

    menu_list[i]->label = mydup(line);
    menu_list[i]->letter = label[0];
  }
  
  selection = browse(menu_list, total_items, MENU_ABILITIES, NULL, NULL);

  del_menu(menu_list, total_items);

  return selection;
} /* select_ability_slot */



/*
  Lets the player configure what abilities should be accessible
  through the 1-0 shortcuts. If ABILITY is provided, this ability will
  be mapped to the first free shortcut. If ABILITY is -1, the player
  will be prompted what they want to map.
*/
void map_to_shortcut(signed int ability)
{
  char line[100];
  char label[6];
  unsigned int new_slot;
  signed int i;

  if (count_abilities(game->player) == 0)
  {
    queue_msg("You have no abilities.");
    return;
  }
  
  /*
    If an argument was supplied, just auto-assign that
    ability to the first available slot.
  */
  if (ability > -1)
  {
    for (i = 0; i < 10; i++)
    {
      if (game->ability_slot[i] == -1)
      {
	game->ability_slot[i] = ability;

	key_help(label, true, action_ability_1 + i);
	sprintf(line, "%s assigned to %s.", attr_info[ability]->name, label);
	queue_msg(line);

	return;
      }
    }
  }

  /* No argument was supplied. Prompt the player for which ability to assign. */

  queue_msg("Which ability would you like to assign?");
  ability = select_ability(game->player);

  if (ability == -1)
  {
    clear_msgbar();
    return;
  }

  sprintf(line, "Assign %s to which slot?", attr_info[ability]->name);
  queue_msg(line);
  msgflush_nowait();

  new_slot = select_ability_slot();

  if (new_slot != -1)
    game->ability_slot[new_slot] = ability;

  clear_msgbar();

  return;
} /* map_ability */



/*
  Returns the EP cost for USER to invoke ABILITY. If ABILITY is
  invalid, or USER doesn't have it, 0 is returned, otherwise >= 1.
*/
int get_ability_cost(const creature_t * user, const attr_index_t ability)
{
  int r;

  if ((ability < 0) || (ability >= ATTRIBUTES))
    return 0;

  if (attr_info[ability] == NULL)
    return 0;

  r = attr_info[ability]->cost;

  return r;
} /* get_ability_cost */



/*
  General interface for abilities. CASTER is the creature using
  ABILITY. SOURCE is the item it is invoked from, if any.
 */
blean_t invoke_ability(creature_t * user, const attr_index_t ability, item_t * source)
{
  blean_t res;
  unsigned int cost;

  if ((ability <= 0) ||
      (ability >= ATTRIBUTES) ||
      (attr_info[ability] == NULL))
  {
    return false;
  }

  cost = get_ability_cost(user, ability);

  if (source == NULL && cost > attr_base(user, attr_ep_current))
  {
    return false;
  }

  if (attr_info[ability]->invoke == NULL)
  {
    queue_msg("BUG: Undefined behaviour for ability!");
    return false;
  }

  res = attr_info[ability]->invoke(user, source, 0);
  
  if (source == NULL && res == true)
  {
    spend_ep(user, cost);
  }
     
/*      if (get_power_reagent(power) != -1)
	del_item(get_item_from_stack(find_item_in_inventory(caster, get_power_reagent(power))));
*/

  return res;
} /* invoke_ability */



/*
  Makes USER use ABILITY if available. The creature must possess the
  ability and have enough EP to use it. If the power could be utilised
  the EP of the creature will be decreased and true returned,
  otherwise false is returned.
*/
blean_t try_to_invoke_ability(creature_t * user, const attr_index_t ability)
{
  if (attr_current(user, ability) > 0)
    return invoke_ability(user, ability, NULL);
  
  return false;
} /* try_to_invoke_ability */



void unmapped_abilities(item_t * item)
{
  unsigned int i;
  unsigned int j;
  blean_t mapped;
  blean_t anything_mapped;
  char line[100];
  char label[6];
  
  anything_mapped = false;

  for (i = 0; i < 10; i++)
  {
    if (game->ability_slot[i] != -1 &&
	attr_current(game->player, game->ability_slot[i]) == 0)
    {
      game->ability_slot[i] = -1;
    }
  }

  for (i = 0; i < ATTRIBUTES; i++)
  {
    if ((attr_info[i] != NULL) &&
	(attr_info[i]->key != ' ') &&
	attr_current(game->player, i) >= 1)
    {
      mapped = false;

      for (j = 0; j < 10; j++)
      {
	if (game->ability_slot[j] == i)
	{
	  mapped = true;
	  break;
	}
      }
	
      if (!mapped)
      {
	map_to_shortcut(i);
	anything_mapped = true;
      }
    }
  }

  if (anything_mapped)
  {
    key_help(label, true, action_ability_config);
    sprintf(line, "(%s to change.)", label);
    queue_msg(line);
  }

  return;
} /* unmapped_abilities */



/*
  Builds a list of all abilities mapped to shortcuts and stores it in
  BUF (which must be large enough, etc).
*/
void list_shortcuts(char * buf, const blean_t all)
{
  unsigned int i;
  unsigned int ability_index;
  blean_t show;
  char line[100];
  char label[6];

  if (buf == NULL)
    return;

  strcpy(buf, "");

  for (i = 0; i < 10; i++)
  {
    ability_index = game->ability_slot[i];

    if ((ability_index != -1) &&
	(ability_index < ATTRIBUTES) &&
	(attr_info[ability_index] != NULL))
      show = true;
    else
      show = false;

    if (show || all)
    {
      key_help(label, false, action_ability_1 + i);
      sprintf(line, "[%s] ", label);
      strcat(buf, line);
    }
    
    if (show)
    {
      if (attr_current(game->player, ability_index) == 0)
	sprintf(line, "(%s)", attr_info[ability_index]->name);
      else
	sprintf(line, "%s", attr_info[ability_index]->name);

      strcat(buf, line);
    }

//    if (show || all)
//      {
      sprintf(line, "\n");
      strcat(buf, line);
//    }
  }

  return;
} /* list_shortcuts */



/*
  Explains what ABILITY does. Stores a string at DEST, which must of
  course be large enough, etc. 
*/
void explain_ability(char * dest, const unsigned int ability)
{
  if (dest == NULL)
    return;

  strcpy(dest, "");

  if (ability == attr_m_blink)
    strcpy(dest, "Instantly teleports to a random location on the level.");
  else if (ability == attr_m_first_aid)
    strcpy(dest, "Removes Wounded status and gain 1-3 Health.");
  else if (ability == attr_m_force_bolt)
    sprintf(dest, "Shoots a projectile in a chosen direction. The bolt will bounce against walls and do %d-%d damage to a creature hit. It has a range of %d tiles.", FORCE_BOLT_MIN_DAMAGE, FORCE_BOLT_MAX_DAMAGE, FORCE_BOLT_RANGE);
  else if (ability == attr_m_push)
    strcpy(dest, "Pushes something in a chosen direction.");
  else if (ability == attr_m_phase)
    strcpy(dest, "Teleports two tiles in a chosen direction.");
  else if (ability == attr_a_interface)
    strcpy(dest, "Connects to an electrical device.");
  else if (ability == attr_m_deathspell)
    strcpy(dest, "Has a 50/50 chance of killing either the caster or a target within touch range.");
  else if (ability == attr_m_destroy_trap)
    strcpy(dest, "Disables an adjacent trap.");
  else if (ability == attr_m_summon_familiar)
    strcpy(dest, "Calls forth a small creature that will follow and help the caster.");
  else if (ability == attr_m_noxious_breath)
    sprintf(dest, "Exhales a cone of toxic fumes. Anyone that inhales it (does not affect things that do not breathe) will take %s damage and be poisoned for %d turns.", NOXIOUS_BREATH_DAMAGE, NOXIOUS_BREATH_POISON);
  else if (ability == attr_m_breathe_fire)
    sprintf(dest, "Exhales a cone of flame that does %s damage to anyone caught in it.", BREATHE_FIRE_DAMAGE);
  else if (ability == attr_m_frost_ray)
    sprintf(dest, "Shoots a ray of intense cold in a chosen direction. The ray will freeze water, harden lava and do %d-%d cold damage to creatures. Its range is %d tiles.", FROST_RAY_MIN_DAMAGE, FROST_RAY_MAX_DAMAGE, FROST_RAY_RANGE);
/*  else if (ability == attr_m_)
    strcpy(dest, "");
  else if (ability == attr_m_)
  strcpy(dest, "");*/

  return;
} /* explain_ability */
