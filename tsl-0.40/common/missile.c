#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <ctype.h>
#include <stdio.h>

#include "main.h"
#include "missile.h"
#include "stuff.h"
#include "combat.h"
#include "effect.h"
#include "item.h"
#include "level.h"
#include "ui.h"
#include "game.h"
#include "inventory.h"
#include "creature.h"
#include "player.h"
#include "actions.h"
#include "fov.h"
#include "elements.h"
#include "losegame.h"
#include "ai.h"
#include "explode.h"
#include "poison.h"
#include "wounds.h"
#include "sleep.h"
#include "potions.h"
#include "gore.h"
#include "memory.h"
#include "traps.h"
#include "find.h"
#include "craft.h"



/*
  Returns the maximum damage ATTACKER can deal with a WEAPON and
  MISSILE. WEAPON may be null, MISSILE is then assumed to be
  thrown. If non-null, MISSILE was launched with WEAPON.
*/
unsigned int get_missile_damage(const creature_t * attacker,
				const item_t * weapon,
				const item_t * missile)
{
  signed int ret;
  
  if ((attacker == NULL) ||
      (missile == NULL))
    return 0;

  ret = 0;

  /* Add the base damage of the missile */
  ret += get_min_damage(missile);

  ret = MAX(0, ret);

  return ret;
} /* get_missile_damage */



/*
  Fires MISSILE with WEAPON from CREATURE in direction DIR.
*/
void fire_missile(creature_t * creature, dir_t dir,
		  item_t * weapon, item_t * missile,
		  blean_t destroy_missile, unsigned int range)
{
  signed int y;
  signed int x;
  signed int move_y;
  signed int move_x;
  /*unsigned int i;*/
  level_t * level;
  char line[80];
  char * temp;
  char missile_name[40];
  char verb[40];
  blean_t attacker_seen;
  blean_t missile_seen;
  
  if (creature == NULL ||
      (level = creature->location) == NULL ||
      missile == NULL ||
      dir == dir_none)
  {
    return;
  }

  dir_to_speed(dir, &move_y, &move_x);

  /* TODO: Move this to its own function. */
  if (weapon != NULL &&
      missile->item_number == treasure_s_buckshot)
  {
    set_item_single_name(missile, "buckshot", "buckshot");
    missile->single_id_article = article_i_some_is;
    missile->single_unid_article = article_i_some_is;
  }

  /*
    Get the item name and store it in a local char array, so we won't
    have to care about releasing it later.
  */
  temp = get_item_name(missile);
  strncpy(missile_name, temp, 39);
  free(temp);
  temp = NULL;

  /* The missile will travel from the creatures current coordinates. */
  y = creature->y;
  x = creature->x;
  
  /*
    Figure out what is going on. If we have a weapon we're "firing",
    we "shoot" spells and "throw" anything else.
  */
  if (weapon != NULL)
    strcpy(verb, "fire");
  else if (missile->item_type == item_type_spell)
    strcpy(verb, "shoot");
  else
    strcpy(verb, "throw");
 
  /* Firearm ammo is always destroyed when fired from a weapon. */
  if (weapon)
  {
    if ((missile->custom[CUSTOM_AMMO_ATYPE] == ammo_type_shell) ||
	(missile->custom[CUSTOM_AMMO_ATYPE] == ammo_type_bullet))
    {
      destroy_missile = true;
    }
  }
  
  /* Potions will always shatter */
  if (missile->item_type == item_type_potion)
    destroy_missile = true;

  /* Can we see who launched it? */
  attacker_seen = can_see_creature(game->player, creature);

  /*
    If missile_seen is false and the missile gets within sight, a
    message will be printed.
  */
  if (is_player(creature) ||
      (attacker_seen && creature->detected))
  {
    /*
      We either launched it ourselves or saw who did - we're aware of
      this missile.
    */
    missile_seen = true;
  }
  else
  {
    /* We're not aware of this missile until we see (or get hit by) it. */
    missile_seen = false;
  }

  /* Display who did what... */
  if (is_player(creature))
  {
    sprintf(line, "You %s %s!", verb, missile_name);
    upperfirst(line);
    queue_msg(line);
  }
  else if (attacker_seen)
  {
    if (creature->detected)
    {
      sprintf(line, "%s %ss %s!",
	      creature->name_one,
	      verb, missile_name);
      upperfirst(line);
    }
    else
      sprintf(line, "Something %ss %s!", verb, missile_name);

    queue_msg(line);
  }

  /*
    Move the missile until we get out of range or hit something.
  */

  while (range-- > 0)
  {
    y += move_y;
    x += move_x;
    
    if (on_map(level, y, x) == false)
      break;

    /* Did we hit anything? */
    if (is_wall(level, y, x) &&
	(missile->ricochets))
    {
      if (is_wall(level, y, x - move_x) == false)
      {
	move_x *= -1;
	x += move_x;
      }
      else if (is_wall(level, y - move_y, x) == false)
      {
	move_y *= -1;
	y += move_y;
      }
    }

    if (is_flyable(level, y, x) == false)
    {
      if (missile_hit(level, creature, weapon, missile,
		      y, x, move_y, move_x, destroy_missile,
		      NULL, NULL) == true)
      {
	msgflush_wait();
	clear_msgbar();
  	return;
      }
    }

    /* We didn't hit anything - the missile keeps travelling. */
    
    /* RFE: I'm not sure both of these could happen. Investigate? */
    if (can_see(game->player, y, x) &&
	is_blinded(game->player) == false)
    {
      /* If we haven't seen the missile previously... */
      if (missile_seen == false)
      {
	sprintf(line, "You see %s!", missile_name);
	queue_msg(line);
      }
      
      /* In either case, we have seen it *now*. */
      missile_seen = true;
	
      /* Display the missile on the map. */
      if (missile->item_type == item_type_spell)
	draw_missile(y, x, missile_spell, move_y, move_x);
      else
	draw_missile(y, x, missile_arrow, move_y, move_x);
    } /* can_see */
  } /* while */

  /* The missile is out of range. */
  
  /* If the missile just "lands" on the floor without hitting anything */
  if (missile->item_number == treasure_w_frost_ray)
    expose_tile_to_cold(level, y, x);

/*  if (can_see(player, y, x) == false)
  {
    if (missile_seen)
    {
      sprintf(line, "%s disappears out of sight.", missile_name);
      upperfirst(line);
      queue_msg(line);
      }
  }
  else if (creature == player)
    queue_msg("You miss!");
  else if (attacker_seen)
  {
    sprintf(line, "%s%s misses!",
	    get_creature_article(creature),
	    creature->name);
    upperfirst(line);
    queue_msg(line);
    }*/

  /*
    Some missiles (spells, etc) must always be destroyed instead of
    landing on the floor.
  */
  /* RFE: Grenades should probably explode instead. */
  if (missile->item_number == treasure_grenade)
  {
    /* Chainsaw ogres always "fire" their grenades */
    if (weapon || creature->id == monster_chainsaw_ogre)
      explosion(level, creature, NULL, y, x, expl_fired, explosive(missile), NULL, NULL);
    else
      explosion(level, creature, NULL, y, x, expl_thrown, explosive(missile), NULL, NULL);
    
    del_item(missile);
  }
  else if (destroy_missile)
  {
    if (missile->item_type == item_type_potion)
    {
      broken_glass(level, y, x);
      
      if (can_see(game->player, y, x))
	msg_glue(missile_name, "shatters!");
    }
    
    del_item(missile);
  }
  else
  {
    /* Put it on the floor... */
    attach_item_to_level(level, missile);
    place_item(missile, y, x);
    floor_effect(missile);

    /* Make a sound */
    /*  draw_attention(level, y, x, 3);*/
  }

  msgflush_wait();
  clear_msgbar();

  return;
} /* fire_missile */



/*
  Returns: false if the missile should continue travelling, true if it
  hit something.

  If CASUALTY is set (non-null), it will be set to true if WATCH is
  killed by the missile, otherwise false.
*/
blean_t missile_hit(level_t * level, creature_t * creature,
		    item_t * weapon, item_t * missile,
		    const unsigned int y, const unsigned int x,
		    const signed int move_y, const signed int move_x,
		    const blean_t destroy_missile,
		    const creature_t * watch, blean_t * casualty)
{
  char line[100];
  char missile_name[40];
  char * temp;
  creature_t * target;
  blean_t seen;
  unsigned int damage_taken;
  unsigned int damage_to_deal;
  blean_t poisoned;
  blean_t wounded;
  /*blean_t sedated;*/
  damage_type_t damage_type;
  signed int dodge;
  signed int attack_skill;
  unsigned int old_number;

  if (level == NULL || missile == NULL)
    return false;

  if (casualty != NULL)
    casualty = false;

  poisoned = false;
  wounded = false;
/*  sedated = false;*/
  
  /*
    If there's nothing there that would block the missiles path,
    missile_hit() should never have been called.
  */
  if (is_flyable(level, y, x))
  {
    queue_msg("BUG: missile_hit() called, but there was nothing there.");
    return false;
  }

  /*
    Get the item name and store it in a local char array, so we won't
    have to care about releasing it later.
  */
  temp = get_item_name(missile);

  if (is_player(creature))
  {
    strcpy(missile_name, "your");
    strncat(missile_name, strchr(temp, ' '), 30);
  }
  else
  {
    strncpy(missile_name, temp, 39);
  }

  free(temp);
  temp = NULL;

  /* This shouldn't occur outside the map. */
  if (on_map(level, y, x) == false)
    return false;
  
  /* Can we see what's going on? */
  
  /* RFE: Can both of these really happen? */
  if (can_see(game->player, y, x) &&
      is_blinded(game->player) == false)
  {
    seen = true;
  }
  else
  {
    seen = false;
  }

  /* We check this first so grenades don't explode when hitting forcefields. */
  if (get_tile(level, y, x) == tile_forcefield)
  {
    del_item(missile);

    if (can_see(game->player, y, x))
      msg_glue(missile_name, "disintegrates in the forcefield!");

    return true;
  }

  /* Is anyone there? */
  target = find_creature(level, y, x);

  /* If we've hit an obstacle and the missile is explosive. */
  if (is_flyable(level, y, x) == false &&
      (damage_to_deal = explosive(missile)))
  {
    /*
      RFE: I'm not sure how to do this. In what cases should we
      display a message?
    */
    
    if (seen)
      msg_glue(missile_name, "explodes!");

    old_number = missile->item_number;

    del_item(missile);

    if (old_number == treasure_fireball)
      explosion(level, creature, NULL, y, x, expl_fireball, damage_to_deal, NULL, NULL);
    else if (old_number == treasure_grenade)
    {
      /* Chainsaw ogres always "fire" their grenades */
      if (weapon || creature->id == monster_chainsaw_ogre)
	explosion(level, creature, NULL, y, x, expl_fired, damage_to_deal, NULL, NULL);
      else
	explosion(level, creature, NULL, y, x, expl_thrown, damage_to_deal, NULL, NULL);
    }
    else
      explosion(level, creature, NULL, y, x, expl_thrown, damage_to_deal, NULL, NULL);

    /* In case the explosion destroyed generators */
    update_level_memory(level);
    draw_level();

    return true;
  }
  else if (target == NULL &&
	   is_flyable(level, y, x) == false)
  {
    /*
      The missile can't continue in this direction, and there was no
      one there to hit. Let's assume it was a wall. Place the missile
      on the floor (this will be one step _before_ the wall, so we
      subtract move_y/move_x). Some missiles (spells) should always be
      destroyed instead of landing on the floor.
    */
    if (destroy_missile)
    {
      /* Potions are destroyed and leave a glass trap if the terrain permits. */
      if (missile->item_type == item_type_potion)
      {
	broken_glass(level, y - move_y, x - move_x);
	
	if (can_see(game->player, y - move_y, x - move_x))
	  msg_glue(missile_name, "shatters!");
      }

      del_item(missile);
    }
    else
    {
      /* It should land on the floor. */

      if (seen == true)
      {
/*	if (on_map(level, y, x) == false)
	  msg_glue(missile_name, "bounces off the edge of the universe.");
	else if (get_tile(level, y, x) == tile_door_closed)
	  msg_glue(missile_name, "bounces off a door.");
	else if (get_tile(level, y, x) == tile_capsule)
	  msg_glue(missile_name, "bounces off a capsule.");
	else
	  msg_glue(missile_name, "bounces off a wall.");*/

	msg_glue(missile_name, "bounces off.");
      }

      /* Make a sound */
      draw_attention(level, y, x, noise_loud);
      
      /* Place the missile on the floor, *before* the wall */
      attach_item_to_level(level, missile);
      place_item(missile, y - move_y, x - move_x);
      floor_effect(missile);
    }
    
    msgflush_wait();
    clear_msgbar();
    
    return true;
  }
  else if (target != NULL)
  {
    /* There was someone there. */

    reveal_mimic(target);
    
    if (target->detected == false)
    {
      if (can_see_creature(game->player, target))
      {
	target->detected = true;
	msg_one(target, "is there!");
      }
    }

    /* Can we see the creature that was hit? */

    /* TODO: Shouldn't this reveal ALL creatures? */

    if (can_see_creature(game->player, target) &&
	target->detected &&
	is_blinded(game->player) == false)
    {
      seen = true;
    }

    /* Get a hit roll for the missile. */

    /* Who launched this missile? */
    if (creature == NULL)
    {
      /* No one launched it; it was probably a trap. */
      attack_skill = TRAP_MISSILE_SKILL;
    }
    else
    {
      /*
	It was a creature. For spells, we'll just leave it up to the
	magic code to decide hit chances. For regular weapons, we
	include creature attack, weapon precision, missile
	precision.
      */

      if (missile->item_type == item_type_potion && is_player(creature))
      {
	attack_skill = 98;
      }
      else if (missile->item_type == item_type_spell)
      {
	attack_skill = 98;
      }
      else if (missile->item_type != item_type_spell)
      {
	attack_skill = attr_current(creature, attr_attack);
	attack_skill += get_item_mod(missile, attr_i_precision);
	
	/* Is there is a weapon involved? Apply its precision modifier. */
	if (weapon != NULL)
	  attack_skill += get_item_mod(weapon, attr_i_precision);
      }
    }

    /* Did the missile hit? */
    if (tslrnd() % 100 >= attack_skill)
    {
      /* It was a miss - the missile keeps travelling. */
      
      if (is_player(target))
      {
	msg_glue((seen ? missile_name : "something"), "flies by!");
      }
      else if (seen)
      {
	sprintf(line, "%s misses %s!", missile_name, target->name_one);
	upperfirst(line);
	queue_msg(line);
      }
      
      return false;
    }

    /* Give the creature a chance to dodge the missile */
    if (is_awake(target))
    {
      dodge = attr_current(target, attr_dodge);
    
      if (tslrnd() % 100 < dodge)
      {
	/* Target dodged - the missile keeps travelling. */
	
	if (is_player(target))
	{
	  sprintf(line, "You dodge %s!", (seen ? missile_name : "something"));
	  queue_msg(line);
	}
	else if (seen)
	{
	  sprintf(line, "%s dodges %s!", target->name_one, missile_name);
	  upperfirst(line);
	  queue_msg(line);
	}
	
	return false;
      }
    }

    /* It was a hit! */
    
    /* Make a sound. */

    /* We want to exclude some items or they will wake the target up before putting it to sleep again. */
    if (missile->item_number != treasure_d_tranq &&
	missile->item_number != treasure_p_sleep)
    {
      draw_attention(level, y, x, 3);
    }
    
    if (is_player(target))
    {
      /* The player was hit. Did we see by what? */
      sprintf(line, "You are hit by %s!", (seen ? missile_name : "something"));
      queue_msg(line);

      break_rest(target);
    }
    else if (seen == true)
    {
      /* We saw the target being hit. */
      sprintf(line, "is hit by %s!", missile_name);
      msg_the(target, line);
    }
      
    if (is_player(target) == false)
    {
      aggravate(target);
/*      draw_attention(creature->location, creature->y, creature->x, 7);
	alert_enemy(target);*/
      target->target_y = game->player->y;
      target->target_x = game->player->x;
    }
    
    /* Time to make the item take effect */

    if (missile->item_number == treasure_d_tranq)
    {
      tranquilize(target, missile);
      damage_to_deal = damage_taken = 0;
    }
    else if (missile->item_type == item_type_potion)
    {
      /* Potions have the same effect as if they had been quaffed. */
      if (apply_potion(target, missile, creature) == false)
      {
	/* The creature survived. */
	
	/* Wake the creature up unless it was a potion of sleep. */
	if (missile->item_number != treasure_p_sleep)
	  creature_sleep(target, false);
      }
    }
    else
    {
      /* RFE: Other throwable objects go here */
      damage_type = damage_general;
      damage_to_deal = random_damage(creature, weapon, missile);
      damage_to_deal = damage_armor(target, damage_to_deal, damage_type);
      damage_taken = damage(target, damage_to_deal);
      
      /* Poison */
      if (get_item_mod(missile, attr_i_poison))
      {
	if (poison_creature(target, MISSILE_POISON))
	  poisoned = true;
      }
      
      /* Wounding */
      if (get_item_mod(missile, attr_i_wound))
      {
	if (wound_creature(target, MISSILE_WOUND_TIME))
	  wounded = true;
      }
      
      if (damage_taken >= 0)
      {
	if (killed(target))
	{
	  if (is_player(target))
	  {
	    /* We've been killed. */
	    queue_msg("You die...");
	    
	    /* If it was a creature that killed us, say so... */
	    if (creature != NULL)
	    {
	      sprintf(line, "killed by %ss %s",
		      creature->name_one,
		      missile->single_id_name);
	    }
	    else
	    {
	      /* ... otherwise use the missile as the cause of death. */
	      sprintf(line, "were killed by %s", missile_name);
	    }
	    
	    /* We need to do this here or the missile won't be freed properly. */
	    del_item(missile);
	    
	    /* Goodbye. */
	    check_for_player_death(line);
	  }
	  else
	  {
	    /* It was an NPC that was hit. Is this the one we're watching? */
	    if ((target == watch) && (casualty != NULL))
	      *casualty = true;

	    add_anim(anim_type_damage, target->uid, damage_taken, target->y, target->x);
	    
	    creature_death(target, creature, missile);
	    
	    /* We no longer wish to display these messages. */
	    poisoned = false;
	    wounded = false;
 	  }
	} /* killed */
	else
	{
	  /* The creature survived. */
	  creature_sleep(target, false);
	  
	  add_anim(anim_type_damage, target->uid, damage_taken, target->y, target->x);
	    
	  if (can_see_creature(game->player, target))
	  {
	    if (poisoned)
	    {
	      if (is_player(target))
		queue_msg("You have been poisoned!");
	      else
		msg_one(target, "is poisoned!");

	      make_item_known(missile);
	    }
	    
	    if (wounded)
	    {
	      if (is_player(target))
		queue_msg("You have been wounded!");
	      else
		msg_one(target, "is wounded!");
	    }
	  } /* can see*/
	} /* survived */
      }
    } /* item type */

    /* We no longer need this. */
    del_item(missile);
    missile = NULL;
  } /* target */

  return true;
} /* missile_hit */



/*
  Returns the maximum range for CASTER when casting SPELL.
*/
unsigned int get_spell_range(creature_t * caster, const attr_index_t spell)
{
  if (caster == NULL)
    return 0;

  switch (spell)
  {
    case attr_m_force_bolt:
      return FORCE_BOLT_RANGE;

    case attr_m_mudball:
      return 4;

    case attr_m_breathe_fire:
      return 3;

    case attr_m_noxious_breath:
      return 4;

    case attr_m_frost_ray:
      return FROST_RAY_RANGE;
      
    case attr_m_fireball:
      return 3;
      
    default:
      return 0;
  }
} /* get_spell_range */
