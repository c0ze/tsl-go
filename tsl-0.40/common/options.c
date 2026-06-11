#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <string.h>

#include "stuff.h"
#include "options.h"
#include "input.h"
#include "keymap.h"
#include "browser.h"
#include "game.h"
#include "message.h"
#include "ui.h"


/*
  Parses the config file CONFIG_NAME (#define in main.h), located in
  the users home directory. If no config file is found or it couldn't
  be read, false is returned. If a file was read (even if it contained
  no valid settings), true is returned.
*/
blean_t parse_config_file()
{
  char * config_path;
  char error[80];
  blean_t first;
  char * file;
  char * token;
  char delim[3] = {' ',  '\n', '\0'};

  /*
    Build a path to the config file and read it into memory. Under
    Windows we won't try to read from the home directory.
  */

  config_path = NULL;
  file = NULL;

  #ifndef _WIN32
  config_path = get_file_path(CONFIG_NAME);
  file = read_file(config_path);
  #endif
  
  /* Did we get it? */
  if (file == NULL)
  {
    free(config_path);

    /* Try to read another file from the CWD. */
    config_path = mydup("tsl_conf");
    file = read_file(config_path);

    if (file == NULL)
    {
      free(config_path);
      return false;
    }
  }

  /* Start parsing... */
  strcat(error, "");
  token = strtok(file, delim);
  first = true;

  while (token != NULL)
  {
    if (first == false)
      token = strtok(NULL, delim);
    else
      first = false;
    
    if (token == NULL)
    {
      /* We're done */
      free(file);
      free(config_path);
      return true;
    }

    if (mycmp(token, "nomorgue"))
    {
      options.morgue = false;
      continue;
    }
    else if (mycmp(token, "largecursor"))
    {
      options.large_cursor = true;
      continue;
    }
    else if (mycmp(token, "dotfloors"))
    {
      options.dotfloors = true;
      continue;
    }
    else if (mycmp(token, "fullscreen"))
    {
      options.fullscreen = true;
      continue;
    }
    else if (mycmp(token, "noautoequip"))
    {
      options.autoequip = false;
      continue;
    }
    else if (mycmp(token, "autocenter"))
    {
      options.autocenter = true;
      continue;
    }
    else if (mycmp(token, "forcegetname"))
    {
      options.forcegetname = true;
      continue;
    }
    else if (mycmp(token, "dvorak"))
    {
      dvorak_keymap();
      continue;
    }
    else if (mycmp(token, "name"))
    {
      /* name <s> - Sets the default name of the player to s. */
      
      token = strtok(NULL, delim);
      
      if (token != NULL)
      {
	options.default_name = mydup(token);
	continue;
      }
      else
      {
	sprintf(error, "usage: name NAME");
	break;
      }
    }
    else if (mycmp(token, "bind") ||
	     mycmp(token, "bindn"))
    {
      /*
	bind <s> <c> - Binds character c to action s.
	bindn <s> <i> - Binds ASCII numeral i to action s.
      */
      
      int mode;
      action_t action;
      
      if (mycmp(token, "bindn"))
	mode = 2;
      else
	mode = 1;
      
      token = strtok(NULL, delim);

      if (token != NULL)
      {
	action = string_to_action(token);

	if (action != action_undefined)
	{
	  token = strtok(NULL, delim);

	  if (token == NULL) break;
	     
	  if (mode == 1)
	  {
	    bind_key(action, token[0]);
	    continue;
	  }
	  else if (mode == 2)
	  {
	    bind_key(action, atoi(token));
	    continue;
	  }
	}
      }
      
      if (mode == 1)
	sprintf(error, "usage: bind <action> <c>");
      else
	sprintf(error, "usage: bindn <action> <i>");

      break;
    }
    else
    {
      sprintf(error, "unknown directive");
      break;
    }
  }

  printf("In %s; parse error at \"%s\": %s\n",
	 config_path, token, error);
  free(config_path);
  free(file);
  exit(2);
} /* parse_config_file */



/*
  Sets the default options that will typically be in place unless
  overridden in the config file.
 */
void set_default_options()
{
  options.autocenter = false;
  options.morgue = true;
  options.large_cursor = false;
  options.dotfloors = false;
  options.fullscreen = false;
  options.autoequip = true;
  options.forcegetname = false;
  options.default_name = NULL;
  options.safe_teleport = true;
  options.glyph_mode = false;
  options.diagonals = DIAGONALS_OFF;
  /*options.item_categories = false;*/

  options.debug_fov = false;

  return;
} /* set_default_options */



void options_menu()
{
  menu_item_t ** menu_list;
//  int total_items;
  int selection;
//  browser_t * b;
  int c;
  int i;
  char line[100];  
  char select_key[20];

/*  b = &browser[MENU_OPTIONS];
  b->cur_pos = 0;
  b->start_row = 0;*/

  key_help(select_key, false, action_select);

  menu_list = malloc(sizeof(menu_item_t *) * OPTION_MENU_LEN);

  if (menu_list == NULL)
    out_of_memory();

  for (i = 0; i < OPTION_MENU_LEN; i++)
    menu_list[i] = alloc_menu_item();

  c = 0;

  sprintf(line, "%s to change", select_key);
  menu_list[OPTION_NAME]->explanation = mydup(line);
  c++;

  sprintf(line, "%s to toggle", select_key);
  menu_list[OPTION_MORGUE]->explanation = mydup(line);
  menu_list[OPTION_MORGUE]->inspect = mydup("Toggles whether a morgue file will be written when your character dies or you quit the game. A morgue file contains all character scores, fully identified equipment and can be used to review a character after the game is over.");
  c++;

  sprintf(line, "%s to toggle", select_key);
  menu_list[OPTION_AUTOEQUIP]->explanation = mydup(line);
  menu_list[OPTION_AUTOEQUIP]->inspect = mydup("Automatically equips items picked up, if you do not already have an item of that type equipped.");
  c++;

  sprintf(line, "%s to toggle", select_key);
  menu_list[OPTION_AUTOCENTER]->explanation = mydup(line);
  menu_list[OPTION_AUTOCENTER]->inspect = mydup("Automatically centers the view around the player.");
  c++;

  sprintf(line, "%s to toggle", select_key);
  menu_list[OPTION_SAFE_TELEPORT]->explanation = mydup(line);
  menu_list[OPTION_SAFE_TELEPORT]->inspect = mydup("Displays a confirmation prompt when attempting an obviously unsafe teleport (into walls, for example). This does not guarantee the destination is safe, it's only for preventing player errors.");
  c++;

  sprintf(line, "%s to cycle", select_key);
  menu_list[OPTION_DIAGONALS]->explanation = mydup(line);
  menu_list[OPTION_DIAGONALS]->inspect = mydup("Allow diagonal input on orthagonal D-pad or directional keys. Sloppy: Hold two keys and release to move. Step: Press \"step\" key to move.");
  c++;

  sprintf(line, "%s to toggle", select_key);
  menu_list[OPTION_GLYPH_MODE]->explanation = mydup(line);
  menu_list[OPTION_GLYPH_MODE]->inspect = mydup("Text-only display.");
  c++;

/*  sprintf(line, "%s to toggle", select_key);
  menu_list[OPTION_ITEM_CATEGORIES]->explanation = mydup(line);
  menu_list[OPTION_ITEM_CATEGORIES]->inspect = mydup("Display categories in inventory and item selection screens.");
  c++;*/

  for (i = 0; i < c; i++)
    menu_list[i]->act = i;

  while (1)
  {
    free(menu_list[OPTION_NAME]->label);
    sprintf(line, "Character name: %s", game->native->name_only);
    menu_list[OPTION_NAME]->label = mydup(line);

    free(menu_list[OPTION_MORGUE]->label);
    sprintf(line, "Dump morgue file: %s", (options.morgue ? "Yes" : "No"));
    menu_list[OPTION_MORGUE]->label = mydup(line);

    free(menu_list[OPTION_AUTOEQUIP]->label);
    sprintf(line, "Auto-equip: %s", (options.autoequip ? "Yes" : "No"));
    menu_list[OPTION_AUTOEQUIP]->label = mydup(line);

    free(menu_list[OPTION_AUTOCENTER]->label);
    sprintf(line, "Auto-center: %s", (options.autocenter ? "Yes" : "No"));
    menu_list[OPTION_AUTOCENTER]->label = mydup(line);

    free(menu_list[OPTION_SAFE_TELEPORT]->label);
    sprintf(line, "Safe teleport: %s", (options.safe_teleport ? "Yes" : "No"));
    menu_list[OPTION_SAFE_TELEPORT]->label = mydup(line);

    free(menu_list[OPTION_GLYPH_MODE]->label);
    sprintf(line, "Glyph mode: %s", (options.glyph_mode ? "Yes" : "No"));
    menu_list[OPTION_GLYPH_MODE]->label = mydup(line);

/*    free(menu_list[OPTION_ITEM_CATEGORIES]->label);
    sprintf(line, "Item categories: %s", (options.item_categories ? "Yes" : "No"));
    menu_list[OPTION_ITEM_CATEGORIES]->label = mydup(line);*/

    free(menu_list[OPTION_DIAGONALS]->label);
    if (options.diagonals == DIAGONALS_OFF)
      sprintf(line, "Fake diagonals: %s", "Off");
    else if (options.diagonals == DIAGONALS_SLOPPY)
      sprintf(line, "Fake diagonals: %s", "Sloppy");
    else if (options.diagonals == DIAGONALS_STEP)
      sprintf(line, "Fake diagonals: %s", "Step");
    menu_list[OPTION_DIAGONALS]->label = mydup(line);

    selection = browse(menu_list, c, MENU_OPTIONS, NULL, NULL);
    
    switch (selection)
    {
      case OPTION_MORGUE:
	options.morgue = !options.morgue;
	break;

      case OPTION_AUTOEQUIP:
	options.autoequip = !options.autoequip;
	break;
	
      case OPTION_AUTOCENTER:
	options.autocenter = !options.autocenter;
	break;

      case OPTION_SAFE_TELEPORT:
	options.safe_teleport = !options.safe_teleport;
	break;

      case OPTION_GLYPH_MODE:
	set_glyph_mode(!options.glyph_mode);
	recenter_if_needed();
	draw_level();	
	break;

      case OPTION_DIAGONALS:
	options.diagonals = (options.diagonals + 1) % 3;
	break;

	/*
	  case OPTION_ITEM_CATEGORIES:
	  options.item_categories = !options.item_categories;
	  break;
	*/

      case OPTION_NAME:
	read_string("Character name?", line, 29);
	clear_msgbar();

	if (strlen(line) > 0)
	{
	  set_creature_name(game->native, line, line, line);
	  
	  if (options.default_name)
	    free(options.default_name);

	  options.default_name = mydup(line);
	}

	break;

      case -1:
	goto cleanup;
    }
  }

cleanup:
  del_menu(menu_list, c);
  
  return;
} /* options_menu */
