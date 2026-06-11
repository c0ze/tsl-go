#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "main.h"
#include "stuff.h"
#include "game.h"
#include "losegame.h"
#include "message.h"
#include "options.h"
#include "player.h"
#include "inventory.h"
#include "ui.h"

/*
  Checks if the player has died. If it has, quit the game.  REASON is
  the death message (such as "stumbled on a frog") that will be
  written in various places.
*/
void check_for_player_death(const char * reason)
{
  char * death_message;

  if (game->player == NULL)
    return;
  
  if (killed(game->player) ||
      (game->game_over))
  {
    game->died = time(NULL);
    game->game_over = true;

    msgflush_wait();

    if (options.morgue)
    {
      morgue_dump(reason);
    }

    death_message = get_death_message(reason, true);

    game_over(death_message, false);
    shutdown_everything();

    free(death_message);

    exit(0);
  }
  
  return;
} /* check_for_player_death */



/*
  Builds a summary of the player character and saves it in morgue.txt
  in the current working directory.
*/
void morgue_dump(const char * cause_of_death)
{
  char * temp;
  char * death_message;
  char * dump;
  FILE * dumpfile;

  if (game->player == NULL)
    return;

  dump = build_character_dump();
  
  dumpfile = fopen(MORGUE_NAME, "w");

  if (dumpfile == NULL)
    return;

  death_message = get_death_message(cause_of_death, true);
  fprintf(dumpfile, "%s\n", death_message);
  free(death_message);

  /* The \ns are added automatically by ctime_r */
  temp = ctime(&game->started);

  fprintf(dumpfile, "%s %s\n", TSL_NAME, TSL_VERSION);

  if (temp != NULL)
    fprintf(dumpfile, "Started: %s", temp);

  if (game->game_over)
  {
    temp = ctime(&game->died);

    if (temp != NULL)
      fprintf(dumpfile, "Died: %s", temp);
  }

  fprintf(dumpfile, "\n");

  fwrite(dump, sizeof(char), strlen(dump), dumpfile);

  free(dump);

  return;
} /* morgue_dump */



/*
  Returns a string describing the player.
*/
/* TODO: This needs some serious work to make it aestethically pleasing. */
char * build_character_dump()
{
  unsigned int i;
  unsigned int known;
  unsigned int base;
  char * ret;
  char * item_name;
  char * item_data;
  char list[2000];
  char temp[4000];
  char line[80];
  item_t * item;

  /*
    If the game has ended, identify everything in the players
    inventory. We do before we display any attributes, since these
    will rely on what is _known_ about equipment.
  */
  if (game->game_over)
  {
    for (item = game->player->first_item; item != NULL; item = item->next_item)
      identify(item);
  }

  strcpy(temp, "");

  strcat(temp, "Facets\n");
  strcat(temp, "-------\n");

  item = game->first_facet;

  if (item)
    item = item->next_item;

  while (item != NULL)
  {
    sprintf(line, "%s\n", item->single_id_name);
    strcat(temp, line);
    item = item->next_item;
  }

  strcat(temp, "\n");

  strcat(temp, "Attributes\n");
  strcat(temp, "----------\n");

  /* Display temporary flags */
  /* RFE: Make this work with temp + string offset instead. */
  if (killed(game->player))
    status_flags(game->player, list, flag_mode_morgue);
  else
    status_flags(game->player, list, flag_mode_summary);

  strcat(temp, list);

  strcat(temp, "\n");

  sprintf(line, "Health: %d\n", attr_base(game->player, attr_health));

/*  sprintf(line, "Fatigue: %d/%d (%d)\n",
	  get_attr_base(game->player, attr_fatigue),
	  get_attr_known(game->player, attr_fatigue_limit),
	  get_attr_base(game->player, attr_fatigue_limit));
	  strcat(temp, line);*/

  sprintf(line, "Energy: %d/%d (%d)\n",
	  attr_base(game->player, attr_ep_current),
	  attr_known(game->player, attr_ep_max),
	  attr_base(game->player, attr_ep_max));
  strcat(temp, line);

  /* TODO: I'm not sure how this works. */
  for (i = 0; i < ATTRIBUTES; i++)
  {
    if (attr_info[i] != NULL &&
	attr_info[i]->morgue == true)
    {
      known = attr_known(game->player, i);
      base = attr_base(game->player, i);

      if (known != 0 || base != 0)
      {
	sprintf(line, "%s: %d (%d)\n",
		attr_info[i]->name,
		known, base);
	
	strcat(temp, line);
      }
    }
  }

  strcat(temp, "\n");

  strcat(temp, "Inventory\n");
  strcat(temp, "---------\n");

  for (item = game->player->first_item; item != NULL; item = item->next_item)
  {
    if (game->game_over)
      identify(item);

    item_name = get_item_name(item);

    sprintf(line, "%c) ", item->letter);
    strcat(temp, line);

    if (item->equipped)
      strcat(temp, "<");

    sprintf(line, "%s", item_name);
    strcat(temp, line);

    if (item->equipped)
      strcat(temp, ">");

    strcat(temp, "\n");

    free(item_name);

    item_data = get_item_data(item);
    strcat(temp, item_data);
    strcat(temp, "\n");
    free(item_data);
  }

  /* Finished */
  ret = mydup(temp);

  if (ret == NULL)
    out_of_memory();

  return ret;
} /* build_character_dump */



/*
  Returns the message to be printed when the player dies.
*/
char * get_death_message(const char * reason, const blean_t goodbye)
{
  char line1[160];
  char line2[160];
  char notes[80];
  char place[80];
  char temp[170];

  sprintf(line1, "Game over!\n\n%s%s...\n", 
	  (goodbye ? "Goodbye " : ""),
	  game->native->name_only);
  
  if (is_helpless(game->player) == true)
    strcat(notes, " (while helpless)");
  else
    strcpy(notes, "");
  
  if (game->won)
    strcpy(place, "");
  else
    sprintf(place, " in %s", get_current_level()->name);
  
  
  sprintf(line2, "%s%s%s%s after %ld turns.\n",
	  (goodbye ? "You " : ""),
	  (reason != NULL ? reason : "died for no reason"),
	  notes,
	  place,
	  game->turns);
  
  upperfirst(line2);

  sprintf(temp, "%s%s", line1, line2);

  return mydup(temp);
} /* get_death_message */


