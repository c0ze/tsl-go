#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <ctype.h>
#include <stdio.h>

#include "stuff.h"
#include "player.h"
#include "ui.h"
#include "level.h"
#include "game.h"
#include "inventory.h"
#include "item.h"
#include "creature.h"
#include "effect.h"
#include "rules.h"
#include "actions.h"
#include "ai.h"
#include "traps.h"
#include "monster.h"
#include "magic.h"
#include "combat.h"
#include "shapeshf.h"
#include "elements.h"
#include "fov.h"
#include "altitude.h"
#include "stacks.h"
#include "vweapon.h"
#include "places.h"
#include "losegame.h"
#include "missile.h"
#include "area.h"
#include "poison.h"
#include "explode.h"
#include "memory.h"
#include "ffield.h"
#include "find.h"
#include "equip.h"
#include "sleep.h"
#include "dwiminv.h"



/*
  Makes all traps on a level visible to the caster. This only affects the player.
 */
blean_t reveal_traps(creature_t * caster, item_t * source, signed int param)
{
  trap_t * trap;
  level_t * level;
  unsigned int y;
  unsigned int x;
  unsigned int detected;

  if (caster != game->player) return false;
  level = caster->location;
  if (level == NULL) return false;

  /* Start detecting traps... */
  detected = 0;

  for (trap = level->first_trap; trap != NULL; trap = trap->next_trap)
  {
    y = trap->y;
    x = trap->x;

    if (trap->hidden)
      continue;

    if (trap->revealed == true)
      continue;

    trap->revealed = true;
    detected++;
    level->memory[y][x] = trap->gent;
  }
  
  if (source != NULL)
  {
    queue_msg("You sense the presence of traps.");
    make_item_known(source);
  }
  
  return true;
} /* reveal_traps */




/*
*/
blean_t magic_weapon(creature_t * caster, item_t * source, signed int param)
{
/*  item_t * new_weapon;*/

  if (caster == NULL) return false;

  make_item_known(source);
  
/*  new_weapon = build_item(treasure_doomblade);
    new_weapon->lifetime = MAGIC_WEAPON_LIFETIME;*/
  queue_msg("Your hands seem to be on fire.");
  set_temp_weapon(caster, virtual_flame_hands, MAGIC_WEAPON_LIFETIME);

  if (is_player(caster))
    queue_msg("");

  return true;
} /* magic_weapon */



blean_t destroy_trap(creature_t * caster, item_t * source, signed int param)
{
  signed int move_y;
  signed int move_x;
  unsigned int trap_y;
  unsigned int trap_x;
  blean_t spell_known;
  level_t * level;
  trap_t * trap;
  creature_t * someone;
  dir_t dir;

  if (caster == NULL)
    return false;

  level = caster->location;
  
  if (level == NULL)
    return false;

  if (is_player(caster) == false)
    return false;
  
  if ((source == NULL) || (identified(source) & known_name))
    spell_known = true;
  else
    spell_known = false;
  
  if (spell_known)
    queue_msg("Destroy trap in which direction?");
  else
    queue_msg("In which direction?");

  msgflush_nowait();

  if (param)
    dir = param;
  else
    dir = get_direction();

  if (dir == dir_none)
    return false; /* Cancelled */

  dir_to_speed(dir, &move_y, &move_x);

  trap_y = caster->y + move_y;
  trap_x = caster->x + move_x;
  
  someone = find_creature(level, trap_y, trap_x);

  if (is_mimic(someone) && someone->gent == gent_dart_trap)
  {
    if (spell_known)
      reveal_mimic(someone);
    else
      queue_msg("Nothing happens.");

    return true;
  }

  trap = find_trap(level, trap_y, trap_x);

  if (trap != NULL)
  {
    if (trap->hidden)
    {
      queue_msg("Nothing happens.");
      return true;
    }

    if ((trap->revealed == false) ||
	(can_see(game->player, trap_y, trap_x) == false))
    {
      queue_msg("Nothing happens.");
    }
    else
    {
      make_item_known(source);
      queue_msg("You have destroyed a trap!");
    }
    
    del_trap(trap);
    update_level_memory(level);
    draw_level();
  }
  else
    queue_msg("Nothing happens.");
	 
  return true;
} /* destroy_trap */



/*
  Lets CASTER select an inventory item to be recharged.
 */
blean_t cast_recharge(creature_t * caster, item_t * source, signed int param)
{
  item_t * item_to_recharge;
    
  if (caster == NULL)
    return false;

  id_if_not_known(source);

  if (is_player(caster))
  {
    if (count_inventory(caster, &is_rechargeable) == 0)
    {
      queue_msg("You have nothing to recharge.");
      return false;
    }

    queue_msg("Recharge which item?");
    
    msgflush_nowait();
    
    /* Let the user select what item to recharge */
    item_to_recharge = dwim_select(caster, &is_rechargeable, "w");
    
    if (item_to_recharge != NULL)
    {
      /* An item was selected; try to recharge it */
      if (is_rechargeable(item_to_recharge) == true)
      {
	make_item_known(source);
	recharge(caster, item_to_recharge, source);
	return true;
      }
      else
      {
	queue_msg("Nothing happens.");
	return true;
      }
    }
    else
    {
      /* User cancelled */
      return false;
    }
  }

  return false;
} /* cast_recharge */



/*
  Lets CASTER identify an inventory item.
*/
blean_t cast_identify(creature_t * caster, item_t * source, signed int param)
{
  item_t * item_to_id;
  char * name;
  char line[80];
/*  blean_t was_known;*/

  if (caster == NULL)
    return false;

  if (is_player(caster) == false)
    return false;
  
/*  was_known = false;

  if ((source == NULL) ||
      (identified(source) & known_name))
  {
    was_known = true;
    }*/
  
  id_if_not_known(source);

  if (count_inventory(caster, NULL) == 0)
  {
    queue_msg("Unfortunately, you don't have any items.");
    return false;
  }

  /* Let the user select what item is to be identified */
  queue_msg("Identify which item?");
  msgflush_nowait();
  item_to_id = dwim_select(caster, NULL, NULL);  /*select_inventory_item(caster, NULL, false, NULL);*/
  
  if (item_to_id == NULL)
  {
    queue_msg("The magic fades.");
    return true;
  }

  identify(item_to_id);
  make_item_known(source);
  
  name = get_item_name(item_to_id);
  sprintf(line, "%s.", name);
  line[0] = toupper(line[0]);
  queue_msg(line);
  free(name);
  name = NULL;
  
  return true;
} /* cast_identify */



/*
  Dissolves any items under CREATURE, accompanied by a "X dissolves in
  Y" message. This ALWAYS returns FALSE, since we want slimes to
  continue their turn even after they've dissolved items.
*/
blean_t dissolve_items(creature_t * creature, item_t * source, signed int param)
{
  unsigned int items;
  unsigned int y;
  unsigned int x;
  unsigned int i;
  item_t * item;
  item_t ** item_list;
  char line[80];
  char * name;

  if (creature == NULL || creature->location == NULL)
    return false;

  y = creature->y;
  x = creature->x;

  /* Get all items under the creature. */
  items = find_items(creature->location,
		     creature->y, creature->x,
		     &item_list);
  
  /* Destroy each one. */
  for (i = 0; i < items; i++)
  {
    item = item_list[i];

    /* This shouldn't normally happen */
    if (item == NULL)
      continue;

    if (item->indestructible)
      continue;

    /* If it's within the players view, display a message. Otherwise,
       it should silently disappear. */
    if (is_player(creature))
    {
      name = get_item_name(item);
      sprintf(line, "%s dissolve%s under you!", name, stack_size(item) > 1 ? "" : "s");
      upperfirst(line);
      queue_msg(line);
      free(name);
      name = NULL;
    }
    else if (can_see(game->player, y, x))
    {
      name = get_item_name(item);
      sprintf(line, "%s dissolve%s in %s!", name, stack_size(item) > 1 ? "" : "s", creature->name_the);
      upperfirst(line);
      queue_msg(line);
      free(name);
      name = NULL;
    }
    
    del_item(item);
  }

  free(item_list);

  return false;
} /* dissolve_items */



/*
  Reveals the map of the current level.
  This is only intended for use by the player.
 */
blean_t magic_mapping(creature_t * caster, item_t * source, signed int param)
{
  unsigned int y;
  unsigned int x;
  level_t * level;
  
  if (is_player(caster) == false)
    return false;

  level = game->player->location;

  if (level == NULL)
    return false;

  id_if_not_known(source);

  for (y = 0; y < level->size_y; y++)
  {
    for (x = 0; x < level->size_x; x++)
    {
      /*
	All non-floor tiles on the map will be copied to the level
	memory. We skip floors so the player can still see what parts
	they haven't explored. We only map areas that are currently
	blank in level memory. One reason is to preserve traps that
	might be present on regular floors.
      */
      if (level->map[y][x] != tile_floor && level->memory[y][x] == gent_blank)
	level->memory[y][x] = tile_info[level->map[y][x]]->gent;
    }
  }
  
  /* We must rebuild this */
/*  build_fov(game->player);*/

  set_creature_coordinates(caster, caster->y, caster->x);
  update_level_memory(level);
  draw_level();

  return true;
} /* magic_mapping */






/*
 */
blean_t raise_dead(creature_t * caster, item_t * source, signed int param)
{
  unsigned int y;
  unsigned int x;
  signed int move_y;
  signed int move_x;
  level_t * level;
  item_t ** stuff;
  unsigned int items;
  creature_t * creature;
  signed int range;
  unsigned int i;
  blean_t spell_known;
  char line[80];

  /* TODO: This spell is disabled for now. Needs work. */
  return false;

  if (caster == NULL)
    return false;

  level = caster->location;

  if (level == NULL)
    return false;

  if ((source == NULL) ||
      (identified(source) & known_name))
  {
    spell_known = true;
  }
  else
  {
    spell_known = false;
  }

  if (is_player(caster))
  {
    /* Let the user select a direction */
    if (spell_known)
      queue_msg("Raise Dead in which direction?");
    else
      queue_msg("In which direction?");

    msgflush_nowait();
    
    /* This is the last chance to escape the casting */
/*    if (get_direction(&move_y, &move_x) == false)
      return false;*/
  }
  else
  {
    /* TODO: Feel free to implement this. It should target the direction with most corpses. */
    return false;
  }

  range = roll(1, 4);

  if (source)
    range += 2;
  else
    range += 5;
  
  y = caster->y;
  x = caster->x;
  
  while (range > 0)
  {
    range--;

    y += move_y;
    x += move_x;

    if (is_walkable(level, false, y, x) == false)
      break;

    items = find_items(level, y, x, &stuff);

    if (items > 0)
    {
      for (i = 0; i < items; i++)
      {
	if (stuff[i] == NULL)
	  continue; /* Proceed to the next item on this tile. */

	if ((stuff[i]->item_number == treasure_corpse) ||
	    (stuff[i]->item_number == treasure_cranium))
	{
	  switch (stuff[i]->item_number)
	  {
	    case treasure_corpse:
	      creature = build_monster(monster_ghoul);
	      break;

	    case treasure_cranium:
	      creature = build_monster(monster_burning_skull);
	      break;

	    case treasure_severed_hand:
	      creature = build_monster(monster_severed_hand);
	      break;

	    default:
	      sprintf(line, "BUG: undefined monster for corpse \"%s\"",
		      stuff[i]->single_id_name);
	      return false;
	  }

	  if (creature == NULL)
	    continue;

	  del_item(stuff[i]);

	  if (put_or_destroy(level, creature, y, x))
	  {
	    /*creature->alignment = caster->alignment;*/
	  
	    if (can_see(game->player, y, x))
	    {
	      identify(source);
	      creature->detected = true;

	      sprintf(line, "%s rises from the ground!", creature->name_one);
	      upperfirst(line);
	      queue_msg(line);
	    }
	  }
	  
	  break; /* Proceed to the next tile in range. */
	}
      }

      free(stuff);
    }
  }

  return true;
} /* raise_dead */



/*
  Summons a small creature to aid CASTER.
*/
blean_t summon_familiar(creature_t * caster, item_t * source, signed int param)
{
  creature_t * familiar;

  if ((caster == NULL) ||
      (is_player(caster) == false))
  {
    return false;
  }

  make_item_known(source);

  switch (roll(1, 4))
  {
    default:
      familiar = build_monster(monster_imp);
      break;
  }

  charm(familiar);

  familiar->lifetime = DEFAULT_SUMMON_LIFETIME;

  msg_one(familiar, "has arrived.");

  if (attach_creature(caster->location, familiar) == NULL)
  {
    find_nearest_free_spot(familiar, caster->y, caster->x);

    draw_level();

    return true;
  }
  else
  {
    del_creature(familiar);
    familiar = NULL;
    
    return false;
  }
  
  return false;
} /* summon_familiar */



blean_t first_aid(creature_t * creature, item_t * source, signed int param)
{
  effect_t * eff;

  if (creature == NULL)
    return false;

  eff = get_effect_by_id(creature, effect_wound);

  if (eff == NULL)
  {
    queue_msg("You are not wounded.");
    return false;
  }
  
  msg_expire(eff);
  effect_expire(eff);
  
  heal(creature, roll(1, 3));

  return true;
} /* first_aid */


void charm(creature_t * victim)
{
  if (victim == NULL)
    return;

/*  victim->alignment = alignment_player;*/
  set_attr(victim, attr_player_ally, 1);
/*  prolong_effect(victim, effect_confusion, CONFUSION_TIME);*/
  victim->ai_state = ai_offensive;
  victim->detected = true;
  /*victim->aggravated = true;*/

  

/*TODO:  set_master(victim, game->player);*/

  return;
} /* charm */



blean_t enslave(creature_t * caster, item_t * source, signed int param)
{
  creature_t * target;
  signed int move_y;
  signed int move_x;
  blean_t spell_known;
  dir_t dir;

  if (caster == NULL)
    return false;

  if ((source == NULL) || (identified(source) & known_name))
    spell_known = true;
  else
    spell_known = false;
  
  if (is_player(caster))
  {
    /* Let the user select a direction */
    if (spell_known)
      queue_msg("Enslave whom?");
    else
      queue_msg("In which direction?");

    msgflush_nowait();
    
    /* This is the last chance to escape the casting */
    dir = get_direction();

    if (dir == dir_none)
      return false;
  }
  else
  {
    /* TODO: NPCs can't use this spell yet. */
    return false;
  }

  dir_to_speed(dir, &move_y, &move_x);

  target = find_creature(caster->location,
				    caster->y + move_y,
				    caster->x + move_x);

  if (target == NULL)
  {
    if (spell_known)
    {
      queue_msg("No one is there!");

      /* RFE: Should this still consume a turn? We could return false here... */
    }
    else
    {
      queue_msg("Nothing happens.");
    }

    return true;
  }
  else
  {
    make_item_known(source);
	  
    if (is_player(caster))
    {
/*      char line[80];*/
      
/*      if (target->alignment == alignment_player)
      {
	sprintf(line, "%s%s already is your loyal servant.",
		(is_unique(target) ? "" : "That "), target->name_only);
	upperfirst(line);
	queue_msg(line);
      }
      else */if ((source != NULL)
	       && roll_xn(WAND_OF_ENSLAVEMENT, attr_current(target, attr_magic)))
      {
	/* This was cast from a wand, uses default spell power. */
	charm(target);
	msg_one(target, "accepts you as its new master.");
	draw_level();
      }
      else if (roll_xn(attr_current(caster, attr_magic), attr_current(target, attr_magic)))
      {
	/* This was cast from memory, uses Magic attribute for spell power. */
	charm(target);
	msg_one(target, "accepts you as its new master.");
      }
      else
      {
	msg_one(target, "resists!");
      }
    }
  
    /* TODO: Again, no effect if caster is NPC. */
    
    return true;
  }

  return false;
} /* enslave */



/*
  Makes CREATURE levitate.
 */
blean_t levitate(creature_t * creature)
{
/*  blean_t already_levitating;*/

  if (creature == NULL)
    return false;

/*  if (attr_current(creature, attr_levitate) > 0)
    already_levitating = true;
  else
  already_levitating = false;*/

  creature->attr_base[attr_levitate] += LEVITATE_TIME + tslrnd() % LEVITATE_TIME;

  change_altitude(creature);

  return false;
} /* levitate */



blean_t leap(creature_t * creature, item_t * source, signed int param)
{
  unsigned int y;
  unsigned int x;
  unsigned int i;
  signed int move_y;
  signed int move_x;
  level_t * level;
  creature_t * target;
  char line[80];
  char creature_name[40];
  char target_name[40];
  dir_t dir;

  if (creature == NULL)
    return false;

  level = creature->location;

  if (level == NULL)
    return false;
  
  if (is_player(creature))
  {
    /* Let the user select a direction */
    queue_msg("Leap in which direction?");

    msgflush_nowait();
    
    /* This is the last chance to escape the leaping. */
    dir = get_direction();

    if (dir == dir_none)
    {
      clear_msgbar();
      return false;
    }
  }
  else
  {
    dir = target_best_direction(creature, LEAP_RANGE + 2);

    if (dir == dir_none)
      return false;
  }

  dir_to_speed(dir, &move_y, &move_x);

  /* */
  if (is_walkable(level, false, creature->y + move_y, creature->x + move_x) == false)
  {
    if (is_player(creature))
      queue_msg("Not enough room!");

    return false;
  }

  y = creature->y;
  x = creature->x;

  for (i = 1; i <= LEAP_RANGE; i++)
  {
    y += move_y;
    x += move_x;

    target = find_creature(level, y, x);

    if ((target != NULL) ||
	(i == LEAP_RANGE) ||
	(on_map(level, y, x) == false) ||
	is_wall(level, y, x))
    {
      set_creature_coordinates(creature, y - move_y, x - move_x);

      if (can_see_creature(game->player, creature))
      {
	sprintf(creature_name, "%s", creature->name_one);
	creature->detected = true;
      }
      else
	sprintf(creature_name, "something");

      if (can_see_creature(game->player, target))
      {
	sprintf(target_name, "%s", target->name_one);
	target->detected = true;
      }
      else
	sprintf(target_name, "something");

      strcat(line, "");

      if ((target != NULL) &&
	  (can_see_creature(game->player, creature) ||
	   can_see_creature(game->player, target) ||
	   is_player(creature) ||
	   is_player(target)))
      {
	if (is_player(target))
	{
	  sprintf(line, "%s leaps at you!", creature_name);
	  creature->detected = true;
	  upperfirst(line);
	}
	else if (is_player(creature))
	{
	  sprintf(line, "You leap at %s!", target_name);
	}
	else
	{
	  sprintf(line, "%s leaps at %s!", creature_name, target_name);
	  upperfirst(line);
	}
      }
      else
      {
	if (is_player(creature))
	{
	  if (is_wall(level, y, x))
	    sprintf(line, "You leap into a wall!");
	  else
	    sprintf(line, "You leap!");
	}
	else
	{	  
	  if (is_wall(level, y, x))
	    sprintf(line, "%s leaps into a wall!", creature_name);
	  else
	    sprintf(line, "%s leaps!", creature_name);

	  upperfirst(line);
	}
      }
      
      creature->altitude = altitude_floating;

      queue_msg(line);
      
      if (target != NULL)
	attack(creature, target);
      
      change_altitude(creature);
      
      draw_level();
      
      msgflush_wait();
      clear_msgbar();
      
      draw_level();
      
      return true;
    }
  }
  
  return false;
} /* leap */




/*
  Lets the player (and only the player) wish for an item.
*/
blean_t wish(creature_t * caster, item_t * source, signed int param)
{
  char name[80];
  item_t * item;
  
  if (caster == NULL)
    return false;

  if (is_player(caster) == false)
    return false;

  id_if_not_known(source);
  msgflush_wait();

  read_string("Wish for what?", name, 79);

  item = build_any_item(name, true);

  if (item == NULL)
    return true;

  if (item->no_wish)
  {
    /* The player is not allowed to wish for this item. */
    /* TODO: Perhaps transform the item into something useless? */
    del_item(item);
    return true;
  }

  attach_item_to_creature(caster, item);

  return true;
} /* wish */



/*
  Lets CASTER cast "Bone Crush".
*/
blean_t bone_crush(creature_t * caster, item_t * source, signed int param)
{
  level_t * level;
  creature_t * target;
  signed int move_y;
  signed int move_x;
/*  signed int dam_dealt;*/
  char line[80];
/*  blean_t message;*/
  blean_t spell_known;
  blean_t skeleton;
  dir_t dir;

/*  message = false;*/
  
  if (caster == NULL)
    return false;

  level = caster->location;

  if (level == NULL)
    return false;

  if ((source == NULL) || (identified(source) & known_name))
    spell_known = true;
  else
    spell_known = false;

  if (is_player(caster))
  {
    /* Let the user select a direction */

    /* TODO: write something better here */
    if (spell_known)
      queue_msg("Cast Bone Crush in which direction?");
    else
      queue_msg("In which direction?");

    msgflush_nowait();
    
    /* This is the last chance to escape the casting */
    dir = get_direction();

    if (dir == dir_none)
    {
      clear_msgbar();
      return false;
    }
  }
  else
  {
    dir = target_best_direction(caster, 2);

    if (dir == dir_none)
      return false;
  }

  dir_to_speed(dir, &move_y, &move_x);

  /* Check if we hit anyone... */
  target = find_creature(level,
				    caster->y + move_y,
				    caster->x + move_x);

  if (target == NULL)
    return true; /* Casting succeeded, but nothing happened! */

/* TODO: Fix this */

  skeleton = true;

/*
  switch (target->shape)
  {
    case shape_coffin:
    case shape_statue:
    case shape_slime:
    case shape_ghost:
    case shape_elemental:
    case shape_eye:
    case shape_insect:
      skeleton = false;

    default:
      skeleton = true;
      break;
      }*/

  if (skeleton == false)
    return true; /* Only creatures that have a skeleton are affected. */

  /* Deal damage... */
/*  dam_dealt = damage(target, roll(2, 4), damage_general);*/
  
  if (is_player(target))
  {
    msg_one(caster, "%s%s touches you!");
    queue_msg("You feel your bones twitch and crack beneath your flesh!");
    
    if (killed(target))
    {
      queue_msg("You die...");
      
      sprintf(line, "were crushed by %s", caster->name_one);
      
      check_for_player_death(line);
    }
  }
  else if (can_see_creature(game->player, target))
  {
    sprintf(line, "You hear a cracking sound coming from %s!", target->name_the);
    upperfirst(line);
    queue_msg(line);
    
    if (killed(target))
    {
      msg_one(target, "is crushed!");
      kill_creature(target, false);
    }
    else
    {
      /* The creature survived. */
      if (is_player(caster))
	aggravate(target);
    }
  }
  
  return true;
} /* bone_crush */



/*
  Lets CASTER give off an electric shock.
*/
blean_t shock(creature_t * caster, item_t * source, signed int param)
{
  level_t * level;
  creature_t * target;
  signed int move_y;
  signed int move_x;
  char line[100];
  blean_t spell_known;
  dir_t dir;
  blean_t see_target;
  blean_t see_caster;
  
  if (caster == NULL)
    return false;

  level = caster->location;

  if (level == NULL)
    return false;

  if ((source == NULL) || (identified(source) & known_name))
    spell_known = true;
  else
    spell_known = false;

  if (param)
  {
    dir = param;
  }
  else if (is_player(caster))
  {
    /* Let the user select a direction */

    /* TODO: write something better here */
    if (spell_known)
      queue_msg("Shock in which direction?");
    else
      queue_msg("In which direction?");

    msgflush_nowait();
    
    /* This is the last chance to escape the casting */
    dir = get_direction();

    if (dir == dir_none)
    {
      clear_msgbar();
      return false;
    }
  }
  else
  {
    dir = target_best_direction(caster, 2);

    if (dir == dir_none)
      return false;
  }

  dir_to_speed(dir, &move_y, &move_x);

  /* Check if we hit anyone... */
  target = find_creature(level,
			 caster->y + move_y,
			 caster->x + move_x);
  
  if (target == NULL)
  {
    if (is_player(caster) == false)
    {
      queue_msg("BUG: bad direction for shock()");
      return true;
    }

    if (get_tile(level, caster->y + move_y, caster->x + move_x) == tile_generator)
    {
      disable_forcefield(level, caster->y + move_y, caster->x + move_x);

      if (can_see(caster, caster->y + move_y, caster->x + move_x))
      {
	queue_msg("You overload the generator, temporarily disabling it.");
	update_level_memory(level);
	draw_level();
	return true;
      }
    }

    queue_msg("You shock!");
    
    return true; /* Casting succeeded, but nothing happened! */
  }

  see_caster = can_see_creature(game->player, caster);
  see_target = can_see_creature(game->player, target);

  /* Deal damage... */
  /* TODO: Insulation */
  damage(target, sroll(SHOCK_DAMAGE));

  if (is_player(target))
  {
    if (see_caster)
      msg_one(caster, "shocks you!");
    else
      queue_msg("You get shocked!");
    
    if (killed(target))
    {
      queue_msg("You die...");
      sprintf(line, "were electrocuted by %s", caster->name_one);
      check_for_player_death(line);
    }
  }
  else if (is_player(caster))
  {
    if (see_target)
    {
      sprintf(line, "You shock %s!", target->name_the);
      queue_msg(line);
    }
    else
      queue_msg("You shock something!");
  }
  else if (see_target || see_caster)
  {
    sprintf(line, "%s shocks %s!",
	    (see_caster ? caster->name_one : "something"),
	    (see_target ? target->name_one : "something"));
    upperfirst(line);
    queue_msg(line);
  }

  if (killed(target))
  {
    if (see_target)
    {
      if (is_player(caster))
	msg_the(target, "is electrocuted!");
      else
	msg_one(target, "is electrocuted!");
    }

    kill_creature(target, false);
  }
  else
  {
    creature_sleep(target, false);

    /* The creature survived. */
    if (is_player(caster))
      aggravate(target);
  }
  
  return true;
} /* shock */



/*
  Deathspell. 50/50 chance of the caster or the target dying.
*/
blean_t deathspell(creature_t * caster, item_t * source, signed int param)
{
  level_t * level;
  creature_t * target;
  signed int move_y;
  signed int move_x;
/*  char line[100];*/
  blean_t spell_known;
  dir_t dir;

  if (caster == NULL ||
      (level = caster->location) == NULL)
    return false;

  if ((source == NULL) || (identified(source) & known_name))
    spell_known = true;
  else
    spell_known = false;

  /* We don't want monsters to do this. */
  if (is_player(caster) == false)
    return false;

  /* Let the player select a direction */
  if (spell_known)
    queue_msg("Deathspell in which direction?");
  else
    queue_msg("In which direction?");
  
  msgflush_nowait();
  
  /* This is the last chance to escape the casting */
  dir = get_direction();
  
  if (dir == dir_none)
  {
    /* Cancelled */
    clear_msgbar();
    return false;
  }

  dir_to_speed(dir, &move_y, &move_x);

  /* Check if we hit anyone... */
  target = find_creature(level,
			 caster->y + move_y,
			 caster->x + move_x);
  
  if (target == NULL)
  {
    queue_msg("No one is there!");

    /*
      This doesn't cost a turn. Due to the inherent risk of dying, I
      don't think anyone will use this to probe tiles
      spontaneously.
    */
    return false;
  }

  queue_msg("Death...");
  msgflush_wait();

  if (roll(1, 2) == 1)
  {
    /* You died */
    queue_msg("Yours!");

    set_attr(caster, attr_killed, 1);
    check_for_player_death("failed a Deathspell");
  }
  else
  {
    if (can_see_creature(game->player, target))
      msg_one(target, "dies.");
    else
      queue_msg("Theirs!");
    
    add_anim(anim_type_deathspell, target->y, target->x, 0, 0);

    kill_creature(target, false);
  }
  
  return true;
} /* deathspell */



/*
  
 */
blean_t aimed_shot_prepare(creature_t * creature, item_t * source, signed int param)
{
  level_t * level;
  creature_t * target;
  item_t * weapon;
  signed int move_y;
  signed int move_x;
  dir_t dir;
  signed int y;
  signed int x;
  signed int range;

  if (creature == NULL)
    return false;

  level = creature->location;

  if (level == NULL)
    return false;

  weapon = get_equipped(creature, item_type_r_weapon, 0);

  if (weapon == NULL)
  {
    if (is_player(creature))
    {
      queue_msg("You don't have any missile weapon equipped!");
      return false;
    }
    else if (creature->id != monster_chainsaw_ogre)
      return false;
  }

  if (is_blinded(creature))
  {
    if (is_player(creature))
      queue_msg("It is difficult to aim when you cannot see.");

    return false;
  }

  if (get_equipped(creature, item_type_ammo, 0) == NULL)
  {
    if (is_player(creature))
      queue_msg("Out of ammo!");

    return false;
  }

  if (weapon)
    range = get_weapon_range(weapon);
  else
    range = attr_current(creature, attr_throw_range);

  range += 1;

  if (creature == game->player)
  {
    /* Let the user select a direction */
    queue_msg("Aim in which direction?");
    msgflush_nowait();
    
    dir = get_direction();

    if (dir == dir_none)
    {
      clear_msgbar();
      return false;
    }
  }
  else
  {
    dir = target_best_direction(creature, range);

    if (dir == dir_none)
      return false;
  }

  creature->multiaction = MULTIACTION_AIM;
  creature->multi_param = dir;
  
  /* TODO: Why won't this work? */
  dir_to_speed(dir, &move_y, &move_x);
  y = creature->y;
  x = creature->x;

  while (range > 0)
  {
    range--;
    y += move_y;
    x += move_x;

    target = find_creature(level, y, x);

    if (target &&
	is_player(target) &&
	can_see_creature(game->player, creature))
    {
      msg_one(creature, "aims at you...");
      draw_level();
/*      msgflush_wait();
	clear_msgbar();*/
      return true;
    }
    
    if (is_flyable(level, y, x) == false)
      break;
  }

  if (is_player(creature))
    queue_msg("You take aim...");
  else if (can_see_creature(game->player, creature))
    msg_one(creature, "aims...");

  return true;
} /* aimed_shot_prepare */





/*
  
 */
blean_t aimed_shot_release(creature_t * creature, item_t * source, signed int param)
{
  item_t * launcher;
  item_t * missile;
  unsigned int range;

  if (creature == NULL)
    return false;

  if (creature->multiaction != MULTIACTION_AIM)
  {
    queue_msg("BUG: In aimed_shot_release() but multiaction wasn't set!");
    return false;
  }

  launcher = get_equipped(creature, item_type_r_weapon, 0);

  if (launcher != NULL)
    range = get_weapon_range(launcher);
  else
    range = attr_current(creature, attr_throw_range);

  missile = get_item_from_stack(get_equipped(creature, item_type_ammo, 0));
  
  if (missile == NULL)
  {
    queue_msg("BUG: No ammo in aimed_shot_release() - how did this happen?");
    return false;
  }

  if (missile->item_number == treasure_grenade)
  {
    missile->ricochets = true;
  }
  
  /* This is the point of Aimed Shot */
  missile->attr_mod[attr_i_precision] += 5;
/*  missile->custom[attr_i_damage] += 5;*/

  if (is_player(creature))
  {
    queue_msg("You release your Aimed Shot!");
  }

  fire_missile(creature, creature->multi_param,
	       launcher, missile, false,
	       range);

  return true;
} /* aimed_shot_release */



blean_t dash(creature_t * creature, item_t * source, signed int param)
{
  if (creature == NULL)
    return false;

  damage(creature, 1);

  if (killed(creature))
  {
    if (is_player(creature))
    {
      queue_msg("Your over-worked heart finally gives up.");
      queue_msg("You suffer a heart attack and die.");
      check_for_player_death("had a heart attack");
    }
    else if (can_see_creature(game->player, creature))
    {
      msg_one(creature, "%s%s collapses from exhaustion!");
      kill_creature(creature, false);
    }
  }

  if (is_player(creature))
  {
    queue_msg("You dash!");
  }
  else if (can_see_creature(game->player, creature))
  {
    msg_one(creature, "dashes!");
  }   
   
  /* Cancels slow and speeds up creature */
  effect_expire(get_effect_by_id(creature, effect_slow));
  prolong_effect(creature, effect_haste, HASTE_LENGTH);

  return true;
} /* dash */


/*
  Lets CASTER shoot a frost ray.
*/
blean_t frost_ray(creature_t * caster, item_t * source, signed int param)
{
  unsigned int y;
  unsigned int x;
  level_t * level;
  creature_t * creature;
  signed int damage_to_deal;
  unsigned int range;
  /*  unsigned int i;*/
  char line[80];
  blean_t message;
  blean_t spell_known;
  area_t * area;
  area_t * temp;
  dir_t dir;
  blean_t extinguisher;

  message = false;
  dir = dir_none;
  
  if (caster == NULL)
    return false;

  level = caster->location;

  if (level == NULL)
    return false;

  if ((source == NULL) || (identified(source) & known_name))
    spell_known = true;
  else
    spell_known = false;

  if (source && source->item_number == treasure_fire_extinguisher)
    extinguisher = true;
  else
    extinguisher = false;

  if (extinguisher)
    range = 2;
  else
    range = FROST_RAY_RANGE;

  if (param != dir_none)
  {
    dir = param;
  }
  else if (caster == game->player)
  {
    /* Let the user select a direction */
    if (extinguisher)
      queue_msg("Spray in which direction?");
    else if (spell_known)
      queue_msg("Shoot Frost Ray in which direction?");
    else
      queue_msg("In which direction?");
    
    msgflush_nowait();
    
    dir = get_direction();
    
    if (dir == dir_none)
    {
      clear_msgbar();
      return false;
    }
  }
  else
  {
    /*
      Let the AI decide what direction we should cast in.
    */
    dir = cone_direction(caster, range, true);
    
    if (dir == dir_none)
      return false;
  }
  
  /*
    If the player is casting this there's a random chance of
    failure. Wands and enemy spellcasters never fail.
    Note that the roll is reversed, this is the case for _failure_.
  */
  if (source == NULL && is_player(caster) &&
      roll_xn(FROST_RAY_DIFFICULTY, attr_current(caster, attr_magic)))
  {
    queue_msg("Your frost ray fizzles.");
    
    /* Action was completed. */
    return true;
  }

  /* Casting succeeded! */

  /*
    If we can see the item activating the spell, identify the item. If
    an enemy zaps a wand out of sight, or the player zaps one while
    blind, we won't see what's going on.
  */
  if (can_see(game->player, caster->y, caster->x))
    make_item_known(source);

  /* Figure out which tiles are affected. */
  y = caster->y;
  x = caster->x;
  area = new_area(y, x);
  area_cone(area, level, y, x, dir, range, true);

  if (is_player(caster))
  {
    if (extinguisher)
      queue_msg("You spray!");
    else
      queue_msg("You shoot a frost ray!");

    message = true;
  }
  else if (can_see(game->player, y, x))
  {
    /* TODO: What about creatures that haven't been detected? */
    msg_one(caster, "shoots a frost ray!");
    message = true;
  }
  
  if (area_of_effect(game->player, area->next, missile_frost))
  {
    /*draw_missile(y, x, missile_frost, 0, 0);*/

    /*
      We have now seen the ray. If this was the first time (we didn't
      see the caster earlier), display another message.
    */
    if (message == false)
    {
      queue_msg("You see a frost ray!");
      message = true;
    }
  }

  for (temp = area->next; temp != NULL; temp = temp->next)
  {
    y = temp->y;
    x = temp->x;

    expose_tile_to_cold(level, y, x);
    
    /* Check if we hit anyone... */
    creature = find_creature(level, y, x);

    if (creature != NULL)
    {
      reveal_mimic(creature);

      /* We hit someone. Let's do some damage. */
      damage_to_deal = intrnd(FROST_RAY_MIN_DAMAGE, FROST_RAY_MAX_DAMAGE);
      damage_to_deal = damage_armor(creature, damage_to_deal, damage_cold);
      damage(creature, damage_to_deal);
      
      if (is_player(creature))
      {
	queue_msg("You are hit by the frost ray!");

	damage_popup(creature, damage_to_deal);
	
	if (killed(creature))
	{
	  queue_msg("You die...");
	  sprintf(line, "were killed by %ss frost ray", caster->name_one);
	  check_for_player_death(line);
	}
      }
      else
      {
	damage_popup(creature, damage_to_deal);

	if (can_see(game->player, y, x))
	{
	  /* TODO: Invisible (but seen) caster */
	  if (extinguisher)
	    msg_one(creature, "is hit by the spray!");
	  else
	    msg_one(creature, "is hit by the frost ray!");
	  
	  if (killed(creature))
	  {
	    /* TODO: This should be more interesting. */
	    msg_one(creature, "is destroyed!");
	    kill_creature(creature, false);
	  }
	  else
	  {
	    /* The creature survived. */
	    aggravate(creature);
	  }
	}
      } /* else */
    }
  } /* for temp */

  del_area(area);

  area = NULL;
  temp = NULL;

  /*
    If message has been set, then at some point we have seen the ray
    or its effects. If this is the case, we should pause so that the
    player can see what area was affected.
  */
  if (message == true)
  {
    update_level_memory(caster->location);
    msgflush_wait();
    clear_msgbar();
  }

  return true;
} /* frost_ray */



/*
  Makes the player forget the layout of the level we're on,
  i.e. clears the automap.
*/
blean_t amnesia(creature_t * caster, item_t * source, signed int param)
{
  level_t * level;
  unsigned int y;
  unsigned int x;

  /*
    RFE: Maybe this could be made into a touch spell for certain
    creatures.
  */
  
  if (is_player(caster) == false)
    return false;

  level = get_current_level();

  if (level == NULL)
    return false;

  for (y = 0; y < level->size_y; y++)
  {
    for (x = 0; x < level->size_x; x++)
    {
      level->memory[y][x] = gent_blank;
    }
  }

  /*
    RFE: What about traps and enemies? Should we reset their detected
    status as well?
  */

  queue_msg("You suddenly feel very forgetful.");

  build_fov(caster);
  update_level_memory(level);
  draw_level();

  msgflush_wait();
  clear_msgbar();

  return true;
} /* amnesia */



/*

*/
blean_t datajack(creature_t * caster, item_t * source, signed int param)
{
  level_t * level;

  dir_t dir;

  signed int target_y;
  signed int target_x;

/*  char line[80];*/

  if (caster == NULL)
    return false;
  
  level = caster->location;

  if (level == NULL)
    return false;

  if (is_player(caster) == false)
    return false;

  /* Let the user select a direction. */
  if (source)
    queue_msg("Probe what?");
  else
    queue_msg("Interface with what?");

  msgflush_nowait();
  
  dir = get_direction();
  
  /* Should we abort? */
  if (dir == dir_none)
  {
    clear_msgbar();
    return false;
  }
  
  dir_to_speed(dir, &target_y, &target_x);

  target_y += caster->y;
  target_x += caster->x;

  switch (get_tile(level, target_y, target_x))
  {
    case tile_terminal:
      queue_msg("You connect to the terminal.");
      msgflush_wait();
      set_tile(level, target_y, target_x, tile_floor);
      magic_mapping(caster, NULL, 0);
      queue_msg("You download a layout of the level.");
      break;

    case tile_generator:
      queue_msg("You disable the generator.");
      /*set_tile(level, target_y, target_x, tile_floor);*/
      disable_forcefield(level, target_y, target_x);
      /*build_forcefields(level);*/
      break;
      
    default:
      queue_msg("There is nothing to interface with there.");
      return false;
  }

  if (source)
  {
    if (stack_size(source) == 1)
      del_item(source);
    else
      del_item(split_stack(source, 1));
  }

  update_level_memory(level);
  draw_level();

  clear_msgbar();

  return true;
} /* interface */



/*
 */
blean_t flash_spell(creature_t * caster, item_t * source, signed int param)
{
  unsigned int y;
  unsigned int x;
  creature_t * target;
  unsigned int affected;
  level_t * level;
  blean_t message;
  area_t * area;
  area_t * temp;
  
  if (caster == NULL || (level = caster->location) == NULL)
    return false;

  message = false;

  if (is_player(caster))
  {
    if (is_blinded(caster) == false)
    {
      queue_msg("You emit a bright flash!");
      message = true;
    }
  }
  else if (can_see_creature(game->player, caster))
  {
    msg_the(caster, "emits a bright flash!");
    message = true;
  }

  area = new_area(caster->y, caster->x);

  for (y = 0; y < level->size_y; y++)
  {
    for (x = 0; x < level->size_x; x++)
    {
      if (can_see(caster, y, x) &&
	  (is_flyable(level, y, x) ||
	   find_creature(level, y, x)))
      {
	grow_area(area, y, x);
      }
    }
  }
  
  affected = area_of_effect(game->player, area, missile_flash);

  if (affected && message == false)
  {
    queue_msg("You see a bright flash!");
    message = true;
  }

  for (temp = area->next; temp != NULL; temp = temp->next)
  {
    target = find_creature(level, temp->y, temp->x);

    if (target)
    {
      if (can_see_creature(game->player, target))
	msg_the(target, "is blinded!");
      
      prolong_effect(target, effect_blindness, 3 + tslrnd() % 4);
    }
  }

  if (message)
  {
    msgflush_wait();
    clear_msgbar();
  }

  del_area(area);

  return true;
} /* flash_spell */



blean_t hide(creature_t * creature, item_t * source, signed int param)
{
  if (creature == NULL)
    return false;

  if (is_player(creature))
    queue_msg("You sink into the shadows.");

  prolong_effect(creature, effect_hide, HIDE_DURATION);

  return false;
} /* hide */
