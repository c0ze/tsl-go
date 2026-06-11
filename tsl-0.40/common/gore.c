#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "main.h"
#include "stuff.h"
#include "gore.h"
#include "message.h"
#include "fov.h"
#include "game.h"




/*
 */
void creature_death(creature_t * vic,
		    creature_t * killer, item_t * weapon)
{
  char * vic_name;
  char wpn_name[50];
  char with[60];
  char line[120];
  char temp[80];
  blean_t vic_blind;
  blean_t vic_breathing;
  gore_t attack_type;
/*  blean_t seen;*/

  /* RFE: This function is a mess. */

  if (is_player(killer) && is_blinded(killer))
    goto pure_kill;

  if (can_see_creature(game->player, vic) == false)
    return;

  if (is_player(killer) == false)
  {
    if (can_see_creature(game->player, vic) && vic->detected)
    {
      sprintf(line, "%s %s", vic->name_the, vic->death_msg);
    }

    upperfirst(line);
    queue_msg(line);
    kill_creature(vic, true);

    return;
  }

  vic_name = vic->name_the;
  sprintf(line, "BUG: no death message for %s!", vic_name);
  strcpy(with, "");

  vic_blind = is_blinded(vic);
  vic_breathing = (0 == attr_current(vic, attr_nonbreathing));

  if (weapon)
  {
    if (identified(weapon) & known_name)
      strcpy(wpn_name, weapon->single_id_name);
    else
      strcpy(wpn_name, weapon->single_unid_name);

    if (maybe())
      sprintf(with, " with %s%s", "your ", wpn_name);

    if (maybe() && (weapon->custom[WEAPON_FINISH_ALT] != gore_none))
      attack_type = weapon->custom[WEAPON_FINISH_ALT];
    else
      attack_type = weapon->custom[WEAPON_FINISH];
  }
  else
  {
    strcpy(wpn_name, "(BUG: no weapon)");
    attack_type = gore_none;
  }

/*  if (attack_type == gore_none)
  {
    sprintf(line, "BUG: undefined finish move for %s.", wpn_name);
  }
  else*/
 if (is_unique(vic) && is_player(killer))
  {
    sprintf(line, "You have slain %s!", vic_name);
  }
  else if (attack_type == gore_unarmed)
  {
    if (maybe() && vic_breathing && (vic->parts & part_neck))
      sprintf(line, "You choke %s to death.", vic_name);
    else
    {
      if (maybe() && (vic->parts & part_teeth))
      {
	sprintf(line, "You punch %s.", vic_name);
	sprintf(temp, "%s chokes on its own broken teeth.", vic_name);
	upperfirst(temp);
	strcat(line, " ");
	strcat(line, temp);
      }
      else if (maybe() && (vic->parts & part_teeth))
      {
	sprintf(line, "You curb-stomp %s.", vic_name);

	if (maybe() && (vic->parts & part_eyes))
	{
	  sprintf(temp, "One of %ss sad, large eyes bursts free of its head.", vic_name);
	  strcat(line, " ");
	  strcat(line, temp);
	}
      }
      else if (maybe() && (vic->parts & part_neck))
	sprintf(line, "You %s %ss neck until it snaps.", (maybe() ? "wring" : "twist"), vic_name);
      else
	sprintf(line, "You pound %s to a pulp%s.", vic_name, with);
    }
  }
  else if (attack_type == gore_cut)
  {
    if (maybe() && (vic->parts & part_throat) && vic_breathing)
      sprintf(line, "You slit %ss throat%s.", vic_name, with);
    else if (maybe() && (vic->parts & part_bowel))
    {
      sprintf(line, "You slash %s open.", vic_name);

      if (maybe())
      {
	sprintf(temp, "%s collapses, clutching its disemboweled stomach.", vic_name);
	upperfirst(temp);
	strcat(line, " ");
	strcat(line, temp);
      }
    }
    else if (maybe() && (vic->parts & part_head))
      sprintf(line, "You decapitate %s%s.", vic_name, with);
    else
      sprintf(line, "You split %s in two%s.", vic_name, with);
  }
  else if (attack_type == gore_claw)
  {
    if (maybe())
      sprintf(line, "You rip %s to pieces%s.", vic_name, with);
    else
      sprintf(line, "You tear %s apart%s.", vic_name, with);
  }
  else if (attack_type == gore_stab)
  {
    if (maybe() && (vic->parts & part_bowel))
      sprintf(line, "You stab %s in the gut%s.", vic_name, with);
    else if (maybe() && (vic->parts & part_throat))
      sprintf(line, "You stab %s in the throat%s.", vic_name, with);
    else if (maybe() && (vic->parts & part_head))
      sprintf(line, "You stab %s in the head%s.", vic_name, with);
    else if (maybe() && (vic->parts & part_eyes))
      sprintf(line, "You stab %s between its eyes%s.", vic_name, with);
    else if (maybe() && maybe())
      sprintf(line, "You stab and stab and stab %s%s until the voices go away.", vic_name, with);
    else
      sprintf(line, "You stab %s%s.", vic_name, with);

    if (maybe() && (vic->parts & part_ribs))
    {
      sprintf(temp, "%s dies with a gurgling sound.", vic_name);
      upperfirst(temp);
      strcat(line, " ");
      strcat(line, temp);
    }
  }
  else if (attack_type == gore_crush)
  {
/*
      sprintf(line, "You hear ribs cracking. A %s collapses.", vic_name);*/

    if (maybe() && (vic->parts & part_teeth))
    {
      sprintf(line, "You break %ss teeth in%s.", vic_name, with);
    }
    else if (maybe() && (vic->parts & part_skull))
    {
      sprintf(line, "You bash %ss skull open%s.", vic_name, with);
    }
    else if (maybe())
      sprintf(line, "You demolish %s with your %s.", vic_name, wpn_name);
    else
      sprintf(line, "You smash %s to bits%s.", vic_name, with);
  }
  else if (attack_type == gore_impale)
  {
    if (maybe())
      sprintf(line, "You impale %s on your %s.", vic_name, wpn_name);
    else
      sprintf(line, "You run %s through%s.", vic_name, with);

    if (!vic_blind && vic_breathing)
    {
      if (maybe())
      {
	sprintf(temp, "%s draws its last breath looking you in the eye.", vic_name);
	upperfirst(temp);
	strcat(line, " ");
	strcat(line, temp);
      }
      else if (maybe() && (vic->parts & part_ribs))
      {
	sprintf(temp, "%ss mouth fills with blood.", vic_name);
	upperfirst(temp);
	strcat(line, " ");
	strcat(line, temp);
      }
    }
  }
  else if (can_see_creature(game->player, vic) && vic->detected)
  {
    if (vic->death_msg != NULL)
      sprintf(line, "%s %s", vic_name, vic->death_msg);
    else
      sprintf(line, "BUG: %s doesn't have any death message set.", vic_name);
  }

  upperfirst(line);
  queue_msg(line);

pure_kill:

  kill_creature(vic, true);
  
  return;
} /* creature_death */
