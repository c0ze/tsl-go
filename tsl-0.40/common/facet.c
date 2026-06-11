#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "ui.h"
#include "facet.h"
#include "message.h"
#include "player.h"
#include "game.h"
#include "stuff.h"
#include "item.h"
#include "ability.h"
#include "shapeshf.h"


/*
  Populate the facet list in THE_GAME.
*/
void randomize_facets(game_t * the_game)
{
  unsigned int i;

  unsigned int temp;
  unsigned int a;
  unsigned int b;

  if (the_game == NULL)
    return;

  /*
    facet_none serves as a separator - facets listed after that
    won't be included in the initial list (see facet.h)
  */
  for (i = 0; i < FACET_SLOTS; i++)
    the_game->available_facets[i] = MIN(i, facet_none);

  /* Swap facets around at random. */
  for (i = 0; i < facet_none; i++)
  {
    a = tslrnd() % facet_none;
    b = tslrnd() % facet_none;

    temp = the_game->available_facets[a];
    the_game->available_facets[a] = the_game->available_facets[b];
    the_game->available_facets[b] = temp;
  }

  return;
} /* randomize_facets */



/*
  Returns the first available facet (index) in THE_GAME. The facet will be
  removed from the list.
*/
facet_t pop_facet(game_t * the_game)
{
  unsigned int t;
  unsigned int temp;

  if (the_game == NULL)
    return aug_generic;

  /* Start at the end, iterate backwards. */
  t = FACET_SLOTS;

  while (t > 0)
  {
    t--;
    temp = the_game->available_facets[t];

    if (temp != facet_none)
    {
      the_game->available_facets[t] = facet_none;
      return temp;
    }
  }

  /* No more facets! Fall back to a generic upgrade. */
  return aug_generic;
} /* pop_facet */



/*
  What happens when the player enters an upgrade capsule.
*/
void capsule(creature_t * creature, const unsigned int capsule_y, const unsigned int capsule_x)
{
  int choice;
  item_t * opt[2];
  char * summary;
  level_t * level;
  
  if (creature == NULL ||
      is_player(creature) == false)
  {
    queue_msg("BUG: No, not you.");
    return;
  }

  level = creature->location;
  
  if (level == NULL)
    return;

  queue_msg("You enter an augmentation capsule...");
  msgflush_wait();
  clear_msgbar();
  
  opt[0] = define_facet(pop_facet(game));
  opt[1] = define_facet(pop_facet(game));

  /* When option A is a generic upgrade, there isn't any choice in this anymore. */
  if (opt[0]->item_number == aug_generic)
  {
    queue_msg("Generic upgrade installed.");
    apply_facet(opt[0]);
    del_item(opt[1]);
  }
  else
  {
    /* Give the player a choice. */

    /* RFE: This shouldn't use curses calls directly */
    scr_erase();
    
    summary = get_item_data(opt[0]);
    scr_move(1, 2);
    scr_addstr("a) ");
    scr_addstr(opt[0]->single_id_name);
    constrained_wrap(3, 5, 60, opt[0]->description);
    free(summary);
    
    summary = get_item_data(opt[1]);
    scr_move(9, 2);
    scr_addstr("b) ");
    scr_addstr(opt[1]->single_id_name);
    constrained_wrap(11, 5, 60, opt[1]->description);
    free(summary);
    
    scr_flush();
    
    queue_msg("Which augmentation will you take? (a/b)");
    msgflush_nowait();
    
    choice = 'i';
    while ((choice != 'a') && (choice != 'b'))
      choice = get_keypress();
    
    if (choice == 'a')
    {
      apply_facet(opt[0]);
      del_item(opt[1]);
    }
    else
    {
      apply_facet(opt[1]);
      del_item(opt[0]);
    }
  }

  clear_msgbar();
  
  /* Remove capsule from map */
  set_tile(level, capsule_y, capsule_x, tile_floor);
/*  level->memory[capsule_y][capsule_x] = tile_floor;*/
  
  /* Move creature to the capsules old location and update FOV */
  set_creature_coordinates(creature, capsule_y, capsule_x);
  
  /* Let's see what happened */
  draw_level();
  display_stats(creature);
  unmapped_abilities(NULL);
  
  return;
} /* capsule */



/*
  Generates for FACET a template item that can be applied to a character.
*/
item_t * define_facet(const facet_t facet_index)
{
  item_t * ret;
  
  ret = alloc_item();
  ret->id = known_all;
  ret->item_number = facet_index;
  ret->item_type = item_type_aug;

  switch (facet_index)
  {
    case aug_strength:
      set_item_single_name(ret, "", "strength enhancement");
      set_item_description(ret, "Improves bone and muscle structure, allowing the subject to carry more weight. Mandatory for all population designated as Working Class.");
      ret->attr_mod[attr_carrying_capacity] = STRENGTH_AUG;
      break;

    case aug_arms:
      set_item_single_name(ret, "", "mechanical arms");
      set_item_description(ret, "Allows the subject to throw objects farther and push heavy objects around.");
      ret->attr_mod[attr_throw_range] = +2;
      ret->attr_mod[attr_m_push] = +1;
      break;

    case aug_stealth:
      set_item_single_name(ret, "", "cloaking device");
      set_item_description(ret, "Alters the subjects visual characteristics, including heat signature.");
      ret->attr_mod[attr_stealth] = +2;
      break;

    case aug_swimming:
      set_item_single_name(ret, "", "synthetic lung");
      set_item_description(ret, "A combined carbon dioxide scrubber and particle filter. Protects the subject from harmful aerosols and allows more efficient use of oxygen. Subject becomes immune to gas attacks and can spend two more turns in water.");
      ret->attr_mod[attr_swimming] = +2;
      ret->attr_mod[attr_gas_immunity] = 1;
      break;

    case aug_poison_res:
      set_item_single_name(ret, "", "filter circuit");
      set_item_description(ret, "Helps purge the subjects bloodflow from toxins.");
      ret->attr_mod[attr_poison_resistance] = +3;
      break;

    case aug_vision:
      set_item_single_name(ret, "", "vision enhancement");
      set_item_description(ret, "");
      ret->attr_mod[attr_vision] = +1;
      ret->attr_mod[attr_perception] = +4;
      break;

    case aug_skin:
      set_item_single_name(ret, "", "exoskeleton");
      set_item_description(ret, "Reinforces the subjects skin with synthetic plates (absorb one point of damage, applied after regular armor). When broken mends itself almost instantly, making the subject immune to wounding effects.");
      ret->attr_mod[attr_wound_immunity] = +1;
      ret->attr_mod[attr_absorption] = +1;
      break;

    case aug_claws:
      set_item_single_name(ret, "", "blade hands");
      set_item_description(ret, "A set of metal claws inserted into the subjects palms and knuckles.");
      break;

    case aug_datajack:
      set_item_single_name(ret, "", "datajack");
      set_item_description(ret, "A high-speed communication port installed in the neck of the subject. Bio-hybrid alloy fibers fuse with the spine and let the subject interface directly with terminals and similar devices without additional hardware. Compatible with all major data protocols and stream formats.");
      ret->attr_mod[attr_a_interface] = 1;
      break;

    case aug_phase:
      set_item_single_name(ret, "", "matter transmission");
      set_item_description(ret, "Enables the subject to briefly enter a sub-particle energy state and travel short distances, even through obstacles.");
      ret->attr_mod[attr_m_phase] = 1;
      break;

    case aug_energy:
      set_item_single_name(ret, "", "capacitor");
      set_item_description(ret, "Increases the maximum amount of energy the subject can hold and the rate at which it recovers.");
      ret->attr_mod[attr_ep_max] = 5;
      ret->attr_mod[attr_magic] = 5;
      break;

    case aug_dualwield:
      set_item_single_name(ret, "", "dualwield");
      set_item_description(ret, "Enables the subject to wield two melee weapons in parallell. The most powerful attack in sequence is automatically chosen. Each weapon must occupy one hand only.");
      ret->attr_mod[attr_dualwield] = 1;
      break;

    case facet_dragon:
      set_item_single_name(ret, "", "dragon slayer");
      set_item_description(ret, "");
      break;

    case facet_necro:
      set_item_single_name(ret, "", "lich heir");
      set_item_description(ret, "");
      break;

    case facet_spellbook:
      set_item_single_name(ret, "", "spellbook");
      set_item_description(ret, "");
      break;

    default:
      ret->item_number = aug_generic;
      set_item_single_name(ret, "", "generic upgrade");
      ret->attr_mod[attr_speed] = +10;
      ret->attr_mod[attr_ep_max] = +5;
      break;
  }

  return ret;
} /* define_facet */



/*
  Adds FACET to the players list of chosen facets.
*/
void apply_facet(item_t * facet)
{
  item_t * temp;

  if (facet == NULL)
    return;

/*  temp = game->first_facet;
  facet->next_item = temp;

  if (temp != NULL)
    temp->prev_item = facet;

    game->first_facet = facet;*/

  temp = game->first_facet->next_item;
  facet->next_item = temp;

  if (temp != NULL)
    temp->prev_item = facet;

  game->first_facet->next_item = facet;

  if (facet->item_number == aug_claws)
    game->native->unarmed = virtual_blade_hands;

  list_attributes(facet);

  return;
} /* apply_facet */



void list_attributes(const item_t * facet)
{
  unsigned int i;
  signed int old_base;
  signed int new_base;
  char line[100];

  if (player_shapeshifted())
    return;

  for (i = 0; i < ATTRIBUTES; i++)
  {
    if (facet->attr_mod[i] == 0)
      continue;

    new_base = attr_base(game->native, i);
    old_base = new_base - facet->attr_mod[i];

    if (old_base == 0)
    {
      switch (i)
      {
	case attr_wound_immunity:
	  queue_msg("You are now immune to wounding.");
	  break;
	  
	case attr_p_sleep:
	  queue_msg("You are now immune to sleep.");
	  break;
      }
    }
    else
    {
      switch (i)
      {
	case attr_ep_max:
	case attr_swimming:
	case attr_poison_resistance:
	case attr_speed:
	case attr_stealth:
	case attr_perception:
	case attr_magic:
	case attr_dodge:
	case attr_attack:
	case attr_vision:
	case attr_throw_range:
	case attr_fire_resistance:
	case attr_cold_resistance:
	  sprintf(line, "Your %s is now %d%s.",
		  attr_info[i]->name, new_base,
		  attr_info[i]->percent ? "%" : "");
	  queue_msg(line);
	  break;
      }
    }
  } /* for i */

  return;
} /* list_attribute */
