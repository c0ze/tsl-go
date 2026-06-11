#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <ctype.h>
#include <stdio.h>

#include "main.h"
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
#include "wounds.h"
#include "teleport.h"
#include "sleep.h"
#include "pushing.h"
#include "gore.h"
#include "equip.h"
#include "burdened.h"



/*
  Stores in DEST a description of CREATUREs hand(s), paw(s),
  tentacle(s), etc. If P is true the plural form will be used.
*/
void hand_string(char * dest, const creature_t * creature, const blean_t p)
{
  if (dest == NULL || creature == NULL)
    return;

  switch (creature->id)
  {
    case monster_ratman:
    case monster_graveling:
    case monster_hellhound:
      if (p) strcpy(dest, "paws");
      else   strcpy(dest, "paw");
      break;

    case monster_nameless_horror:
      if (p) strcpy(dest, "tentacles");
      else   strcpy(dest, "tentacle");
      break;

    default:
      if (p) strcpy(dest, "hands");
      else   strcpy(dest, "hand");
      break;
  }

  return;
} /* hand_string */


/*
  Determines what "attack verb" to use for WEAPON, e.g. "you CLAW the
  greedy merchant". There are two forms; one to be used for NPCs and
  one when the player is the attacker. THIRD and SELF must be char
  arrays large enough to hold whatever we put into them.
*/
/*
  RFE: Perhaps we should decide on an exact max length of the
  strings here, to avoid "make it 15 characters or more just to be
  safe"?
*/
void get_attack_strings(const attack_string_t str, char * third, char * self)
{
  switch (str)
  {
    case attack_string_claws:
      strcpy(third, "claws");
      strcpy(self,  "claw");
      break;

    case attack_string_bites:
      strcpy(third, "bites");
      strcpy(self,  "bite");
      break;

    case attack_string_stings:
      strcpy(third, "stings");
      strcpy(self,  "sting");
      break;

    case attack_string_slaps:
      strcpy(third, "slaps");
      strcpy(self,  "slap");
      break;

    case attack_string_slimes:
      strcpy(third, "slimes");
      strcpy(self,  "slime");
      break;

    case attack_string_burns:
      strcpy(third, "burns");
      strcpy(self,  "burn");
      break;

    case attack_string_touches:
      strcpy(third, "touches");
      strcpy(self,  "touch");
      break;

    case attack_string_kicks:
      strcpy(third, "kicks");
      strcpy(self,  "kick");
      break;

    case attack_string_stabs:
      strcpy(third, "stabs");
      strcpy(self,  "stab");
      break;

    case attack_string_punches:
      strcpy(third, "punches");
      strcpy(self,  "punch");
      break;

    case attack_string_saws:
      strcpy(third, "saws");
      strcpy(self,  "saw");
      break;

    case attack_string_whips:
      strcpy(third, "whips");
      strcpy(self,  "whip");
      break;

    case attack_string_hits:
    default:
      strcpy(third, "hits");
      strcpy(self,  "hit");
      break;
  }

  return;
} /* get_attack_strings */



item_t * next_weapon(creature_t * attacker)
{
  struct item_t * best;
  struct item_t * next;
  effect_t * temp_weapon;
  signed int bdam;
  signed int ndam;
/*  unsigned int hand;*/

  if (attacker == NULL)
    return NULL;

  best = NULL;

  /*
    Determine the type of attack used.
  */
  temp_weapon = get_effect_by_id(attacker, effect_temp_weapon);

  if (temp_weapon != NULL)
  {
    best = virtual_weapon[temp_weapon->param[EFFECT_VWEAPON_INDEX]];
  }
  else if (attacker->attack_pos < 3)
  {
    return NULL;
  }
  else
  {
    /* */
    best = get_equipped(attacker, item_type_m_weapon, 0);
    next = get_equipped(attacker, item_type_m_weapon, 1);

    if (next != NULL)
    {
      ndam = weapon_seq(next, attacker->attack_pos);
      bdam = weapon_seq(best, attacker->attack_pos);
      
      if ((ndam > bdam) ||
	  (ndam == bdam && tslrnd() % 2 == 0))
      {
	best = next;
      }
    }
  }

  /*
    If the attacker has no temp weapon and isn't using any regular
    weapon, we'll use the unarmed virtual weapon.
  */
  
  if (best == NULL)
    best = get_unarmed_weapon(attacker);

  return best;
} /* next_weapon */



/*
  Melee between ATTACKER and DEFENDER.

  Returns: TRUE if the attack was carried out (not necessarily if it
  succeeded or did any damage, but that an attempt was made),
  otherwise FALSE.
*/
blean_t attack(creature_t * attacker, creature_t * defender)
{
/*  blean_t defender_killed;*/
  item_t * weapon;

  /* First, do some sanity checks */
  if (attacker == NULL ||
      defender == NULL  ||
      attacker->location == NULL || 
      defender->location == NULL)
  {
    return false;
  }

  /* This is a really stupid situation that we want to avoid. */
  if (attacker == defender)
    return false;

  if (reveal_mimic(defender))
    return true; /* turn wasted! */

  weapon = next_weapon(attacker);
    
  /* This shouldn't occur... */
  if (weapon == NULL)
    queue_msg("BUG: weapon was NULL.");
    
  /* Perform the attack with whatever weapon we ended up with */
  attack_internal(attacker, defender, weapon);

  /*defender_killed =*/

  
  /*
    Was the defender killed (or moved away)? If so, we shouldn't
    continue attacking with any other weapon(s).
  */
/*  if (defender_killed)
    return */
  
  return true;
} /* attack */



/*
  Returns: True if the defender was slain (or moved away), otherwise false.
*/
blean_t attack_internal(creature_t * attacker,
			creature_t * defender,
			item_t * weapon)
{
  char hit_message[80];
  char line[80];

  char att_fullname[50];
  char def_fullname[50];

  char hit_type_3rd[30];
  char hit_type_own[30];

  int def_y;
  int def_x;
  int att_y;
  int att_x;

  blean_t display_msg;
 
/*  blean_t hit;*/
/*  signed int attack_skill;
    signed int dodge;*/

  blean_t player_involved;

  blean_t defender_gone;

  /*
    We must have an attacker and a defender. We also *must* have a
    weapon. If no weapon is equipped, one of virtual_weapon[] (see
    vweapon.h) should be used. We simply won't accept a NULL.
  */
  if (attacker == NULL ||
      defender == NULL ||
      weapon == NULL)
  {
    return false;
  }

  /* This only works for the player so we don't need to care if they are the same. */
  break_rest(defender);
      
  /* This might change later if we allow *any* item to be wielded. */
  if (weapon->item_type != item_type_m_weapon)
  {
    queue_msg("BUG: suspicious weapon");
    return false;
  }

  if (is_player(attacker) || is_player(defender))
    player_involved = true;
  else
    player_involved = false;

  /* Make it easier to access the coordinates */
  att_y = attacker->y;
  att_x = attacker->x;
  def_y = defender->y;
  def_x = defender->x;

  /*
    Determine what attack verb to use ("A X:es B!"). The strings will
    be stored in hit_type_3rd and hit_type_own - since they are local
    they do not need to be freed afterwards.
  */
  get_attack_strings(weapon->custom[WEAPON_ATTACK_STRING],
		     hit_type_3rd, hit_type_own);

  /*
    If the player can't see any of the combatants display it as
    "something" instead of the regular name.
  */
  if (can_see_creature(game->player, attacker))
  {
    sprintf(att_fullname, "%s", attacker->name_one);
  }
  else
    strcpy(att_fullname, "something");
  
  if (can_see_creature(game->player, defender))
  {
    sprintf(def_fullname, "%s", defender->name_one);
  }
  else
    strcpy(def_fullname, "something");
  
  /*
    If the melee takes place outside the players field of view, no
    message should be displayed, UNLESS the player itself is
    involved.
  */
  if (can_see(game->player, def_y, def_x) ||
      can_see(game->player, att_y, att_x) ||
      player_involved)
  {
    display_msg = true;
  }
  else
  {
    display_msg = false;
  }

  /* Make sure the player gets an updated view of what's going on. */
  draw_level();

  /*
    Most messages come in 3 forms; player fighting NPC, NPC fighting
    player, NPC fighting NPC. We'll write the message to be displayed
    in this string and print it near the end of the function; if more
    than one line is needed, we'll handle it in some special way.
  */
  strcpy(hit_message, "");

  /* Start the actual combat */
/*  instant_reveal(attacker);
    instant_reveal(defender);*/

  /* If the attacker hasn't been detected yet... */
  if (attacker->detected == false)
  {
    if (can_see(game->player, att_y, att_x))
    {
      attacker->detected = true;
      draw_level();

      if (reveal_mimic(attacker))
      {
      }
      else if (is_player(defender))
      {
	sprintf(line, "Ambushed by %s!", att_fullname);
	upperfirst(line);
	queue_msg(line);
	msgflush_wait();
      }
      else
      {
	msg_glue(att_fullname, "is revealed!");
      }
    }
  }
  
  /* If the defender hasn't been detected yet... */
  if (defender->detected == false)
  {
    if (can_see(game->player, def_y, def_x))
    {
      defender->detected = true;
      draw_level();
      msg_glue(def_fullname, "is revealed!");
    }
  }

  aggravate(defender);
      
  /* Deal damage */
  melee_hit(attacker, defender, weapon);
  
  if (killed(defender))
  {
    /* The defender was slain! */
    
    if (is_player(defender))
    {
      queue_msg("You die...");
      
      sprintf(line, "were killed by %s", attacker->name_one);

      /* Goodbye. We won't proceed after this call. */
      check_for_player_death(line);
    }
    else
    {
      /* Kill the creature. */
      creature_death(defender, attacker, weapon);
      defender = NULL;
      
      /* Update the map */
      draw_level();

      if (is_player(attacker))
	try_to_id_weapon(attacker, weapon);
      
      return true;
    }
  }

  if (is_player(attacker))
    try_to_id_weapon(attacker, weapon);

  weapon_break(attacker, weapon);

  /* It's still alive */
  
  /*
    RFE: This should be handled some other way.
  */
  
  /* Slimes divide when hit */
  if (mycmp(defender->name_only, "slime") == true)
  {
    split_creature(defender);
  }
  
  defender_gone = weapon_effect(attacker, defender, weapon);
  
  /*
    Sleeping creatures wake up (if they are still alive). This is
    after the hit message is queued since we *hit* before the enemy
    wakes up.
  */
  creature_sleep(defender, false);
  
  /* If the player was involved, update stats */
  if (player_involved)
  {
    display_stats(game->player);
  }

  if (display_msg) {}
  
  return defender_gone;
} /* attack_internal */



/*
  When ATTACKER hits DEFENDER with WEAPON.
*/
void melee_hit(creature_t * attacker,
	       creature_t * defender,
	       item_t * weapon)
{
  char hit_type_3rd[30];
  char hit_type_own[30];

  char line[80];

  unsigned int damage_to_deal;
/*  signed int damage_dealt;*/
  unsigned int damage_type;
  
  blean_t display_msg;

  char att_fullname[50];
  char def_fullname[50];

  strcpy(line, "");

  if (attacker == NULL || defender == NULL || weapon == NULL)
    return;
      
  /*
    If any of the combatants are outside the players field of view,
    display it as "something" instead of its regular name.
  */
  if (can_see(game->player, attacker->y, attacker->x))
  {
    sprintf(att_fullname, "%s", attacker->name_one);
  }
  else
    strcpy(att_fullname, "something");
  
  if (can_see(game->player, defender->y, defender->x))
  {
    sprintf(def_fullname, "%s", defender->name_one);
  }
  else
    strcpy(def_fullname, "something");

  get_attack_strings(weapon->custom[WEAPON_ATTACK_STRING],
		     hit_type_3rd, hit_type_own);
  
  /*
    If the melee takes place outside the players field of view, no
    message should be displayed, UNLESS the player itself is
    involved.
  */
  if (can_see_creature(game->player, defender) ||
      can_see_creature(game->player, attacker) ||
      (is_player(attacker)) ||
      (is_player(defender)))
  {
    display_msg = true;
  }
  else
  {
    /* We can't see/feel it. */
    display_msg = false;
  }

  /* Deal damage. Enemies take 2x max damage if they are helpless. */
  damage_to_deal = weapon->custom[WEAPON_ATKSEQ + attacker->attack_pos];

  if (damage_to_deal <= 0)
  {
    queue_msg("BUG: bad attack sequence");
    return;
  }

  if (is_helpless(defender) && is_player(defender) == false)
  {
    damage_to_deal *= 2;
  }

  attacker->next_attack = attacker->attack_pos + 1;

  damage_type = get_damage_type(weapon);
  damage_to_deal = damage_armor(defender, damage_to_deal, damage_type);
  damage(defender, damage_to_deal);

//  if (display_msg)
  add_anim(anim_type_damage, defender->uid, damage_to_deal, defender->y, defender->x);

  if (!is_player(defender) && killed(defender))
    return;

  /* Generate a preliminary hit message. */
  if (is_player(attacker))
    sprintf(line, "You %s!", hit_type_own);
  else if (is_player(defender))
    sprintf(line, "%s %s you!", att_fullname, hit_type_3rd);
  else
    sprintf(line, "%s %s %s!", att_fullname, hit_type_3rd, def_fullname);

  sprintf(line + strlen(line) - 1, " (%d)!", damage_to_deal);
/*  queue_msg(line);*/

  if (display_msg)
  {
    upperfirst(line);
    queue_msg(line);
  }

  return;
} /* melee_hit */



/*
  Applies DAMAGE to any armor CREATURE is wearing, unless DT is a type
  that doesn't affect armor (e.g. cold). Also takes into account any
  static damage absorption or resistances the creature has. Returns
  how much damage should be passed through to the creatures health.
*/
unsigned int damage_armor(creature_t * creature, signed int damage_to_deal, const damage_type_t dt)
{
  unsigned int passthrough;
  item_t * armor;
  char line[80];
  unsigned int armor_damage;
  unsigned int weight;

  if (creature == NULL)
    return 0;

  /* The first point of damage always gets through. */
  passthrough = 1;
  damage_to_deal--;

  if (dt == damage_general)
  {
    armor = get_equipped(creature, item_type_body, 0);
    
    if (armor)
    {
      armor_damage = MIN(armor->custom[ARMOR_DURABILITY], MIN(armor->custom[ARMOR_PROTECTION], damage_to_deal));
      damage_to_deal -= armor_damage;
      
      if (armor_damage >= armor->custom[ARMOR_DURABILITY])
      {
	if (is_player(creature))
	{
	  if (identified(armor) & known_name)
	  {
	    if (armor->single_id_article == article_i_some_are)
	      sprintf(line, "Your %s have been destroyed!", armor->single_id_name);
	    else
	      sprintf(line, "Your %s has been destroyed!", armor->single_id_name);
	  }
	  else
	    sprintf(line, "Your %s has been destroyed!", armor->single_unid_name);
	  
	  queue_msg(line);
	}
	
	weight = armor->weight;
	del_item(armor);
	unburdened(creature, weight);
      }
      else if (armor->indestructible == false)
      {
	armor->custom[ARMOR_DURABILITY] -= armor_damage;
      }
    }
    
    /* Absorption applies to "general" damage. */
    damage_to_deal -= attr_current(creature, attr_absorption);
  }
  else if (dt == damage_cold)
  {
    damage_to_deal -= attr_current(creature, attr_cold_resistance);
  }
  else if (dt == damage_acid)
  {
    damage_to_deal -= attr_current(creature, attr_acid_resistance);
  }
  else if (dt == damage_fire)
  {
    damage_to_deal -= attr_current(creature, attr_fire_resistance);
  }

  if (damage_to_deal > 0)
    passthrough += damage_to_deal;
  
  return passthrough;
} /* damage_armor */



/*
  Returns: true if DEFENDER was moved or slain, otherwise false.
*/
blean_t weapon_effect(creature_t * attacker,
		      creature_t * defender,
		      item_t * weapon)
{
  char line[80];

  char att_fullname[50];
  char def_fullname[50];

  strcpy(line, "");

  if ((attacker == NULL) ||
      (defender == NULL) ||
      (weapon == NULL))
  {
    return false;
  }
      
  /* If any of the combatants are outside the players field of view,
     display it as "something" instead of the regular name */
  if (can_see(game->player, attacker->y, attacker->x))
  {
    sprintf(att_fullname, "%s", attacker->name_one);
  }
  else
    strcpy(att_fullname, "something");
  
  if (can_see(game->player, defender->y, defender->x))
  {
    sprintf(def_fullname, "%s", defender->name_one);
  }
  else
    strcpy(def_fullname, "something");
  
  /* Make special weapon effects take effect. Since we got here, we'll
   * assume the defender is still alive. */
  if (tslrnd() % 100 < get_item_mod(weapon, attr_i_drain))
  {
/*    set_attr_base(defender, attr_hp_max, attr_base(defender, attr_hp_max) - 1);*/
    heal(attacker, 1);
    
    if (is_player(defender))
      queue_msg("You feel drained!");
    else if (can_see(game->player, defender->y, defender->x))
      msg_glue(def_fullname, "is drained!");
  }
  
  if (tslrnd() % 100 < get_item_mod(weapon, attr_i_stun))
  {
    /* Stunning weapon */
    prolong_effect(defender, effect_stun, sroll(STUN_LENGTH));
    
    if (is_player(defender))
      queue_msg("You are stunned!");
    else if (can_see(game->player, defender->y, defender->x))    
      msg_glue(def_fullname, "is stunned!");
  }
  
  if (tslrnd() % 100 < get_item_mod(weapon, attr_i_wound))
  {
    /* Wounding weapon */

    if (wound_creature(defender, MELEE_WOUND_TIME) > 0) /* checks for wound immunity */
    {
      if (is_player(defender))
	queue_msg("You have been wounded!");
      else if (can_see(game->player, defender->y, defender->x))    
	msg_glue(def_fullname, "is wounded!");
    }
  }
  
  /* There is a chance the defender will be blinked away, if the weapon supports it */
  if (tslrnd() % 100 < get_item_mod(weapon, attr_i_blink))
  {
    cast_blink(defender, weapon, 0);

    return true;
  }
  else if (tslrnd() % 100 < get_item_mod(weapon, attr_i_knockback))
  {  
    /*
      There is a chance we will knock the defender back, if the
      attackers weapon supports it. This won't happen if we already
      blinked the creature away.
    */
    
    signed int move_y;
    signed int move_x;
    blean_t moved;
    dir_t dir;
    
    /* The defender should move the opposite direction from the attacker */
    if (attacker->y > defender->y)
      move_y = -1;
    else if (attacker->y < defender->y)
      move_y = +1;
    else
      move_y = 0;
    
    if (attacker->x > defender->x)
      move_x = -1;
    else if (attacker->x < defender->x)
      move_x = +1;
    else
      move_x = 0;

    dir = speed_to_dir(move_y, move_x);

    moved = push_internal(defender, dir, sroll(PUSH_ROLL));
    
    if (moved)
      return true;
  }

  return false;  
} /* weapon_effect */



/*
 */
blean_t weapon_break(creature_t * attacker, item_t * weapon)
{
  char line[200];
  char * pos;
  char * name;
  char * temp;
  blean_t player;

  if (attacker == NULL || weapon == NULL)
    return false;

  if (weapon->indestructible)
    return false;
  
  player = is_player(attacker);

  if (player)
    strcpy(line, "Your ");
  else
  {
    sprintf(line, "%ss ", attacker->name_the);
    upperfirst(line);
  }

  pos = line + strlen(line);

  if (tslrnd() % WEAPON_BREAK_SCALE < weapon->custom[WEAPON_BREAK])
  {
    name = get_item_name(weapon);
    temp = strchr(name, ' ') + 1;

    switch (weapon->item_number)
    {
      case treasure_wooden_stick:
      case treasure_quarterstaff:
	sprintf(pos, "%s snaps!", temp);
	break;

      case treasure_crystal_sword:
      case treasure_broken_bottle:
	sprintf(pos, "%s shatters!", temp);
	break;

      default:
	sprintf(pos, "%s breaks!", temp);
	break;
    }

    free(name);

    del_item(weapon);

    if (player || can_see_creature(game->player, attacker))
    {
      queue_msg(line);

      if (player)
      {
	msgflush_wait();
	clear_msgbar();
      }
    }
    
    return true;
  }

  weapon->custom[WEAPON_BREAK] += WEAPON_BREAK_INCREASE;

  return false;
} /* */
