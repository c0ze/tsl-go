#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include "main.h"
#include "stuff.h"
#include "rules.h"
#include "game.h"
#include "ui.h"
#include "monster.h"
#include "level.h"
#include "player.h"
#include "effect.h"
#include "inventory.h"
#include "treasure.h"
#include "creature.h"
#include "actions.h"
#include "magic.h"
#include "ai.h"
#include "traps.h"
#include "places.h"
#include "shapeshf.h"
#include "fov.h"
#include "altitude.h"
#include "losegame.h"
#include "ability.h"
#include "poison.h"
#include "facet.h"
#include "sleep.h"
#include "swimming.h"
#include "burdened.h"
#include "elements.h"
#include "find.h"
#include "equip.h"
#include "ffield.h"



/*
  This should be set to true whenever we change level, so we won't
  run outside the creature array.
*/
blean_t level_changed;

/*
  Main game loop. This is where it all comes together!
*/
void play()
{
  int i;
  creature_t * creature;
  effect_t * eff;

  tick = 0;

  /*
    This ensures the player always gets to move first at the start of
    the game. It also ensures the player gets to move first after
    reloading a saved game, so no time may pass - saving is a free
    action.
  */
  for (i = 0; i < get_current_level()->creatures; i++)
  {
    creature = get_current_level()->creature[i];

    if (is_player(creature))
    {
      goto creature_turn;
    }
  }

  /*
    This is the main game loop that we'll never leave. Each iteration
    of this while represents one "tick".
  */
  while (1)
  {
    tick++;

    respawn(get_current_level());

    /* Loop through each creature on the level */
    for (i = 0; i < get_current_level()->creatures; i++)
    {
      if (level_changed)
      {
	/*
	  If we've changed level, we must restart the creature[]
	  iteration. The new level could have more enemies than the
	  old and we risk running outside the array.
	*/
	level_changed = false;
	break;
      }

      creature = get_current_level()->creature[i];
      
      if (creature == NULL)
	continue;
      
      /*
	This could kill the creature, so we have to check after that
	it's still around...
      */
      increment_counters(creature);

      if (get_current_level()->creature[i] != creature)
	continue;

      /* Has this creature accumulated enough time to perform a move? */
      if (creature->move_counter < TURN_TIME)
	continue; /* No - proceed to the next creature. */

      /* Ok, let's start a turn. */
      creature->move_counter -= TURN_TIME;

      /*
	If this creature has a lifetime set, decrease it and see if
	the creature "timed out". A shapeshifted player will revert to
	its native shape, while NPCs will just "disappear".
      */
      if (creature->lifetime > 0)
      {
	creature->lifetime--;
	
	if (creature->lifetime == 0)
	{
	  if (is_player(creature) &&
	      player_shapeshifted())
	  {
	    shapeshift_native(game->player, NULL, 0);
	  }
	  else
	  {
	    if (can_see(game->player, creature->y, creature->x))
	      msg_one(creature, "disappears.");
	    
	    del_creature(creature);
	  }

	  /*
	    Since we've messed up the creature array, move to the
	    next creature.
	  */
	  continue;
	}
      }

      /*
	Decrement the remaining levitation time. This could be
	dangerous - it's possible the creature might be killed
	(landing in lava, landing on a lethal trap, etc).
      */
      decrement_levitation(creature);

      /*
	This hack should ensure we won't end up with an invalid
	creature pointer - if the creatures slot has changed after
	decrementing levitation, the creature was possibly killed and
	we can't trust the pointer anymore. We must start over.
      */
      if (get_current_level()->creature[i] != creature)
	continue;

      /*
	Now we'll perform some things that need to be done for every
	creature every turn.
      */
      pass_time_on_effects(creature);
      energy(creature);
      decrement_light_source_lifetime(creature);
/*      decrement_magic_weapon_lifetime(creature);*/

      /* Use passive powers. */
      try_to_invoke_ability(creature, attr_dissolve);
	
      /* Check if we're asleep and if we should wake up. */
      eff = get_effect_by_id(creature, effect_sleep);

      if (eff)
      {
	if (eff->ttl <= 1)
	  creature_sleep(creature, false);
	else
	{
	  eff->ttl--;
	  continue;
	}
      }

      /* Perform multi-actions */
      if (creature->multiaction == MULTIACTION_AIM)
      {
	aimed_shot_release(creature, NULL, 0);
	creature->multiaction = MULTIACTION_NONE;
	continue;
      }

      /* Is the creature stunned? */
      eff = get_effect_by_id(creature, effect_stun);
      
      if (eff && is_player(creature) == false)
      {
	/* Completely disables monsters */
	creature->move_counter -= TURN_TIME;
	continue;
      }

      /*
	This must be done before we give the player the initiative, or
	we will reset their damage sequence!
      */
      advance_pos(creature);

    creature_turn: /* See start of function. */

      if (is_player(creature))
      {
	scan_monsters(creature);
	player_control(creature);
      }
      else
	basic_ai(creature);

      /* 
	 This will have changed if the player used stairs or
	 level-teleported. We can't proceed since the creature array
	 might be invalid.
      */
      if (level_changed)
	break;
      
      /* turn_func might have killed the creature... */
      if (get_current_level()->creature[i] != creature)
	continue;
      
      if (swim(creature))
	continue;

      /*
	This turn is over. If the creature got itself killed
	during its turn, the pointer could be invalid by now!
      */
    } /* for each creature */
  } /* main loop */
  
  return;
} /* play */



/*
*/
void advance_pos(creature_t * creature)
{
  effect_t * temp_weapon;
  item_t * weapon[2];
  
  creature->attack_pos = creature->next_attack;
  creature->next_attack = 0;

  temp_weapon = get_effect_by_id(creature, effect_temp_weapon);
  
  if (temp_weapon != NULL)
  {
    weapon[0] = weapon[1] = virtual_weapon[temp_weapon->param[EFFECT_VWEAPON_INDEX]];
  }
  else
  {
    weapon[0] = get_equipped(creature, item_type_m_weapon, 0);
    weapon[1] = get_equipped(creature, item_type_m_weapon, 1);

    if (weapon[0] == NULL)
      weapon[0] = virtual_weapon[creature->unarmed];

    if (weapon[1] == NULL)
      weapon[1] = weapon[0];
  }

  if (creature->attack_pos < 0)
  {
    creature->attack_pos = 0;
  }

  if ((weapon[0]->custom[WEAPON_ATKSEQ + creature->attack_pos] == 0) &&
      (weapon[1]->custom[WEAPON_ATKSEQ + creature->attack_pos] == 0))
  {
    creature->attack_pos = 3;
  }

  while (creature->attack_pos < 3)
  {
    if ((weapon[0]->custom[WEAPON_ATKSEQ + creature->attack_pos] == -2) &&
	(weapon[1]->custom[WEAPON_ATKSEQ + creature->attack_pos] == -2))
    {
      creature->attack_pos++;
    }
    else
    {
      creature->next_attack = creature->attack_pos + 1;
      break;
    }
  }

  if ((weapon[0]->custom[WEAPON_ATKSEQ + creature->attack_pos] == -1) ||
      (weapon[1]->custom[WEAPON_ATKSEQ + creature->attack_pos] == -1))
  {
    /*creature->attack_pos++;*/
    creature->next_attack = creature->attack_pos + 1;
  }

  if (creature->attack_pos == 3)
    creature->next_attack = 3;
  else if (creature->attack_pos > 3)
    creature->next_attack = 0;

  return;
} /* advance_pos */



/*
 */
void reset_seq(creature_t * creature)
{
  if (creature == NULL)
    return;

  creature->attack_pos = 0;
  creature->next_attack = 0;
  advance_pos(creature);

  return;
} /* reset_seq */



/*
  Starts a new game. This includes setting up some basic game state
  variables, creating the player character, placing it in the proper
  starting area, etc.
*/
game_t * start_new_game()
{
  unsigned int i;

  game_t * new_game;

  new_game = malloc(sizeof(game_t));

  if (new_game == NULL)
    out_of_memory();

  mem_alloc.games++;

  for (i = 0; i < LEVELS; i++)
    new_game->level_list[i] = NULL;

  /* Clear the ability slots. */
  for (i = 0; i < 10; i++)
    new_game->ability_slot[i] = -1;

  new_game->turns = 0;
  new_game->game_over = false;
  new_game->won = false;

  new_game->first_facet = define_facet(facet_spellbook);

  /* The player doesn't know about any items or monsters */
  for (i = 0; i < ITEMS; i++)
    new_game->item_known[i] = false;

  for (i = 0; i < monsters; i++)
    new_game->monster_known[i] = false;

  /* Below are things that work on a per-game basis (somewhat random, etc) */
  clear_uniqitems(new_game);
  randomize_facets(new_game);
  init_item_templates(new_game);

  return new_game;
} /* start_new_game */



void end_game(game_t * game_to_end)
{
  unsigned int i;
  item_t * temp;
  item_t * next;

  if (game_to_end == NULL)
    return;

  /* Remove careers */
  temp = game_to_end->first_facet;

  while (temp != NULL)
  {
    next = temp->next_item;
    del_item(temp);
    temp = next;
  }

  /* If the player is shapeshifted, make sure to restore it. */
  if ((game_to_end->native != game_to_end->player) &&
      (game_to_end->native != NULL))
  {
    del_creature(game_to_end->player);
    game_to_end->player = game_to_end->native;
    del_creature(game_to_end->native);
  }

  del_item_templates(game_to_end);

  for (i = 0; i < LEVELS; i++)
    del_level(game_to_end->level_list[i]);
  
  free(game_to_end);

  mem_alloc.games--;

  return;
} /* end_game */



/*
  Moves the player character to level index LEVEL. If the level does
  not exist, nothing happens. If no map has been generated yet, it
  will be now.
*/
void change_level(const unsigned int level_index)
{
  if ((level_index >= LEVELS) ||
      (game->level_list[level_index] == NULL))
  {
    return;
  }
  
  detach_creature(game->player);
  
  if (game->level_list[level_index]->map == NULL)
    build_dungeon(game->level_list[level_index]);

  if (attach_creature(game->level_list[level_index], game->player) != NULL)
  {
    del_creature(game->player);
    shutdown_everything();
    printf("Fatal error: Couldn't reattach player to new level!\n");
    exit(4);
  }

  /* Display & clear level message. */

  if (get_current_level()->message != NULL)
  {
    queue_msg(get_current_level()->message);
    set_level_message(get_current_level(), NULL);
  }

  /* Clear level-specific effects */
  clear_level_effects(game->player);

  build_forcefields(get_current_level());

  level_changed = true;

  return;
} /* change_level */



void energy(creature_t * creature)
{
  int rate;
  int needed;
  
  if (creature == NULL)
    return;
  
  if (attr_base(creature, attr_ep_current) >= attr_base(creature, attr_ep_max))
  {
    creature->ep_counter = 0;
    return;
  }

  rate = attr_current(creature, attr_magic);
  needed = MAX(1, 11 - rate);
  
  creature->ep_counter++;

  if (creature->ep_counter >= needed)
  {
    regain_ep(creature, MAX(1, rate - 9));
    creature->ep_counter = 0;
  }
  
  return;
} /* energy */



/*
  Counts down on all effects CREATURE currently has. Effects that time
  out will be removed and (if it's the player character) a suitable
  timeout message will be displayed.
*/
void pass_time_on_effects(creature_t * creature)
{
  effect_t * temp_effect;
  effect_t * effect;
  effect_id_t id;
  blean_t redraw;

  if (creature == NULL)
    return;

  redraw = false;

  effect = creature->first_effect;
  
  while (effect != NULL)
  {
    id = effect->id;

    /* Should we count down on this effect? */
    if (effect->countdown)
    {
      /* Has the effect expired? */
      if (effect->ttl <= 1)
      {
	/*
	  This is in case we were blinded somehow, but put on a
	  blindfold or similar before the effect timed out.
	*/
	if (id == effect_blindness && is_blinded(creature))
	{ /* Do nothing */ }
	else
	  msg_expire(effect);

	/* Remove the effect. */
	temp_effect = effect->next_effect;
	effect_expire(effect);
	effect = temp_effect;
	
	/* Is it a disabled forcefield that should turn on? */
	if (id == effect_disable)
	{
	  build_forcefields(creature->location);
	  set_creature_coordinates(creature, creature->y, creature->x);
	  redraw = true;
	}
	
	/* Did it affect our vision? */
	if (id == effect_blindness)
	{
	  /* Quickest way to regenerate FOV. */
	  set_creature_coordinates(creature, creature->y, creature->x);
	  
	  if (is_player(creature))
	    redraw = true;
	}

	/* Resume the loop with the next effect. */
	continue;
      }
      else
	effect->ttl--;
    }
      
    effect = effect->next_effect;
  } /* for effect */

  if (redraw)
    draw_level();

  return;
} /* pass_time_on_effects */



/*
  Increments (or decrements...) CREATUREs HP, EP, move, etc, counter.
*/
void increment_counters(creature_t * creature)
{
  unsigned int recovery;
  unsigned int current_speed;

  if (creature == NULL)
    return;

  current_speed = attr_current(creature, attr_speed);
  
  if (is_burdened(creature))
    current_speed = MAX(1, current_speed * BURDENED_FACTOR);

  creature->move_counter += current_speed;

  /* Fatigue */
  recovery = attr_current(creature, attr_recovery);
  creature->fatigue_counter += recovery;
  
  while (creature->fatigue_counter >= FATIGUE_RECOVERY_TIME)
  {
    creature->fatigue_counter -= FATIGUE_RECOVERY_TIME;
    heal(creature, 1);
  }

  /* Poison */
  if (poison_immunity(creature) ||
      attr_current(creature, attr_poisoned) == 0)
  {
    /* We're not poisoned or immune to poison; reset the poison counter. */
    creature->poison_counter = 0;
  }
  else
  {
    /* We're poisoned; increase the counter. */
    creature->poison_counter +=
      MAX(1, POISON_IMMUNITY - attr_current(creature, attr_poison_resistance))
      * (is_player(creature) ? 1 : ENEMY_POISON_FACTOR);
  }
  
  while (creature->poison_counter >= POISON_TIME)
  {
    creature->poison_counter -= POISON_TIME;
    poison_damage(creature);
  }

  return;
} /* increase_counters */





/*
  Counts down the remaining illumination time of CREATUREs light
  source (if present). If it expires, the item will be destroyed and
  (if it's the player character) a suitable message will be
  displayed. The light matrix of the creatures level will also be
  rebuilt.
 */
void decrement_light_source_lifetime(creature_t * creature)
{
  char line[80];
  char item_name[40];
  item_t * item;
  unsigned int weight;
  
  if (creature == NULL)
    return;

  item = get_equipped(creature, item_type_light, 0);
  
  /* If we don't have any light, there's no point in continuing. */
  if (item == NULL)
    return;

  /* -1 ticks remaining means it's an infinite lightsource. */
  if (item->custom[CUSTOM_LIGHT_TICKS] == -1)
    return;
  
  /* It's a normal light source; decrement the remaining time. */
  if (identified(item) & known_name)
    strncpy(item_name, item->single_id_name, 39);
  else
    strncpy(item_name, item->single_unid_name, 39);

  if (is_player(creature)
      && (item->custom[CUSTOM_LIGHT_TICKS] == WARNING_FLICKERS)
      && is_blinded(creature) == false)
  {
    sprintf(line, "Your %s flickers!", item_name);
    queue_msg(line);
    msgflush_wait();
    clear_msgbar();
    break_rest(creature);
  }
  
  if (is_player(creature)
      && (item->custom[CUSTOM_LIGHT_TICKS] == WARNING_GO_OUT)
      && is_blinded(creature) == false)
  {
    sprintf(line, "Your %s is about to go out!", item_name);
    queue_msg(line);
    msgflush_wait();
    clear_msgbar();
    break_rest(creature);
  }
  
  if (item->custom[CUSTOM_LIGHT_TICKS] > 1)
    item->custom[CUSTOM_LIGHT_TICKS]--;
  else
  {
    /* Light source has gone out. */

    weight = item->weight;
    del_item(item);
    build_fov(creature);
    draw_level();

    /*
      This will be displayed even if player is blinded. Right or
      wrong? I think it's fair to tell the player they've lost an
      item, even if they can't see it happening.
    */
    if (is_player(creature))
    {
      sprintf(line, "Your %s has gone out!", item_name);
      queue_msg(line);
      msgflush_wait();
      clear_msgbar();
      unburdened(creature, weight);
      break_rest(creature);
    }
  }
  
  return;
} /* decrement_light_source_lifetime */



void respawn(level_t * level)
{
  int y;
  int x;

  if (level == NULL)
    return;

  level->respawn_counter += level->respawn_speed;

  if (level->respawn_counter > RESPAWN_TIME)
  {
    level->respawn_counter = 0;
    level->respawn_speed += 5;

    find_spot(level, &y, &x, find_obscured);
    add_encounter(level, y, x, encounter_std_enemy);
    
    /*queue_msg("Respawn!");*/
  }

  return;
} /* respawn */



/*
  TODO: This is player-specific code.
*/
void try_to_detect_enemies()
{
  creature_t * observer;
  creature_t * target;
  level_t * level;
  unsigned int i;
  unsigned int stealth;
  char line[200];

  observer = game->player;

  if ((observer == NULL) || (is_player(observer) == false))
    return;

  level = observer->location;

  if (level == NULL)
    return;

  for (i = 0; i < level->creatures; i++)
  {
    target = level->creature[i];

    if (target == NULL || target == observer)
      continue;

    if (can_see_creature(observer, target))
    {
      stealth = attr_base(target, attr_stealth);
      
      if (stealth > 0)
	set_attr(target, attr_stealth, stealth - 1);
      
      if (attr_current(observer, attr_perception) > attr_current(target, attr_stealth))
      {
	if (target->detected == false && stealth)
	{
	  sprintf(line, "You spot %s.", target->name_one);
	  queue_msg(line);
	}
	
	target->detected = true;
      }
    }
  } /* for */

  return;
} /* try_to_detect_enemies */



/*
*/
void try_to_detect_traps()
{
  unsigned int y;
  unsigned int x;
  level_t * level;
  trap_t * temp;
  creature_t * observer;

  observer = game->player;

  if ((observer == NULL) ||
      (is_player(observer) == false))
    return;

  level = observer->location;

  if (level == NULL)
    return;

  for (temp = level->first_trap; temp != NULL; temp = temp->next_trap)
  {
    if (temp->hidden)
      continue;

    y = temp->y;
    x = temp->x;

    if (on_map(level, y, x) == false)
      continue;

    if (can_see(observer, y, x))
    {
      if (attr_current(observer, attr_perception) + 
	  attr_current(observer, attr_s_trap_detection)
	  > temp->difficulty)
      {
	temp->revealed = true;
      }
    }
  }
  
  return;
} /* try_to_detect_traps */



/*
  Moves everyone on and around {OLD_Y, OLD_X} on LEVEL to {NEW_Y,
  NEW_X} on NEW_LEVEL, except mimics that are still mimicing.
*/
void move_everyone(level_t * level, unsigned int old_y,
		   unsigned int old_x, level_t * new_level,
		   unsigned int new_y,unsigned int new_x)
{
  signed int y;
  signed int x;

  creature_t * creature;

  if ((level == NULL) ||
      (on_map(new_level, new_y, new_x) == false))
  {
    return;
  }

  for (y = -1; y <= 1; y++)
  {
    for (x = -1; x <= 1; x++)  
    {
      creature = find_creature(level, old_y + y, old_x + x);

      if (creature == NULL || creature->ai_state == ai_mimic)
	continue;

      detach_creature(creature);

      if (attach_creature(new_level, creature) != NULL)
	del_creature(creature);
      else
	find_nearest_free_spot(creature, new_y, new_x);
    }
  }
} /* move_everyone */



/*
  Decrement CREATUREs remaining time of levitation. This only cares
  about the *base* attribute Levitation. Permanent levitation can be
  achieved by giving an item (or effect) +1 to Levitation; this
  function won't care.

  This isn't implemented like a regular effect since we want to do
  custom things (change_altitude) on expiry.
*/
void decrement_levitation(creature_t * creature)
{
  if (creature == NULL)
    return;

  if (attr_base(creature, attr_levitate) == 0)
    return;

  creature->attr_base[attr_levitate]--;

  if (attr_current(creature, attr_levitate) == 0)
    change_altitude(creature);

  return;
} /* decrement_levitation */



/*
  Returns: The level the player is currently on.
*/
level_t * get_current_level(void)
{
  return game->player->location;
} /* get_current_level */



void scan_monsters(creature_t * observer)
{
  level_t * level;
  int i;

  if (observer == NULL || (level = observer->location) == NULL)
    return;

  for (i = 0; i < level->creatures; i++)
  {
    if (level->creature[i] &&
	level->creature[i] != observer &&
	can_see_creature(observer, level->creature[i]) &&
	level->creature[i]->id < monsters)
    {
      game->monster_known[level->creature[i]->id] = true;
    }
  }

  return;
} /* scan_monsters */
