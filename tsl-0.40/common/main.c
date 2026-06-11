#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <sys/stat.h>
#include <ctype.h>

#include "main.h"
#include "stuff.h"
#include "level.h"
#include "ui.h"
#include "game.h"
#include "debug.h"
#include "item.h"
#include "magic.h"
#include "player.h"
#include "treasure.h"
#include "unique.h"
#include "places.h"
#include "inventory.h"
#include "help.h"
#include "vweapon.h"
#include "saveload.h"
#include "shapeshf.h"
#include "modbuild.h"
#include "options.h"
#include "attrs.h"
#include "find.h"
#include "keymap.h"
#include "dwiminv.h"
#include "browser.h"

const char * bug_string = "BUG";

/* These are the letters to be used to identify items in the inventory */
char * item_letters = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890\0";

char * damage_type_label[] =
{
  "general",
  "fire",
  "cold",
  "acid",
  "electricity",
  "poison"
};



signed int move_mat[DIRECTIONS][2] =
{
  { 0,  0},
  {-1,  0},
  {-1, +1},
  { 0, +1},
  {+1, +1},
  {+1,  0},
  {+1, -1},
  { 0, -1},
  {-1, -1}
};



/*
  Starts the program, reads commandline arguments, etc.
 */
int main(int argc, char * argv[])
{
  int i;
  int y;
  int x;

  /*
    TODO: We won't enable this until the issues with saving/loading
    strings has been solved.
  */
  atexit(&check_unfree_memory);

  /* Set default options and load the default keymap. */
  set_default_options();
  default_keymap();

  /* Read the config file. This will possibly set some custom options. */
  parse_config_file();
  
  for (i = 0; i < argc; i++)
  {
    if (mycmp(argv[i], "--force-get-name"))
      options.forcegetname = true;
  }

  init_static();

  level_changed = false;

  game = NULL;

  init_ui();

  game = try_to_load_game();

  if (game != NULL)
  {
    /* This is a resumed game */
    
    char line[80];

    sprintf(line, "Hello %s, welcome back to TSL!", game->native->name_only);
    queue_msg(line);
  }
  else
  {
    char line[80];

    /* This is a new game */
    game = start_new_game();
    init_levels();

    for (i = 0; i < argc; i++)
    {
      if (mycmp(argv[i], "--test-maps"))
	perform_map_test();
      else if (mycmp(argv[i], "--browser"))
	module_browser();
      else if (mycmp(argv[i], "--item-gen"))
	item_gen_test();
    }

    /*
      Create the player. We must set this to null first, since item
      equipment (and maybe some other things) will try to access the
      global player pointer (to check for blindness, etc)!
    */
    game->native = game->player = NULL;
    game->native = game->player = create_character();

    sprintf(line, "Hello %s, welcome to TSL!", game->native->name_only);
    queue_msg(line);
    queue_msg("Press ? for help.");

    change_level(LEVEL_START);

    if (find_spot(game->player->location, &y, &x, find_start) == false)
      find_spot(game->player->location, &y, &x, find_unoccupied);

    /*
      We must do this before we set the creature coordinates or the
      "start" tile will end up in seen memory instead of a proper
      floor tile (there are very few ways to actually see the start
      tile - being blinded before moving, blink away - but we want to
      be sure). We already have the start coordinates in y, x so we
      can safely lose the start tile now.
    */
    replace_tile(game->player->location, tile_internal_start, tile_floor);

    set_creature_coordinates(game->player, y, x);

    /* Save the start coordinates as default destination for "Recall". */
    game->recall_location = LEVEL_START;
    game->recall_y = y;
    game->recall_x = x;
    
    /* We set this last so we only measure the time spent in-game. */
    game->started = time(NULL);
    
    /*
      RFE: There should be a counter in game_t that is updated
      between saves and keeps track of actual *game time*.
    */
  }

  /*
    Ok! Regardless if we restored a saved game or started a new one, we're ready to go.
  */
  
  /*
    RFE: This could be saved in game_t and preserved between
    sessions, but would it really be an improvement?
  */
  center_view_y(get_current_level(), game->player->y);
  center_view_x(get_current_level(), game->player->x);

  look_at_location(game->player, get_current_level(), game->player->y, game->player->x, false);

  draw_level();

  play();

  shutdown_everything();
  
  return 0;
} /* main */



/*
  Terminates program with an "out of memory" error message.
*/
void out_of_memory()
{
  shutdown_everything();
  printf("Out of memory.");
  exit(1);
} /* out_of_memory */


 
/*
  Frees all static content.
*/
void del_static(void)
{
  free(options.default_name);
  options.default_name = NULL;

  del_all_modules();
/*  del_all_help_pages();*/
  del_virtual_weapons();
  del_all_attr_info();
  del_tiles();

  return;
} /* del_static */



/*
  This must be called before exiting the program. It will free all
  reserved memory and try to ensure a clean shutdown.
*/
void shutdown_everything()
{
  end_game(game);
  game = NULL;

  del_static();

  shutdown_ui();
 
  return;
} /* shutdown_everything */



/*
  Initializes static game data.
*/
void init_static(void)
{
  next_uid = 1;

  /* Seed the random number generator. */
  tslrnd_seed();

  /*
    These aren't really static, but we only need to reset them
    once.
  */
  mem_alloc.chars = 0;
  mem_alloc.levels = 0;
  mem_alloc.tiles = 0;
  mem_alloc.creatures = 0;
  mem_alloc.items = 0;
  mem_alloc.effects = 0;
  mem_alloc.attributes = 0;
  mem_alloc.games = 0;

  map_test.calls = 0;
  map_test.generated = 0;
  map_test.traversable = 0;
  map_test.connected = 0;
  map_test.exits = 0;

  init_browsers();

  /* Set up some stuff that will remain static over the whole game. */
  init_modules();
  /*init_help_pages();*/
  init_tiles();
  init_all_attr_info();
  init_virtual_weapons();
  init_uniques();

  return;
} /* init_static */



/*
  Checks if there is allocated memory that hasn't been freed; if there
  is, prints the memory information.
*/
void check_unfree_memory()
{
  if (mem_alloc.chars ||
      mem_alloc.creatures ||
      mem_alloc.items ||
      mem_alloc.levels ||
      mem_alloc.tiles ||
      mem_alloc.traps || 
      mem_alloc.messages ||
      mem_alloc.effects ||
      mem_alloc.attributes ||
      mem_alloc.games)
  {
    print_memory_usage();
  }

  return;
} /* check_unfree_memory */
