#include <stdlib.h>
#include <string.h>

#include "stuff.h"
#include "actions.h"
#include "debug.h"
#include "creature.h"
#include "level.h"
#include "item.h"
#include "player.h"
#include "game.h"
#include "ui.h"
#include "inventory.h"
#include "effect.h"
#include "magic.h"
#include "places.h"
#include "fov.h"
#include "saveload.h"
#include "modbuild.h"
#include "shapeshf.h"
#include "help.h"
#include "memory.h"
#include "area.h"
#include "ai.h"
#include "losegame.h"
#include "options.h"
#include "find.h"
#include "dwiminv.h"


/*
  Generates a report based on the information in MEM_ALLOC (main.h). A
  pointer to the report is returned, and should be free()d after use.
 */
char * generate_memory_usage_summary()
{
  char temp[500];
  char t[100];
  char * ret;

  int bytes;
  int total;

  total = 0;

  sprintf(temp, "Memory Usage\n\n");

  bytes = sizeof(char) * mem_alloc.chars;
  total += bytes;
  sprintf(t, "%d chars\t  (%d bytes)\n", mem_alloc.chars, bytes);
  strcat(temp, t);

  bytes = sizeof(creature_t) * mem_alloc.creatures;
  total += bytes;
  sprintf(t, "%d creature_t\t  (%d bytes)\n", mem_alloc.creatures, bytes);
  strcat(temp, t);

  bytes = sizeof(item_t) * mem_alloc.items;
  total += bytes;
  sprintf(t, "%d item_t\t  (%d bytes)\n", mem_alloc.items, bytes);
  strcat(temp, t);

  bytes = sizeof(level_t) * mem_alloc.levels;
  total += bytes;
  sprintf(t, "%d level_t\t  (%d bytes)\n", mem_alloc.levels, bytes);
  strcat(temp, t);

  bytes = sizeof(tile_info_t) * mem_alloc.tiles;
  total += bytes;
  sprintf(t, "%d tile_info_t\t  (%d bytes)\n", mem_alloc.tiles, bytes);
  strcat(temp, t);

  bytes = sizeof(trap_t) * mem_alloc.traps;
  total += bytes;
  sprintf(t, "%d traps\t  (%d bytes)\n", mem_alloc.traps, bytes);
  strcat(temp, t);

  bytes = sizeof(message_t) * mem_alloc.messages;
  total += bytes;
  sprintf(t, "%d message_t\t  (%d bytes)\n", mem_alloc.messages, bytes);
  strcat(temp, t);

  bytes = sizeof(effect_t) * mem_alloc.effects;
  total += bytes;
  sprintf(t, "%d effect_t\t  (%d bytes)\n", mem_alloc.effects, bytes);
  strcat(temp, t);

  bytes = sizeof(attribute_t) * mem_alloc.attributes;
  total += bytes;
  sprintf(t, "%d attribute_t\t  (%d bytes)\n", mem_alloc.attributes, bytes);
  strcat(temp, t);

  bytes = sizeof(game_t) * mem_alloc.games;
  total += bytes;
  sprintf(t, "%d game_t\t  (%d bytes)\n", mem_alloc.games, bytes);
  strcat(temp, t);

  sprintf(t, "\n%d bytes total\n", total);
  strcat(temp, t);

  ret = mydup(temp);
  if (temp == NULL) out_of_memory();
  mem_alloc.chars += strlen(ret) + 1;

  return ret;
} /* generate_memory_usage_summary */



/*
  Activates the debug menu.
 */
void debug_menu()
{
  int input;
  char * temp;
  item_t * item;
  signed int number;
  char buf[20];

  char shit[800];
  
  st_erase();
  
  strcpy(shit,
	  "Debug Menu\n"
	  "\n"
	  "a) memory usage\n"
	  "A) set creature alignment\n"
	  "b) map test data\n"
	  "C) cloud test\n"
	  "d) inspect character dump\n"
	  "e) identify item\n"
	  "f) create item\n"
	  "F) debug FOV\n"
	  "g) boost character\n"
	  "i) inspect creature\n"
	  "k) kill all creatures\n"
	  "m) (mis)lead enemies\n"
	  "M) display raw map\n"
	  "o) obelisk\n"
	  "p) pathfinding\n"
	  "r) reveal map\n"
	  "s) set attribute\n"
	  "S) controlled shapeshift\n   (THIS WILL CRASH)\n"
	  "u) undetect all creatures\n"
	  "w) make a wish\n"
	  "z) world travel\n"
    );
  
  st_addstr(shit);

  st_flush();

  input = get_keypress();
  
  switch (input)
  {
    case 'a':
      temp = generate_memory_usage_summary();
      
      display_text(temp);
      queue_msg("Displaying memory usage...");
      msgflush_wait();
      clear_msgbar();
      
      mem_alloc.chars -= strlen(temp) + 1;
      free(temp);
      return;

    case 'A':
      set_alignment();
      return;
     
    case 'b':
      temp = generate_map_test_summary();
      
      display_text(temp);
      queue_msg("Displaying map test data...");
      msgflush_wait();
      clear_msgbar();
      
      free(temp);
      return;

    case 'C':
      cloud_test();
      return;
      
    case 'd':
      summary_test();
      return;
      
    case 'e':
      item = dwim_select(game->player, NULL, NULL);
      
      if (item != NULL)
	identify(item);
      return;
      
    case 'f':
      read_string("Which item#?", buf, 10);
      number = atoi(buf);
      
      item = build_item(number);
      
      if (item != NULL)
      {
	del_item(attach_item_to_creature(game->player, item));
      }
      return;

    case 'F':
      if (options.debug_fov)
	options.debug_fov = false;
      else
	options.debug_fov = true;

      set_creature_coordinates(game->player, game->player->y, game->player->x);
      return;
      
    case 'S':
      read_string("Which monster#?", buf, 10);
      number = atoi(buf);
      return;
      
    case 'g':
      set_attr(game->player, attr_health, 300);
      set_attr(game->player, attr_ep_max, 300);
      set_attr(game->player, attr_ep_current, 300);
      set_attr(game->player, attr_vision, 10);
      set_attr(game->player, attr_speed, 200);
      return;
      
    case 'i':
      inspect_creature();
      return;

    case 'k':
      kill_all_creatures(get_current_level());
      return;

    case 'm':
      mislead_enemies();
      return;

    case 'o':
      capsule(game->player, game->player->y, game->player->x);
      draw_level();
      return;

    case 'p':
      path_test();
      return;
      
    case 'r':
      reveal_level(get_current_level());
      return;

    case 'M':
      display_raw_map();
      return;

    case 's':
      set_attribute();
      return;

    case 'w':
      wish(game->player, NULL, 0);
      return;

    case 'u':
      undetect_everyone();
      return;

    case 'z':
      world_jump();
      return;
  }

  return;
} /* debug_menu */



/*
  Performs an extensive map generation test.
*/
void perform_map_test()
{
  level_t * level;
  unsigned int i;
  char * summary;
  level_t * temp;
  char line[100];

  level = alloc_level(0, "TEST MAP");

  temp = level;

  temp->size_y = 60;
  temp->size_x = 80;
/*  temp->starting_module = LAIR_LURKER;*/
/*  temp->wanted_modules = MODULE_CAVE | MODULE_NARROW;*/
  temp->wanted_modules = MODULE_CELLS | MODULE_ROOM;

  temp->wanted_challenge = 100;
  encounter_table(temp, encounter_dart_trap, 10);

  temp->link[0] = LEVEL_DUNGEON;


  for (i = 0; i < 50; i++)
  {
    st_erase();
    sprintf(line, "Performing map generation test (%d), please wait...", i + 1);
    st_addstr(line);
    st_flush();

    build_dungeon(level);
  }

  del_level(level);
  
  summary = generate_map_test_summary();
  
  shutdown_everything();
  
  printf(summary);
  free(summary);

  exit(0);
} /* perform_map_test */



void undetect_everyone()
{
  level_t * l;
  unsigned int i;

  l = get_current_level();

  for (i = 0; i < l->creatures; i++)
  {
    if (l->creature[i] != NULL && 
	is_player(l->creature[i]) == false)
      l->creature[i]->detected = false;
  }

  draw_level();

  return;
} /* undetect_everyone */




/*
  Generates a report based on the information in the MAP_TEST
  structure (see main.h). The pointer returned must be free()d.
 */
char * generate_map_test_summary()
{
  char temp[5000];
  char * ret;
//  unsigned int i;
//char x[100];

  sprintf(temp,
	  "%d calls\n"
	  "%d generated\n"
	  "\n"
	  "%d too small\n"
	  "%d not connected\n"
	  "%d lacked exits\n"
	  "\n"
	  "%.2f %% were valid\n",
	  map_test.calls,
	  map_test.generated,
	  map_test.traversable,
	  map_test.connected,
	  map_test.exits,
	  (map_test.calls * 100) / (float)map_test.generated);
  
/*  for (i = 0; i < MODULES; i++)
  {
    if (module[i] != NULL)
    {
      sprintf(x, "%d: %d\n", i, module[i]->used);
      strcat(temp, x);
    }
    }*/

  ret = mydup(temp);
  if (ret == NULL) out_of_memory();
  
  return ret;
} /* generate_map_test_summary */



/*
  Prints how much memory is used to STDIN.
 */
void print_memory_usage()
{
  char * summary;

  summary = generate_memory_usage_summary();
  printf(summary);
  free(summary);

  return;
} /* print_memory_usage */



void reveal_level(level_t * level)
{
  unsigned int y;
  unsigned int x;
  unsigned int i;
  creature_t * temp;

/*  for (y = 0; y < level->size_y; y++)
    for (x = 0; x < level->size_x; x++)
    level->lit[y][x] = true;*/
  
  for (y = 0; y < FOV_RANGE * 2 + 1; y++)
    for (x = 0; x < FOV_RANGE * 2 + 1; x++)
      game->player->fov[y][x] = true;

  /* Detect all enemies on the map */
  for (i = 0; i < level->creatures; i++)
  {
    temp = level->creature[i];

    if (temp != NULL)
      temp->detected = true;
  }

  update_level_memory(level);
  draw_level();

  return;
} /* reveal_level */



void inspect_creature()
{
  unsigned int y;
  unsigned int x;

  creature_t * creature;

  queue_msg("Select a location...");
  msgflush_nowait();
  
  if (get_position_on_map(get_current_level(), &y, &x) == false)
    return;

  creature = find_creature(get_current_level(), y, x);

  if (creature == NULL)
  {
    queue_msg("Nothing there!");
    return;
  }

  display_stats(creature);
  
  queue_msg("Inspecting creature...");
  msgflush_wait();

  return;
} /* inspect_creature */



/*
  Removes all creatures from LEVEL
*/
void kill_all_creatures(level_t * level)
{
  unsigned int i;

  if (level == NULL)
    return;

  queue_msg("Argh!");

  for (i = 0; i < level->creatures; i++)
  {
    if (level->creature[i] != game->player)
      kill_creature(level->creature[i], true);
  }

/*  build_light_matrix(level);*/
  draw_level();

  return;
} /* kill_all_creatures */



/*
  This is for AI testing. The user will be able to select a location
  on the map where all enemies will be directed.
*/
void mislead_enemies()
{
  unsigned int i;
  unsigned int y;
  unsigned int x;
  
  queue_msg("Where do you wish to send them?");
  msgflush_nowait();
  
  if (get_position_on_map(get_current_level(), &y, &x) == false)
    return;
  
  for (i = 0; i < get_current_level()->creatures; i++)
  {
    if ((get_current_level()->creature[i] != game->player) &&
	(get_current_level()->creature[i] != NULL))
    {
      set_enemy_target(get_current_level()->creature[i], y, x);
    }
  }
} /* mislead_enemies */



/*
  Lets the player select a creature on the map and an attribute we
  wish to change (given as the attribute index).
*/
void set_attribute()
{
  unsigned int y;
  unsigned int x;
  unsigned int attr_index;
  signed int new_attr;

  char buf[10];
  char line[80];
  creature_t * creature;

  queue_msg("Select creature...");
  msgflush_nowait();
  
  if (get_position_on_map(get_current_level(), &y, &x) == false)
    return;

  creature = find_creature(get_current_level(), y, x);

  if (creature == NULL)
  {
    queue_msg("Nothing there!");
    return;
  }

  read_string("Which attribute?", buf, 10);
  attr_index = atoi(buf);
  
  if ((attr_index >= ATTRIBUTES) ||
      (attr_info[attr_index] == NULL))
  {
    queue_msg("Bad attribute!");
    return;
  }

  sprintf(line, "Set %s (%d) to what?", attr_info[attr_index]->name, attr_base(creature, attr_index));
  read_string(line, buf, 10);
  new_attr = atoi(buf);

  set_attr(creature, attr_index, new_attr);

  queue_msg("Thank you for utilising the debug system.");

  return;
} /* set_attribute */



/*
  Lets the player select a creature on the map and set its alignment.
*/
void set_alignment()
{
  unsigned int y;
  unsigned int x;
/*  signed int new_align;*/

  char buf[10];
  char line[80];
  creature_t * creature;

  queue_msg("Select creature...");
  msgflush_nowait();
  
  if (get_position_on_map(get_current_level(), &y, &x) == false)
    return;

  creature = find_creature(get_current_level(), y, x);

  if (creature == NULL)
  {
    queue_msg("Nothing there!");
    return;
  }

  sprintf(line, "Set alignment to what?");
  read_string(line, buf, 10);
/*  new_align = atoi(buf);*/

/*  creature->alignment = new_align;*/

  return;
} /* set_alignment */



/*
  
*/
void cloud_test()
{
  unsigned int y;
  unsigned int x;
  unsigned int amount;

  char buf[10];

  area_t * cloud;
  area_t * temp;

  read_string("Cloud size?", buf, 10);
  amount = atoi(buf);

  queue_msg("Where?");
  msgflush_nowait();
  
  if (get_position_on_map(get_current_level(), &y, &x) == false)
    return;

  cloud = new_area(y, x);

  area_cloud(cloud, get_current_level(), amount);

  for (temp = cloud; temp != NULL; temp = temp->next)
  {
    draw_missile(temp->y, temp->x, missile_frost, 0, 0);
  }

  del_area(cloud);

  queue_msg("One cloud, here you go.");
  msgflush_wait();

  return;
} /* cloud_test */



/*
  Lets the player jump to any level.
*/
void world_jump()
{
  char line[100];
  unsigned int i;
  char input;
  char count;

  st_erase();
  
  count = 'a';

  for (i = 0; i < LEVELS; i++)
  {
    if (game->level_list[i] == NULL)
      continue;
    
    sprintf(line, "\n%c) %s", count, game->level_list[i]->name);
    st_addstr(line);

    count++;
  }
  
  st_flush();

  while (1)
  {
    queue_msg("Jump to which level?");
    msgflush_nowait();

    input = get_keypress();
   
    if (input == ' ')
    {
      clear_msgbar();
      return;
    }
 
    count = 'a';

    for (i = 0; i < LEVELS; i++)
    {
      if (game->level_list[i] == NULL)
	continue;

      if (count == input)
      {
	change_level(i);
	find_random_free_spot(game->player);
	center_view_y(get_current_level(), game->player->y);
	center_view_x(get_current_level(), game->player->x);
	look_at_location(game->player, get_current_level(), game->player->y, game->player->x, true);
	draw_level();
	
	return;
      }
      else
      {
	count++;
      }
    }

    queue_msg("Unknown level.");
    msgflush_wait();
  }

  return;
} /* world_jump */



void print_modules()
{
  unsigned int i;
  unsigned int y;
  module_t * mod;
  
  for (i = 0; i < MODULES; i++)
  {
    mod = module[i];
    
    if (mod == NULL)
      continue;
    else
      printf("\n\n");

    for (y = 0; y < mod->lines; y++)
    {
      printf("%s\n", mod->data[y]);
    }
  }

  exit(0);

  return;
} /* print_modules */




void module_browser()
{
/*  unsigned int i;
  module_t * m;
  unsigned int y;
  level_t * level;
  int k;

  for (i = 0; i < 100; i++)
  {
    werase(stdscr);
    wprintw(stdscr, "Performing map generation test (%d), please wait...", i + 1);
    wrefresh(stdscr);

    level = alloc_level("TEST MAP");
    level->wanted_modules = 255;
    build_dungeon(level);
    del_level(level);
  }

  i = 0;

  while (1)
  {
    erase();

    printw("Module: %d\n\n", i);

    m = module[i];

    if (m == NULL)
    {
      printw("NULL");
    }
    else
    {
      printw("Used: %d\n\n", m->used);

      for (y = 0; y < m->lines; y++)
      {
	printw("%s\n", m->data[y]);
      }
    }
    
    refresh();

    k = getch();

    switch (k)
    {
      case '+':
	if (i < MODULES - 1)
	  i++;
	break;

      case '-':
	if (i > 0)
	  i--;
	break;

      case 'q':
	shutdown_everything();
	exit(0);
    }
    }*/
} /* module_browser */




void item_gen_test(void)
{
  unsigned int count[ITEMS];
  unsigned int invalid;
  unsigned int i;
  unsigned int t;
  char line[100];
  item_t * item;

  invalid = 0;

  for (i = 0; i < ITEMS; i++)
    count[i] = 0;

  for (i = 0; i < item_table_max; i++)
  {
    sprintf(line, "Item table %d...\n", i);
    /*refresh();*/

    t = 2000;

    while (t-- > 0)
    {
      item = random_treasure(i);
      
      if (item == NULL)
      {
	invalid++;
	continue;
      }
      
      if (item->item_number < ITEMS)
	count[item->item_number]++;
      else
	invalid++;

      del_item(item);
    }
  }

  shutdown_ui();

  for (i = 0; i < ITEMS; i++)
  {
    item = build_item(i);

    if (item == NULL)
      continue;

    fprintf(stderr, "%s: %d\n", item->single_id_name, count[i]);
    del_item(item);
  }

  fprintf(stderr, "INVALID: %d\n", invalid);

  shutdown_everything();
  exit(0);
  
  return;
} /* item_gen_test */



void display_raw_map()
{
  unsigned int y;
  unsigned int x;
  level_t * level;

  level = game->player->location;
  
  for (y = 0; y < level->size_y; y++)
    for (x = 0; x < level->size_x; x++)
      level->memory[y][x] = level->map[y][x];
  
  draw_level();
  queue_msg("Displaying raw map...");
  msgflush_wait();

  return;
} /* display_raw_map */



void path_test(void)
{
  unsigned int start_y;
  unsigned int start_x;
  unsigned int end_y;
  unsigned int end_x;

  queue_msg("Startpoint?");
  msgflush_nowait();
  
  if (get_position_on_map(get_current_level(), &start_y, &start_x) == false)
    return;

  queue_msg("Endpoint?");
  msgflush_nowait();
  
  if (get_position_on_map(get_current_level(), &end_y, &end_x) == false)
    return;

  return;
} /* path_test */



void summary_test()
{
/*  char * dump;

  unsigned int len;
  unsigned int start;

  dump = build_character_dump();

  len = strlen(dump);

  for (start = 0; start < len; )
  {
    erase();
    waddnstr(stdscr, &dump[start], 150);
    refresh();

    start += 150;
    
    getch();
  }
  
  free(dump);

  draw_level();

  return;*/
}
