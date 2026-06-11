#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <ctype.h>

#include "main.h"
#include "stuff.h"
#include "input.h"
#include "game.h"
#include "ui.h"
#include "item.h"
#include "inventory.h"
#include "options.h"
#include "keymap.h"

/*
  Converts a keypress to its corresponding action_t (see ui.h), if
  any. action_undefined is returned for any keys that isn't mapped.
  This special action should never be used for anything in the game.
*/
action_t key_to_action(const int key)
{
  unsigned int i;
  unsigned int j;

  /* Look through the keymap if this key has been assigned to an action */
  for (i = 0; i < ACTIONS; i++)
    for (j = 0; j < MAX_BINDINGS; j++)
      if (keymap[i][j] == key)
	  return i;

  /* This is an undefined key */
  return action_undefined;
} /* key_to_action */



/*
  Converts the key token INPUT to an action, but also considers numpad
  9 and 3 as page up/down.
*/
action_t key_to_action_browse(const key_token_t input)
{
  if (input == kt_np9 || input == kt_dir_left || input == kt_dir_up)
    return action_previous;
  else if (input == kt_np3 || input == kt_dir_right || input == kt_dir_down)
    return action_next;
  
  return key_to_action(input);
} /* key_to_action_browse */



/*
  Copies into DEST a textual representation of KEY. DEST must be 6
  bytes long, including space for the null byte.
*/
void key_to_label(char * dest, const blean_t apostrophes, const unsigned int key)
{
  switch (key)
  {
    case kt_escape:     strcpy(dest, "Down");  break;
    case kt_backspace:  strcpy(dest, "Up");    break;
    case kt_delete:     strcpy(dest, "Up");    break;
    case kt_page_up:    strcpy(dest, "PgUp");  break;
    case kt_page_down:  strcpy(dest, "PgDn");  break;
    case kt_dir_up:     strcpy(dest, "Up");    break;
    case kt_dir_right:  strcpy(dest, "Right"); break;
    case kt_dir_left:   strcpy(dest, "Left");  break;
    case kt_dir_down:   strcpy(dest, "Down");  break;
    case kt_f1:         strcpy(dest, "F1");    break;
    case kt_joy0:       strcpy(dest, "Joy0"); break;
    case kt_joy1:       strcpy(dest, "Joy0"); break;
    case kt_joy2:       strcpy(dest, "Joy0"); break;
    case kt_joy3:       strcpy(dest, "Joy0"); break;
    case kt_joy4:       strcpy(dest, "Joy0"); break;
    case kt_joy5:       strcpy(dest, "Joy0"); break;
    case kt_joy6:       strcpy(dest, "Joy0"); break;
    case kt_joy7:       strcpy(dest, "Joy0"); break;
    case kt_joy8:       strcpy(dest, "Joy0"); break;
    case kt_joy9:       strcpy(dest, "Joy0"); break;
    case kt_np0:        strcpy(dest, "NumP0"); break;
    case kt_np1:        strcpy(dest, "NumP1"); break;
    case kt_np2:        strcpy(dest, "NumP2"); break;
    case kt_np3:        strcpy(dest, "NumP3"); break;
    case kt_np4:        strcpy(dest, "NumP4"); break;
    case kt_np5:        strcpy(dest, "NumP5"); break;
    case kt_np6:        strcpy(dest, "NumP6"); break;
    case kt_np7:        strcpy(dest, "NumP7"); break;
    case kt_np8:        strcpy(dest, "NumP8"); break;
    case kt_np9:        strcpy(dest, "NumP9"); break;
    case kt_shift_np0:  strcpy(dest, "S+NP0"); break;
    case kt_shift_np1:  strcpy(dest, "S+NP1"); break;
    case kt_shift_np2:  strcpy(dest, "S+NP2"); break;
    case kt_shift_np3:  strcpy(dest, "S+NP3"); break;
    case kt_shift_np4:  strcpy(dest, "S+NP4"); break;
    case kt_shift_np5:  strcpy(dest, "S+NP5"); break;
    case kt_shift_np6:  strcpy(dest, "S+NP6"); break;
    case kt_shift_np7:  strcpy(dest, "S+NP7"); break;
    case kt_shift_np8:  strcpy(dest, "S+NP8"); break;
    case kt_shift_np9:  strcpy(dest, "S+NP9"); break;
    case '\t':          strcpy(dest, "Tab");   break;
    case '\n':          strcpy(dest, "Enter"); break;
    case ' ':           strcpy(dest, "Space"); break;

    default:
      if (apostrophes)
      {
	dest[3] = '\0';
	dest[2] = '\'';
	dest[1] = key;
	dest[0] = '\'';
      }
      else
      {
	dest[1] = '\0';
	dest[0] = key;
      }
      break;
  }

  return;
} /* key_to_label */



/*
  Copies into DEST a textual representation (see key_to_label) of the
  first key bound to ACTION.
*/
void key_help(char * dest, const blean_t apostrophes, const action_t action)
{
  unsigned int b;

  if (action >= ACTIONS || action < 0)
  {
    strcpy(dest, "(BUG)");
  }

  for (b = 0; b < MAX_BINDINGS; b++)
  {
    if (keymap[action][b] != 0)
    {
      key_to_label(dest, apostrophes, keymap[action][b]);
      return;
    }
  }

  /* Nothing bound */
  strcpy(dest, "(...)");

  return;
} /* key_help */



/*
  Lets the user pick a direction. One of the dir_* constants (see
  main.h) will be returned, dir_none if the action was cancelled.
*/
dir_t get_direction()
{
  signed int input;

  while (true)
  {
    input = key_to_action(get_keypress());

    switch (input)
    {
      case action_cancel:  return dir_none;
      case action_n:       return dir_n;
      case action_ne:      return dir_ne;
      case action_e:       return dir_e;
      case action_se:      return dir_se;
      case action_s:       return dir_s;
      case action_sw:      return dir_sw;
      case action_w:       return dir_w;
      case action_nw:      return dir_nw;
      default:             continue;
    }
  }
}



/*
  Converts a string representation of an action to an action_* constant.
 */
action_t string_to_action(const char * name)
{
  if (name == NULL) return action_undefined;
  else if (mycmp(name, "quit")) return action_quit;
  else if (mycmp(name, "n")) return action_n;
  else if (mycmp(name, "s")) return action_s;
  else if (mycmp(name, "nw")) return action_nw;
  else if (mycmp(name, "sw")) return action_sw;
  else if (mycmp(name, "e")) return action_e;
  else if (mycmp(name, "w")) return action_w;
  else if (mycmp(name, "ne")) return action_ne;
  else if (mycmp(name, "se")) return action_se;
  else if (mycmp(name, "fire_n")) return action_fire_n;
  else if (mycmp(name, "fire_s")) return action_fire_s;
  else if (mycmp(name, "fire_nw")) return action_fire_nw;
  else if (mycmp(name, "fire_sw")) return action_fire_sw;
  else if (mycmp(name, "fire_e")) return action_fire_e;
  else if (mycmp(name, "fire_w")) return action_fire_w;
  else if (mycmp(name, "fire_ne")) return action_fire_ne;
  else if (mycmp(name, "fire_se")) return action_fire_se;
  else if (mycmp(name, "inventory")) return action_inventory;
  else if (mycmp(name, "inspect")) return action_inspect;
  else if (mycmp(name, "version")) return action_version;
  else if (mycmp(name, "cancel")) return action_cancel;
  else if (mycmp(name, "pickup")) return action_pickup;
  else if (mycmp(name, "equip")) return action_equip;
  else if (mycmp(name, "remove")) return action_remove;
  else if (mycmp(name, "drop")) return action_drop;
  else if (mycmp(name, "debug")) return action_debug;
  else if (mycmp(name, "wait")) return action_wait;
  else if (mycmp(name, "select")) return action_select;
  else if (mycmp(name, "stairs")) return action_stairs;
  else if (mycmp(name, "redraw")) return action_redraw;
  else if (mycmp(name, "fire")) return action_fire;
  else if (mycmp(name, "use")) return action_use;
  else if (mycmp(name, "throw")) return action_throw;
  else if (mycmp(name, "history")) return action_history;
  else if (mycmp(name, "ability")) return action_ability;
  else if (mycmp(name, "ability_1")) return action_ability_1;
  else if (mycmp(name, "ability_2")) return action_ability_2;
  else if (mycmp(name, "ability_3")) return action_ability_3;
  else if (mycmp(name, "ability_4")) return action_ability_4;
  else if (mycmp(name, "ability_5")) return action_ability_5;
  else if (mycmp(name, "ability_6")) return action_ability_6;
  else if (mycmp(name, "ability_7")) return action_ability_7;
  else if (mycmp(name, "ability_8")) return action_ability_8;
  else if (mycmp(name, "ability_9")) return action_ability_9;
  else if (mycmp(name, "ability_10")) return action_ability_10;
  else if (mycmp(name, "ability_config")) return action_ability_config;
  else if (mycmp(name, "interact")) return action_interact;
  else if (mycmp(name, "close")) return action_close;
  else if (mycmp(name, "drink")) return action_drink;
  else if (mycmp(name, "read")) return action_read;
  else if (mycmp(name, "next")) return action_next;
  else if (mycmp(name, "previous")) return action_previous;
  else if (mycmp(name, "use")) return action_use;
  else if (mycmp(name, "filter")) return action_filter;
  else if (mycmp(name, "quiver")) return action_quiver;
  else if (mycmp(name, "status")) return action_status;
  else if (mycmp(name, "label")) return action_label;
  else if (mycmp(name, "help")) return action_help;
  else if (mycmp(name, "recenter")) return action_recenter;
  else if (mycmp(name, "eat")) return action_eat;
  else if (mycmp(name, "flip")) return action_flip;
  else if (mycmp(name, "save")) return action_save;
  else if (mycmp(name, "rest")) return action_rest;
  else return action_undefined;
} /* string_to_action */



/*
  Waits until the spacebar is pressed.
*/
void wait_continue()
{
  int key;

  do
  {
    key = get_keypress();

    /* Do nothing... */
    
    /*
      This accepts cancel action (usually space), select action
      (usually enter) or literal space.
    */
  } while (key_to_action(key) != action_cancel && 
	   key_to_action(key) != action_select && 
	   key_to_action(key) != ' ');
  
  return;
} /* wait_continue */



/*
  Lets the user select a position on the current map. You must supply
  the addresses of two integers, one for each axis, to store the
  coordinates in.

  Returns: true if the user selected a position (the coordinates are stored in *y and *x)
  false if the user cancelled the action
*/
blean_t get_position_on_map(const level_t * level,
			    unsigned int * y,
			    unsigned int * x)
{
  int local_y;
  int local_x;

  int move_y;
  int move_x;

  int key;

  signed int old_top;
  signed int old_left;

  blean_t redraw;

  if ((level == NULL) || (y == NULL) || (x == NULL))
    return false;

  old_top = view_top;
  old_left = view_left;

  /* Set the initial coordinates of the cursor to where the player is */
  local_y = game->player->y;
  local_x = game->player->x;

  /* Display the cursor */
  map_cursor(true);

  while(1)
  {
    map_move(local_y - view_top, local_x - view_left);
    map_flush();

    key = get_action();
      
    if (key == action_select || key == action_wait)
    {
      map_cursor(false);

      *y = local_y;
      *x = local_x;

      view_top = old_top;
      view_left = old_left;
      draw_level();

      return true;
    }
    else if (key == action_cancel) /* If the user wants to cancel */
    {
      map_cursor(false);

      view_top = old_top;
      view_left = old_left;
      draw_level();

      return false;
    }
    /* If the user wants to move the cursor... */
    else if (key == action_nw) { move_y = -1; move_x = -1; }
    else if (key == action_n) { move_y = -1; move_x =  0; }
    else if (key == action_ne) { move_y = -1; move_x =  1; }
    else if (key == action_w) { move_y =  0; move_x = -1; }
    else if (key == action_e) { move_y =  0; move_x =  1; }
    else if (key == action_sw) { move_y =  1; move_x = -1; }
    else if (key == action_s) { move_y =  1; move_x =  0; }
    else if (key == action_se) { move_y =  1; move_x =  1; }
    else continue;
    /* For all other keys no listed above, ignore it and read
       another. Since all if clauses other than the movement related
       ones "continue", we'll only get beyond this point if it was one
       of the movement keys that was pressed. */

    /* Avoid out of bounds, then update cursor */
    if ((local_y + move_y >= 0) &&
	(local_x + move_x >= 0) &&
	(local_y + move_y < level->size_y) &&
	(local_x + move_x < level->size_x))
    {
      local_y += move_y;
      local_x += move_x;

      redraw = false;

      if (local_y - view_top < scroll_limit_y ||
	  local_y - view_top >= board_size_y - scroll_limit_y ||
	  options.autocenter)
      {
	center_view_y(game->player->location, local_y);
	redraw = true;
      }
      
      if (local_x - view_left < scroll_limit_x ||
	  local_x - view_left >= board_size_x - scroll_limit_x ||
	  options.autocenter)
      {
	center_view_x(game->player->location, local_x);
	redraw = true;
      }

      #ifdef TSL_SDL
      /*
	Special case: draw_level flushes the map and sdlui map_flush()
	blits the cursor before flushing. We need to set the cursor
	position before drawing the map or we'll end up with two (or
	more!) cursors.
      */
      redraw = true;
      map_move(local_y - view_top, local_x - view_left);
      #endif

      if (redraw)
	draw_level();
    }
  } /* while 1 */

  /* We'll never get here */
} /* get_position_on_map */



/*
  Converts an action_ability_* into the index of the power slot. All
  other actions return -1.
*/
signed int action_to_ability(const action_t action)
{
  switch (action)
  {
    case action_ability_1: return 0;
    case action_ability_2: return 1;
    case action_ability_3: return 2;
    case action_ability_4: return 3;
    case action_ability_5: return 4;
    case action_ability_6: return 5;
    case action_ability_7: return 6;
    case action_ability_8: return 7;
    case action_ability_9: return 8;
    case action_ability_10: return 9;
    default: return -1;
  }
} /* action_to_ability */



/*
  Displays PROMPT and lets the user enter a number.

  Returns: The number, or -1 on bad input.
*/
signed int read_number(const char * prompt)
{
  char buf[10];
  unsigned int ret;

  read_string(prompt, buf, 5);

  if (is_number(buf) == false)
  {
    return -1;
  }

  ret = atoi(buf);
  
  return ret;
} /* read_number */



/*
  Reads a string of length MAXLEN into the memory area pointed to by
  TARGET, using MSG as prompt. The caller must make sure that target
  is large enough, etc...
*/
void read_string(const char * msg, char * target, const unsigned int maxlen)
{
  unsigned int p;
  unsigned int in;

  /* We need to temporarily display a cursor. */
  mb_cursor(true);

  /*
    Display message at upper left of the message buffer, clear rest
    of the line. The input will be shown on line 2.
  */
  /*wclrtoeol(message_bar);*/

  p = 0;

  do
  {
    target[p] = '\0';

    mb_erase();
    mb_move(0, 0);
    mb_addstr(msg);

    mb_move(1, 0);
    mb_addstr(target);
    mb_move(1, p);
    mb_flush();

    in = get_keypress();

    if (in == '\n')
      break;
    else if (in == kt_backspace && (p > 0))
      p--;
    else if ((isgraph(in) || (in == ' ')) && (p < maxlen))
      target[p++] = in;
  } while (1);
  
  /* Hide the cursor again. */
  mb_cursor(false);

  return;
} /* read_string */
