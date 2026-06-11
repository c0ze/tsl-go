#include <stdlib.h>
#include <stdio.h>

#include "breath.h"
#include "stuff.h"
#include "rules.h"
#include "area.h"
#include "missile.h"
#include "message.h"
#include "player.h"
#include "ui.h"
#include "poison.h"
#include "losegame.h"
#include "input.h"
#include "ai.h"
#include "game.h"
#include "actions.h"
#include "fov.h"
#include "elements.h"
#include "sleep.h"
#include "find.h"
#include "combat.h"



blean_t noxious_breath(creature_t * caster, item_t * source, signed int param)
{
  return breath_weapon(caster, source, false);
} /* noxious_breath */



blean_t breathe_fire(creature_t * caster, item_t * source, signed int param)
{
  return breath_weapon(caster, source, true);
} /* breathe_fire */



blean_t breath_weapon(creature_t * caster, item_t * source, blean_t fire)
{
  unsigned int y;
  unsigned int x;
  unsigned int damage_taken;
  blean_t caster_seen;
  blean_t breathing;
  creature_t * target;
  char line[200];
  dir_t dir;
  blean_t message;
  blean_t spell_known;
  area_t * temp;
  area_t * area;
  unsigned int range;

  if (caster == NULL || caster->location == NULL)
    return false;

  if ((source == NULL) || (identified(source) & known_name))
    spell_known = true;
  else
    spell_known = false;

  if (fire)
    range = get_spell_range(caster, attr_m_breathe_fire);
  else
    range = get_spell_range(caster, attr_m_noxious_breath);

  if (attr_current(caster, attr_nonbreathing))
  {
    if (is_player(caster))
      queue_msg("You can not breathe!");

    return false;
  }

  if (is_player(caster))
  {
    /* Let the player select a direction */
    if (spell_known)
    {
      if (fire)
	queue_msg("Breathe fire in which direction?");
      else
	queue_msg("Breathe poison in which direction?");
    }
    else
    {
      queue_msg("In which direction?");
    }

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
    dir = cone_direction(caster, range, false);

    if (dir == dir_none)
      return false; /* No good direction to breathe in. */
  }

  /*
    This controls if the game will pause after the casting, displaying
    which areas were affected. If no message was displayed, nothing
    will have been displayed on the map either.
  */
  message = false;

  caster_seen = can_see_creature(game->player, caster);

  /* Decide which tiles are affected. */
  area = new_area(caster->y, caster->x);

  area_cone(area, caster->location, caster->y, caster->x, dir, range, false);

  /* Display them. Did the player see ANY tile? */
  if (area_of_effect(game->player, area->next, fire ? missile_fire : missile_poison))
    message = true;

  if (is_player(caster))
  {
    if (fire)
      queue_msg("You breathe fire!");
    else
      queue_msg("You breathe poison!");

    message = true;
  }
  else if (caster_seen)
  {
    if (fire)
      msg_one(caster, "breathes fire!");
    else
      msg_one(caster, "breathes poison!");

    message = true;
  }

  for (temp = area->next; temp != NULL; temp = temp->next)
  {
    y = temp->y;
    x = temp->x;
    
    target = find_creature(caster->location, y, x);
    
/*    if ((is_flyable(caster->location, y, x) == false && target == NULL) ||
	(y == caster->y && x == caster->x))
    {
      continue;
      }*/
    
    if (target)
    {
      reveal_mimic(target);
      
      if (fire)
      {
	damage_taken = damage(target, damage_armor(target, sroll(BREATHE_FIRE_DAMAGE), damage_fire));
      }
      else
      {
	if (attr_current(target, attr_nonbreathing) ||
	    attr_current(target, attr_gas_immunity))
	{
	  breathing = false;
	}
	else
	  breathing = true;
	
	if (breathing)
	{
	  damage_taken = damage(target, sroll(NOXIOUS_BREATH_DAMAGE));
	  
	  if (poison_creature(target, NOXIOUS_BREATH_POISON) > 0)
	    damage_taken++;
	}
	else
	  damage_taken = 0;
      }

      if (is_player(target))
      {
	if (caster_seen == false)
	{
	  if (fire)
	  {
	    queue_msg("You get burned!");
	  }
	  else
	  {
	    queue_msg("You are surrounded by poison gas!");

	    if (breathing)
	      queue_msg("You inhale the vile fumes!");
	  }

	  message = true;
	}
		
	if (killed(target))
	{
	  if (fire)
	    sprintf(line, "were killed by %ss fiery breath", caster->name_one);
	  else
	    sprintf(line, "were killed by %ss poisonous breath", caster->name_one);
	  
	  check_for_player_death(line);
	}

	damage_popup(target, damage_taken);
      }
      else
      {
	damage_popup(target, damage_taken);

	if (can_see_creature(game->player, target))
	{
	  if (!caster_seen)
	  {
	    if (fire)
	      msg_one(target, "is surrounded by flames!");
	    else
	      msg_one(target, "is surrounded by poison gas!");
	    
	    message = true;
	  }
	  else if (breathing)
	  {
	    if (fire)
	      msg_one(target, "gets burned!");
	    else
	      msg_one(target, "is affected!");
	    
	    message = true;
	  }
	  
	  if (killed(target))
	  {
	    if (fire)
	      msg_one(target, "collapses into a smoldering heap.");
	    else
	      msg_one(target, "succumbs to the poison.");
	    
	    kill_creature(target, false);
	  }
	}
      } /* else */
      
      creature_sleep(target, false);
      aggravate(target);
    }

    if (fire)
    {
      if (expose_tile_to_fire(caster->location, y, x) == true)
	message = true;
    }
  }

  del_area(area);

  if (message)
  {
    msgflush_wait();
    clear_msgbar();
  }

  return true;
} /* breath_weapon */
