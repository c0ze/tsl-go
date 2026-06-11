#include <stdlib.h>
#include <stdio.h>

#include "stuff.h"
#include "rules.h"
#include "area.h"
#include "missile.h"
#include "message.h"
#include "player.h"
#include "ui.h"
#include "input.h"
#include "ai.h"
#include "game.h"
#include "actions.h"
#include "fov.h"
#include "elements.h"
#include "balls.h"
#include "explode.h"



blean_t force_bolt(creature_t * caster, item_t * source, signed int param)
{
  return ball_bolt(caster, source, attr_m_force_bolt, param);
} /* force_bolt */



blean_t fireball(creature_t * caster, item_t * source, signed int param)
{
  return ball_bolt(caster, source, attr_m_fireball, param);
} /* fireball */



blean_t mudball(creature_t * caster, item_t * source, signed int param)
{
  return ball_bolt(caster, source, attr_m_mudball, param);
} /* mudball */



/*
  Lets CASTER cast a missile spell (determined by type) in any
  direction. The missile will travel for a limited distance or until
  it reaches a non-walkable tile.

  Fireballs will produce a 3x3 explosion dealing fire damage to any
  creature caught in it.
*/
blean_t ball_bolt(creature_t * caster, item_t * source, attr_index_t type, signed int param)
{
  level_t * level;
  item_t * missile;

  char line[100];

  dir_t dir;

  blean_t spell_known;

  unsigned int range;
  unsigned int difficulty;
  
  if (caster == NULL)
    return false;

  level = caster->location;

  if (level == NULL)
    return false;

  /* Do we know what spell we're trying to use? Unidentified wands, etc. */
  if (source == NULL || (identified(source) & known_name))
    spell_known = true;
  else
    spell_known = false;

  /* How far can this missile travel? The AI needs to know this early. */
  range = get_spell_range(caster, type);

  /* Did we get a direction? */
  if (param != dir_none)
  {
    dir = param;
  }
  else if (is_player(caster))
  {
    /* Let the user select a direction. */
    if (spell_known)
    {
      sprintf(line, "Shoot %s in which direction?", attr_info[type]->name);
      upperfirst(line);
      queue_msg(line);
    }
    else
      queue_msg("In which direction?");

    msgflush_nowait();
    
    /* This is the last chance to escape the casting */
    dir = get_direction();
  }
  else
  {
    /* Non-player caster; let AI decide direction */

    dir = target_best_direction(caster, range);
  }

  /* Should we abort? */
  if (dir == dir_none)
  {
    if (is_player(caster))
      clear_msgbar();

    return false;
  }
  
  /*
    If the player is casting this there's a random chance of
    failure. Wands and enemy spellcasters never fail.
  */

  switch (type)
  {
    case attr_m_force_bolt:
      difficulty = FORCE_BOLT_DIFFICULTY;
      break;

    case attr_m_fireball:
      difficulty = FIREBALL_DIFFICULTY;
      break;

    case attr_m_mudball:
    default:
      difficulty = 0;
      break;
  }

  if ((source == NULL)
      && is_player(caster)
      && roll_xn(difficulty, attr_current(caster, attr_magic)))
  {
    /* Spell misfired! */

    switch (type)
    {
      case attr_m_fireball:
	queue_msg("The fireball explodes in your face!");
	explosion(level, caster, NULL, caster->y, caster->x,
		  expl_fireball, sroll("1d6"), NULL, NULL);
	/* TODO: Fix damage */
	break;
	
      case attr_m_force_bolt:
	queue_msg("Your Force Bolt fizzles!");
	break;
	
      default:
	queue_msg("BUG: WHAT HAPPEN!!");
    }
    
    /* Action was completed */
    return true;
  }
  
  /* Casting succeeded */

  /* Identify the name of the wand (if any). Don't reveal the number of charges. */
  if (source && can_see(game->player, caster->y, caster->x))
    make_item_known(source);
  
  switch (type)
  {
    case attr_m_fireball:
      missile = build_item(treasure_fireball);
      missile->custom[WEAPON_MIN_DAMAGE] = FIREBALL_DAMAGE;
      break;

    case attr_m_force_bolt:
      missile = build_item(treasure_force_bolt);
      missile->custom[WEAPON_MIN_DAMAGE] = intrnd(FORCE_BOLT_MIN_DAMAGE, FORCE_BOLT_MAX_DAMAGE);
      break;
     
    case attr_m_mudball:
      missile = build_item(treasure_mudball);
      missile->custom[WEAPON_MIN_DAMAGE] = MUDBALL_DAMAGE;
      break;
     
    default:
      break;
  }

  if (missile == NULL)
  {
    queue_msg("BUG: Couldn't get missile!");
    return false;
  }

  /* Fire ze missiles! */
  fire_missile(caster, dir, NULL, missile, true, range);

  msgflush_wait();
  
  return true;
} /* ball_bolt */



