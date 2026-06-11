#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "modbuild.h"
#include "stuff.h"
#include "find.h"


void del_all_modules(void)
{
  unsigned int i;

  for (i = 0; i < MODULES; i++)
  {
    del_module(module[i]);
    module[i] = NULL;
  }

  return;
} /* del_all_modules */



void del_module(module_t * m)
{
  unsigned int i;

  if (m == NULL)
    return;
  
  for (i = 0; i < m->lines; i++)
    free(m->data[i]);
  
  free(m->data);
  
  free(m);
  
  return;  
} /* del_module */



module_t * new_module(void)
{
  module_t * temp;
  unsigned int i;

  temp = new_module_noreg();

  for (i = 0; i < MODULES; i++)
  {
    if (module[i] == NULL)
    {
      module[i] = temp;
      return temp;
    }
  }

  out_of_memory();

  return NULL;
} /* new_module */



module_t * new_module_noreg(void)
{
  module_t * temp;

  temp = malloc(sizeof(module_t));

  if (temp == NULL)
    out_of_memory();

  temp->data = NULL;
  temp->lines = 0;

  temp->category = 0;
  temp->rarity = 0;
  temp->used = 0;

  return temp;
} /* new_module */



void add_line(module_t * mod, const char * line)
{
  if (mod == NULL)
    return;

  mod->lines++;

  mod->data = realloc(mod->data, mod->lines * sizeof(char *));
  mod->data[mod->lines - 1] = mydup(line);

  if (mod->lines > 1)
  {
    if (strlen(mod->data[mod->lines - 1]) != strlen(mod->data[0]))
      abort();
  }

  return;
} /* add_line */



void build_chapel(level_t * level)
{
  unsigned int x;
  unsigned int i;
  unsigned int chunks;
  unsigned int valid_chunk[20];

  if (level == NULL)
    return;

  chunks = 0;

  for (i = 0; i < MODULES; i++)
  {
    if (module[i] != NULL && module[i]->category == MODULE_CHAPEL)
    {
      valid_chunk[chunks] = i;
      chunks++;
    }
  }

  x = level->size_x;

  x -= strlen(module[valid_chunk[0]]->data[0]);

  put_module(level, module[valid_chunk[0]], 0, x);

  while (x > 30)
  {
    i = 2 + tslrnd() % MAX(1, chunks - 2);
    x -= strlen(module[valid_chunk[i]]->data[0]);
    put_module(level, module[valid_chunk[i]], 0, x);
  }

  x -= strlen(module[valid_chunk[1]]->data[0]);
  put_module(level, module[valid_chunk[1]], 0, x);

  return;
} /* build_chapel */



void build_module_dungeon(level_t * level)
{
  int i;
  int r;
  int y;
  int x;
  int length;
  char find;

  chunk_bitmask_t wanted_modules;

  if (level == NULL)
    return;

  wanted_modules = level->wanted_modules;

  for (y = 0; y < level->size_y; y++)
    for (x = 0; x < level->size_x; x++)
      set_tile(level, y, x, tile_void);

  put_special_module(level, level->starting_module);

  r = 500;
  
  while (r-- > 0)
  {
    /* TODO: I have no idea why this is needed. */
    if (find_spot(level, &y, &x, find_unconn) == false)
      break;
     
    i = 0;
    length = 0;

    if (level->map[y][x] == tile_internal_con_n ||
	level->map[y][x] == tile_internal_con_s)
    {
      while (on_map(level, y, x + i) &&
	     level->map[y][x + i] == level->map[y][x])
      {
	length++;
	i++;
      }
    }
    else if (level->map[y][x] == tile_internal_con_e ||
	     level->map[y][x] == tile_internal_con_w)
    {
      while (on_map(level, y + i, x) &&
	     level->map[y + i][x] == level->map[y][x])
      {
	length++;
	i++;
      }
    }
    else
      exit(1);

    if (level->map[y][x] == tile_internal_con_n)
      find = 's';
    else if (level->map[y][x] == tile_internal_con_e)
      find = 'w';
    else if (level->map[y][x] == tile_internal_con_s)
      find = 'n';
    else if (level->map[y][x] == tile_internal_con_w)
      find = 'e';
    else break;
    
    if (attach_module(level, y, x, length, find, wanted_modules) == false)
    {
      /* TODO: Seal the connection */
      continue;
    }
  }

  /* Clean up */
  replace_tile(level, tile_void, tile_wall);
  replace_tile(level, tile_internal_con_n, tile_wall);
  replace_tile(level, tile_internal_con_e, tile_wall);
  replace_tile(level, tile_internal_con_s, tile_wall);
  replace_tile(level, tile_internal_con_w, tile_wall);

  for (y = 0; y < level->size_y; y++)
  {
    set_tile(level, y, 0, tile_wall);
    set_tile(level, y, level->size_x - 1, tile_wall);
  }

  for (x = 1; x < level->size_x - 1; x++)
  {
    set_tile(level, 0, x, tile_wall);
    set_tile(level, level->size_y - 1, x, tile_wall);
  }

  return;
} /* build_module_dungeon */




blean_t attach_module(level_t * level,
		      const unsigned int connect_y,
		      const unsigned int connect_x,
		      const unsigned int connect_len,
		      const char connection,
		      const chunk_bitmask_t wanted_modules)
{
  unsigned int mi;
  unsigned int l;
  unsigned int matches;
  unsigned int * filter;
  unsigned int * off_y;
  unsigned int * off_x;
  unsigned int temp_y;
  unsigned int temp_x;

  matches = 0;
  filter = NULL;
  off_y = NULL;
  off_x = NULL;

  for (mi = 1; mi < MODULES; mi++)
  {
    if (module[mi] == NULL)
      continue;
    
    if (module_allowed(level, module[mi],
		       connect_y, connect_x,
		       connect_len, connection,
		       &temp_y, &temp_x,
		       wanted_modules))
    {
      matches++;
      filter = realloc(filter, matches * sizeof(unsigned int));
      off_y = realloc(off_y, matches * sizeof(unsigned int));
      off_x = realloc(off_x, matches * sizeof(unsigned int));
      filter[matches - 1] = mi;
      off_y[matches - 1] = temp_y;
      off_x[matches - 1] = temp_x;
    }
  }

  if (matches > 0)
  {
    l = tslrnd() % matches;
    put_module(level, module[filter[l]], connect_y - off_y[l], connect_x - off_x[l]);
  }

  free(filter);
  free(off_y);
  free(off_x);

  if (matches > 0)
    return true;
  else
    return false;
} /* attach_module */



blean_t module_allowed(level_t * level, module_t * mod,
		       const unsigned int connect_y,
		       const unsigned int connect_x,
		       const unsigned int connect_len,
		       const char connection,
		       unsigned int * off_y,
		       unsigned int * off_x,
		       const chunk_bitmask_t wanted_modules)
{
  unsigned int y;
  unsigned int x;
  unsigned int h;
  unsigned int w;
  unsigned int l;
  blean_t connection_match;

  if (mod == NULL)
    return false;

  if ((off_y == NULL) || (off_x == NULL))
    return false;

  if ((mod->category & wanted_modules) == 0)
    return false;

  if (tslrnd() % (mod->rarity + 1) > 0)
    return false;

  connection_match = false;

  h = mod->lines;
  w = strlen(mod->data[0]);
  
  for (y = 0; y < h; y++)
  {
    for (x = 0; x < w; x++)
    {
      l = 0;
      
      while ((x + l < w) && (mod->data[y][x + l] == connection))
      {
	l++;
	
	if ((l == connect_len) &&
	    ((x + l + 1 >= w) || (mod->data[y][x + l + 1] != connection)) )
	{
	  *off_y = y;
	  *off_x = x;
	  connection_match = true;
	  break;
	}
      }

      l = 0;
      
      while ((y + l < h) && (mod->data[y + l][x] == connection))
      {
	l++;
	
	if ((l == connect_len) &&
	    ((y + l + 1 >= h) ||  (mod->data[y + l + 1][x] != connection)) )
	{
	  *off_y = y;
	  *off_x = x;
	  connection_match = true;
	  break;
	}
      }

      if (connection_match)
	break;
    }

    if (connection_match)
      break;
  }

  if (connection_match == false)
    return false;

  for (y = 0; y < h; y++)
  {
    for (x = 0; x < w; x++)
    {
      if (on_map(level, connect_y - *off_y + y, connect_x - *off_x + x) == false)
	return false;

      if ((mod->data[y][x] != ' ') &&
	  (level->map[connect_y - *off_y + y][connect_x - *off_x + x] != tile_void) &&
	  (level->map[connect_y - *off_y + y][connect_x - *off_x + x] != tile_internal_con_n) &&
	  (level->map[connect_y - *off_y + y][connect_x - *off_x + x] != tile_internal_con_e) &&
	  (level->map[connect_y - *off_y + y][connect_x - *off_x + x] != tile_internal_con_s) &&
	  (level->map[connect_y - *off_y + y][connect_x - *off_x + x] != tile_internal_con_w))
      {
	return false;
      }
    }
  }

  return true;
} /* module_allowed */



void mirror_module(module_t * old)
{
  unsigned int y;
  unsigned int x;
  unsigned int w;
  unsigned int h;
  module_t * mod;
  char * temp;

  if (old == NULL)
    return;

  if (old->data == NULL)
    abort();

  mod = new_module();

  mod->category = old->category;
  mod->rarity = old->rarity;

  w = strlen(old->data[0]);
  h = old->lines;

  temp = malloc(w + 1);
  temp[w] = '\0';

  for (y = 0; y < h; y++)
  {
    for (x = 0; x < w; x++)
      temp[x] = old->data[y][w - x - 1];

    add_line(mod, temp);
  }

  free(temp);
  
  for (y = 0; y < h; y++)
  {
    for (x = 0; x < w; x++)
    {
      switch(mod->data[y][x])
      {
	case 'e': mod->data[y][x] = 'w'; break;
	case 'w': mod->data[y][x] = 'e'; break;
	default: break;
      }
    }
  }

  return;
} /* mirror_module */



void rotate_module(module_t * old)
{
  module_t * mod;
  unsigned int i;
  unsigned int y;
  unsigned int x;
  unsigned int w;
  unsigned int h;
  char * temp;

  if (old == NULL)
    return;

  for (i = 0; i < 3; i++)
  {
    if (old->data == NULL)
      abort();

    mod = new_module();

    mod->category = old->category;
    mod->rarity = old->rarity;

    w = strlen(old->data[0]);
    h = old->lines;

    temp = malloc(h + 1);

    for (x = 0; x < w; x++)
    {
      for (y = 0; y < h; y++)
	temp[y] = old->data[h - 1 - y][x];

      temp[h] = '\0';

      add_line(mod, temp);
    }

    free(temp);

    for (x = 0; x < h; x++)
    {
      for (y = 0; y < w; y++)
      {
	switch(mod->data[y][x])
	{
	  case 'n': mod->data[y][x] = 'e'; break;
	  case 'e': mod->data[y][x] = 's'; break;
	  case 's': mod->data[y][x] = 'w'; break;
	  case 'w': mod->data[y][x] = 'n'; break;
	  case '|': mod->data[y][x] = '-'; break;
	  case '-': mod->data[y][x] = '|'; break;
	  default: break;
	}
      }
    }

    mirror_module(mod);

    old = mod;
  }

  return;
} /* rotate_module */



void put_module(level_t * level,
		module_t * mod,
		const unsigned int t,
		const unsigned int l)
{
  unsigned int y;
  unsigned int x;
  unsigned int h;
  unsigned int w;
  tile_t tile;

  if ((level == NULL) || (mod == NULL))
    return;

  mod->used++;
  
  h = mod->lines;
  w = strlen(mod->data[0]);

  for (y = 0; y < h; y++)
  {
    for (x = 0; x < w; x++)
    {
      if (on_map(level, t + y, l + x) == false)
	continue;

      if ((mod->data[y][x] == ' ') || (mod->data[y][x] == 'v'))
      {
	continue;
      }
      else if ((level->map[t + y][l + x] == tile_internal_con_n) ||
	       (level->map[t + y][l + x] == tile_internal_con_e) ||
	       (level->map[t + y][l + x] == tile_internal_con_s) ||
	       (level->map[t + y][l + x] == tile_internal_con_w))
      {
	tile = tile_floor;
      }
      else
      {
	switch (mod->data[y][x])
	{
	  case '#': tile = tile_wall; break;
	  case '.': tile = tile_floor; break;
	  case 'X': tile = tile_door_closed; break;
	  case '_': tile = tile_pentagram; break;
	  case '<': tile = tile_stair0; break;
	  case '>': tile = tile_stair1; break;
	  case 'L': tile = tile_lava; break;
	  case 'a': tile = tile_water; break;
	  case '|': tile = tile_door_secret_v; break;
	  case '-': tile = tile_door_secret_h; break;
	  case 'F': tile = tile_generator; break;

	  case 'R': tile = tile_internal_reserved; break;
	  case 'S': tile = tile_internal_start; break;
	  case 'E': tile = tile_internal_sweet; break;
	  case 'B': tile = tile_internal_boss; break;

	  case 'n': tile = tile_internal_con_n; break;
	  case 'e': tile = tile_internal_con_e; break;
	  case 's': tile = tile_internal_con_s; break;
	  case 'w': tile = tile_internal_con_w; break;

	  case '/':
	    if (maybe())
	      tile = tile_door_closed;
	    else
	      tile = tile_floor;
	    break;

	  default: tile = tile_wall; break;
	}
      }

      set_tile(level, t + y, l + x, tile);
    }
  }

  return;  
} /* put_module */



void put_special_module(level_t * level, const unsigned int design)
{
  module_t * m;

  if (level == NULL)
    return;

  m = new_module_noreg();

  switch (design)
  {
    case CHAMBER_START:
    default:
      add_line(m, "  nnn  ");
      add_line(m, " #RRR# ");
      add_line(m, "wRRRRRe");
      add_line(m, "wRRSRRe");
      add_line(m, "wRRRRRe");
      add_line(m, " #RRR# ");
      add_line(m, "  sss  ");
      break;

    case CHAMBER_LAB:
      add_line(m, "  nn  ");
      add_line(m, " #..# ");
      add_line(m, "w....e");
      add_line(m, "w....e");
      add_line(m, " #..# ");
      add_line(m, "  ss  ");
      break;

    case CHAMBER_TEST:
      add_line(m, "#####################################");
      add_line(m, "#.....G.............................#");
      add_line(m, "#...................................#");
      add_line(m, "#............................##.....#");
      add_line(m, "#...G.........G..............##.....#");
      add_line(m, "#...................................#");
      add_line(m, "#...................................#");
      add_line(m, "#........G..........................#");
      add_line(m, "#...................................#");
      add_line(m, "#............................##.....#");
      add_line(m, "#..............................S....#");
      add_line(m, "#...................................#");
      add_line(m, "#...................................#");
      add_line(m, "#####################################");
      break;

    case CHAMBER_ENDGAME:
      add_line(m, " ####################################");
      add_line(m, "##LL................................#");
      add_line(m, "#LL...LL......LLLLLLLLL.............#");
      add_line(m, "#LL...LLL.....LLLLLLLLL.............#");
      add_line(m, "####.##LLLL.........................#");
      add_line(m, "  #F.F#LLLLL........................#");
      add_line(m, "  ##.##LLLLL........................#");
      add_line(m, "###...####LL........................#");
      add_line(m, "##.....#F#LL........................#");
      add_line(m, "##..B.....LL........................#");
      add_line(m, "##.....#F#LL........................#");
      add_line(m, "###...####LL........................#");
      add_line(m, "  ##.##LLLLL........................#");
      add_line(m, "  #F.F#LLLLL...................S....#");
      add_line(m, "  ##.##LL...........................#");
      add_line(m, "                                     ");
      add_line(m, "                                     ");
      add_line(m, "                                     ");
      add_line(m, "                                     ");
      break;

    case CHAMBER_KING:
      add_line(m, "   nn               ");
      add_line(m, "  #..#############  ");
      add_line(m, "  #..##RRRRRR###..e ");
      add_line(m, "  #..RRRRRRRRRR...e ");
      add_line(m, "  #.RRRRR##RRRRR.#  ");
      add_line(m, "  #RR##RR##RR##RR#  ");
      add_line(m, " ##RR##RRRRRR##RR## ");
      add_line(m, " #RRRRRRRRRRRRRRRR# ");
      add_line(m, " #RRRRRRRRRRRRRRRR# ");
      add_line(m, " #RR##RRRBRRRR##RR# ");
      add_line(m, " #RR##RRRRRRRR##RR# ");
      add_line(m, " #RRRRRRRRRRRRRRRR# ");
      add_line(m, " #RRRRRRRRRRRRRRRR# ");
      add_line(m, " ##RR##RRRRRR##RR## ");
      add_line(m, "  #RR##RR##RR##RR#  ");
      add_line(m, "  #.RRRRR##RRRRR.#  ");
      add_line(m, " w...RRRRRRRRRR..#  ");
      add_line(m, " w..###RRRRRR##..#  ");
      add_line(m, "  #############..#  ");
      add_line(m, "               ss   ");
      break;

    case CHAMBER_LURKER:
      add_line(m, "               n               ");
      add_line(m, "             #...####          ");
      add_line(m, "           ##a...aaa#          ");
      add_line(m, "         ##aaaaaaaaa###        ");
      add_line(m, "         #..aaaaaaaa..###      ");
      add_line(m, "         ...aaaaaaa......      ");
      add_line(m, "        w...aaaBaaa......e     ");
      add_line(m, "         ...aaaaaaa..##..      ");
      add_line(m, "         #.aaaaaaaa..####      ");
      add_line(m, "          ##aaaaaaaaaaaa#      ");
      add_line(m, "           #a.....aaaaa##      ");
      add_line(m, "           ###...##aa###       ");
      add_line(m, "               s  ####         ");
      break;

    case CHAMBER_NECROMANCER:
      add_line(m, "       #####       ");
      add_line(m, "     ###...###     ");
      add_line(m, "     #.......#     ");
      add_line(m, " nn ##.......## nn ");
      add_line(m, "#...#.........#...#");
      add_line(m, "#...X....B....X...#");
      add_line(m, "#...#.........#...#");
      add_line(m, " ss ##.......## ss ");
      add_line(m, "     #.......#     ");
      add_line(m, "     ###...###     ");
      add_line(m, "       #####       ");
      break;

    case CHAMBER_DRAGON:
      add_line(m, "     #####     ");
      add_line(m, "     #.<.#     ");
      add_line(m, "     #...#     ");
      add_line(m, "     ##.##     ");
      add_line(m, "      #.#      ");
      add_line(m, "    ###.###    ");
      add_line(m, "  ###.....###  ");
      add_line(m, "  #.........#  ");
      add_line(m, "  #....B....#  ");
      add_line(m, "  #.........#  ");
      add_line(m, "  #.........#  ");
      add_line(m, "  #.........#  ");
      add_line(m, "w.............e");
      add_line(m, "  ###.....###  ");
      add_line(m, "    #######    ");
      break;

/*
      add_line(m, "      n      ");
      add_line(m, "   ###.###   ");
      add_line(m, " ###.....### ");
      add_line(m, " #.........# ");
      add_line(m, " #.........# ");
      add_line(m, " #.........# ");
      add_line(m, "w.....B.....e");
      add_line(m, " #.........# ");
      add_line(m, " #.........# ");
      add_line(m, " #.........# ");
      add_line(m, " ###.....### ");
      add_line(m, "   ###.###   ");
      add_line(m, "      s      ");*/

    case CHAMBER_VAULT:
      add_line(m, "     #####     ");
      add_line(m, "     #FFF#     ");
      add_line(m, "  ####RRR# nn  ");
      add_line(m, " w....RRR....# ");
      add_line(m, " w....RRR....# ");
      add_line(m, "  ....RRR....# ");
      add_line(m, " #.>..RRR..<.# ");
      add_line(m, " #....RRR....  ");
      add_line(m, " #....RRR....e ");
      add_line(m, " #....RRR....e ");
      add_line(m, "  ss #RRR####  ");
      add_line(m, "     #FFF#     ");
      add_line(m, "     #####     ");
      break;

    case CHAMBER_ARENA:
      add_line(m, "      nnn      ");
      add_line(m, "   ###...###   ");
      add_line(m, "  ##.......##  ");
      add_line(m, " ##FRRRRRRRF## ");
      add_line(m, " #.RRRRRRRRR.# ");
      add_line(m, " #.RRRRRRRRR.# ");
      add_line(m, "w..RRRRRRRRR..e");
      add_line(m, "w..RRRRBRRRR..e");
      add_line(m, "w..RRRRRRRRR..e");
      add_line(m, " #.RRRRRRRRR.# ");
      add_line(m, " #.RRRRRRRRR.# ");
      add_line(m, " ##FRRRRRRRF## ");
      add_line(m, "  ##.......##  ");
      add_line(m, "   ###...###   ");
      add_line(m, "      sss      ");
      break;
      
/*    case CHAMBER_CHAPEL:
      add_line(m, "      nnn      ");
      add_line(m, "     LLLLL     ");
      add_line(m, "   LLL...LLL   ");
      add_line(m, "  LL.......LL  ");
      add_line(m, "  L..LLLLL..L  ");
      add_line(m, " LL.LL...LL.LL ");
      add_line(m, "wL..L.....L..Le");
      add_line(m, "wL..L..B..L..Le");
      add_line(m, "wL..L.....L..Le");
      add_line(m, " LL.LL...LL.LL ");
      add_line(m, "  L..LLLLL..L  ");
      add_line(m, "  LL.......LL  ");
      add_line(m, "   LLL...LLL   ");
      add_line(m, "     LLLLL     ");
      add_line(m, "      sss      ");
      break;*/
  }

/*  if (design == CHAMBER_CHAPEL)
  {
    put_module(level, m, 0, level->size_x - strlen(m->data[0]));
  }
  else
  {*/
    put_module(level, m,
	       tslrnd() % MAX(1, level->size_y - m->lines),
	       tslrnd() % MAX(1, level->size_x - strlen(m->data[0]))
      );
/*  }*/

  del_module(m);

  return;
} /* put_special_module */



/* */
void init_modules(void)
{
  unsigned int i;
  module_t * m;

  for (i = 0; i < MODULES; i++)
    module[i] = NULL;

  m = new_module();
  m->category = MODULE_NARROW;
  add_line(m, " ### ");
  add_line(m, "w#..e");
  add_line(m, "w#.#e");
  add_line(m, "w..#e");
  add_line(m, " ### ");
  rotate_module(m);

  m = new_module();
  m->category = MODULE_NARROW;
  add_line(m, "  ####e");
  add_line(m, "  #...e");
  add_line(m, "  #.##e");
  add_line(m, "w##.#  ");
  add_line(m, "w...#  ");
  add_line(m, "w####  ");
  rotate_module(m);

  m = new_module();
  m->category = MODULE_NARROW;
  add_line(m, " ### ");
  add_line(m, "w#..e");
  add_line(m, "w..#e");
  add_line(m, " ### ");
  rotate_module(m);

  m = new_module();
  m->category = MODULE_NARROW;
  add_line(m, "  nn  ");
  add_line(m, " #..# ");
  add_line(m, " #...e");
  add_line(m, "w....e");
  add_line(m, " #### ");
  rotate_module(m);

  m = new_module();
  m->category = MODULE_NARROW;
  add_line(m, " n ");
  add_line(m, "#.#");
  add_line(m, "#.#");
  add_line(m, " s ");
  rotate_module(m);

  m = new_module();
  m->category = MODULE_NARROW;
  add_line(m, "#### ");
  add_line(m, "#...e");
  add_line(m, "#.## ");
  add_line(m, "#.#  ");
  add_line(m, " s   ");
  rotate_module(m);

  m = new_module();
  m->category = MODULE_NARROW;
  add_line(m, " ### ");
  add_line(m, " #..e");
  add_line(m, " #.# ");
  add_line(m, " #.# ");
  add_line(m, "w..# ");
  add_line(m, " ### ");
  rotate_module(m);

  m = new_module();
  m->category = MODULE_NARROW;
  add_line(m, " ##### ");
  add_line(m, "w.....e");
  add_line(m, " ##.## ");
  add_line(m, "  #.#  ");
  add_line(m, "  #.#  ");
  add_line(m, "   s   ");
  rotate_module(m);

  m = new_module();
  m->category = MODULE_NARROW;
  add_line(m, "  n    ");
  add_line(m, " #.### ");
  add_line(m, "w..#..e");
  add_line(m, " #.|.# ");
  add_line(m, " ###.# ");
  add_line(m, "    s  ");
  rotate_module(m);

  m = new_module();
  m->category = MODULE_NARROW;
  add_line(m, "  n    ");
  add_line(m, " #.### ");
  add_line(m, "w..#..e");
  add_line(m, " #-#-# ");
  add_line(m, " #...# ");
  add_line(m, " ###.# ");
  add_line(m, "    s  ");
  rotate_module(m);

  m = new_module();
  m->category = MODULE_NARROW;
  add_line(m, "  n n  ");
  add_line(m, " #.#.# ");
  add_line(m, " #.|.# ");
  add_line(m, " #.#.# ");
  add_line(m, "  s s  ");
  rotate_module(m);

  m = new_module();
  m->category = MODULE_NARROW;
  add_line(m, "   n   ");
  add_line(m, "  #.#  ");
  add_line(m, "  #.|e ");
  add_line(m, "  #.#  ");
  add_line(m, "  #.#  ");
  add_line(m, "   s   ");
  rotate_module(m);

  m = new_module();
  m->category = MODULE_ROOM;
  add_line(m, " n ");
  add_line(m, "w.e");
  add_line(m, "w.e");
  add_line(m, "w.e");
  add_line(m, " s ");
  rotate_module(m);

  m = new_module();
  m->category = MODULE_ROOM;
  add_line(m, "   #####   ");
  add_line(m, " ###...### ");
  add_line(m, " #.......# ");
  add_line(m, "##.......# ");
  add_line(m, "#......... ");
  add_line(m, "#....E....e");
  add_line(m, "#......... ");
  add_line(m, "##.......# ");
  add_line(m, " #.......# ");
  add_line(m, " ###...### ");
  add_line(m, "   #####   ");
  rotate_module(m);

  m = new_module();
  m->category = MODULE_ROOM;
  add_line(m, "  #####  ");
  add_line(m, " ##...## ");
  add_line(m, "##.....# ");
  add_line(m, "#....... ");
  add_line(m, "#...E...e");
  add_line(m, "#....... ");
  add_line(m, "##.....# ");
  add_line(m, " ##...## ");
  add_line(m, "  #####  ");
  rotate_module(m);
  
  m = new_module();
  m->category = MODULE_ROOM;
  add_line(m, "   n   ");
  add_line(m, " ##-## ");
  add_line(m, " #...# ");
  add_line(m, "w|.E.|e");
  add_line(m, " #...# ");
  add_line(m, "  sss  ");
  rotate_module(m);

  m = new_module();
  m->category = MODULE_ROOM;
  add_line(m, "     n  ");
  add_line(m, " ####-# ");
  add_line(m, " #.....e");
  add_line(m, "w|.....e");
  add_line(m, " #.....e");
  add_line(m, " ##-### ");
  add_line(m, "   s    ");
  rotate_module(m);

  m = new_module();
  m->category = MODULE_ROOM;
  m->rarity = 2;
  add_line(m, "  ###  ");
  add_line(m, " ##F## ");
  add_line(m, "w..R..e");
  add_line(m, "w..R..e");
  add_line(m, "w..R..e");
  add_line(m, " ##F## ");
  add_line(m, "  ###  ");
  rotate_module(m);

  m = new_module();
  m->category = MODULE_ROOM;
  m->rarity = 2;
  add_line(m, "  ###  ");
  add_line(m, " ##F## ");
  add_line(m, "w..R..e");
  add_line(m, "w..R..e");
  add_line(m, " ##F## ");
  add_line(m, "  ###  ");
  rotate_module(m);

  m = new_module();
  m->rarity = 3;
  m->category = MODULE_FIELDS;
  add_line(m, "  ####  ");
  add_line(m, " ##FF## ");
  add_line(m, "w..RR..e");
  add_line(m, "w..RR..e");
  add_line(m, "w..RR..e");
  add_line(m, " ##FF## ");
  add_line(m, "  ####  ");
  rotate_module(m);

  m = new_module();
  m->category = MODULE_FIELDS;
  m->rarity = 4;
  add_line(m, " ########### ");
  add_line(m, " #....#....# ");
  add_line(m, "w..##.#.##..e");
  add_line(m, "w..#FRRRF#..e");
  add_line(m, "w..##.#.##..e");
  add_line(m, " #....#....# ");
  add_line(m, " ########### ");
  rotate_module(m);

  m = new_module();
  m->category = MODULE_FIELDS;
  m->rarity = 5;
  add_line(m, " nnn ");
  add_line(m, "#...#");
  add_line(m, "##.##");
  add_line(m, "#FRF#");
  add_line(m, "##.##");
  add_line(m, "#...#");
  add_line(m, " sss ");
  rotate_module(m);

  m = new_module();
  m->category = MODULE_FIELDS;
  m->rarity = 8;
  add_line(m, " ##### ");
  add_line(m, " #.E.# ");
  add_line(m, " #...# ");
  add_line(m, "###X###");
  add_line(m, "#FRRRF#");
  add_line(m, "##...##");
  add_line(m, "  sss  ");
  rotate_module(m);

  m = new_module();
  m->category = MODULE_ROOM;
  add_line(m, "        n    ");
  add_line(m, "   #####-#   ");
  add_line(m, "   #.....#   ");
  add_line(m, "  w|.....#   ");
  add_line(m, "   #.....|e  ");
  add_line(m, "   #.....#   ");
  add_line(m, "   ####-##   ");
  add_line(m, "       s     ");
  rotate_module(m);

  m = new_module();
  m->category = MODULE_ROOM;
  m->rarity = 3;
  add_line(m, "    n    ");
  add_line(m, " ###.### ");
  add_line(m, " #F...F# ");
  add_line(m, " #.....# ");
  add_line(m, "w|.....# ");
  add_line(m, " #.....# ");
  add_line(m, " #.....|e");
  add_line(m, " #F...F# ");
  add_line(m, " ##-#### ");
  add_line(m, "   s     ");
  rotate_module(m);

  m = new_module();
  m->category = MODULE_ROOM;
  add_line(m, "    n  ");
  add_line(m, " ###-# ");
  add_line(m, "w|...# ");
  add_line(m, " #.E.# ");
  add_line(m, " #...|e");
  add_line(m, " #-### ");
  add_line(m, "  s    ");
  rotate_module(m);

  m = new_module();
  m->category = MODULE_ROOM;
  add_line(m, " nnn ");
  add_line(m, "w...e");
  add_line(m, "w...e");
  add_line(m, " #.# ");
  add_line(m, "  s  ");
  rotate_module(m);

  m = new_module();
  m->category = MODULE_ROOM;
  add_line(m, "  nnn  ");
  add_line(m, " ##-## ");
  add_line(m, "w.....e");
  add_line(m, "w.....e");
  add_line(m, "w.....e");
  add_line(m, " ##### ");
  rotate_module(m);

  m = new_module();
  m->category = MODULE_ROOM;
  add_line(m, "  n  ");
  add_line(m, " #-# ");
  add_line(m, "w...e");
  add_line(m, "w...e");
  add_line(m, " ### ");
  rotate_module(m);

  m = new_module();
  m->category = MODULE_ROOM;
  add_line(m, "   nn  ");
  add_line(m, " ##-## ");
  add_line(m, "w.....e");
  add_line(m, "w.....e");
  add_line(m, " ##### ");
  rotate_module(m);

  m = new_module();
  m->category = MODULE_ROOM;
  add_line(m, " ###   ");
  add_line(m, " #G### ");
  add_line(m, "w.R...e");
  add_line(m, "w.GRG.e");
  add_line(m, "w...R.e");
  add_line(m, " ###G# ");
  add_line(m, "   ### ");
  rotate_module(m);

  m = new_module();
  m->category = MODULE_WIDE;
  add_line(m, "  nn  ");
  add_line(m, " #..# ");
  add_line(m, "w....e");
  add_line(m, "w....e");
  add_line(m, " #### ");
  rotate_module(m);

  m = new_module();
  m->category = MODULE_WIDE_F;
  add_line(m, " ####### ");
  add_line(m, "w.......e");
  add_line(m, "w.......e");
  add_line(m, " #GRRRG# ");
  add_line(m, " ####### ");
  rotate_module(m);

  m = new_module();
  m->category = MODULE_WIDE_F;
  m->rarity = 3;
  add_line(m, "  ####  ");
  add_line(m, "  #FF#  ");
  add_line(m, " ##RR## ");
  add_line(m, "w..RR..e");
  add_line(m, "w..RR..e");
  add_line(m, " ##RR## ");
  add_line(m, "  #FF#  ");
  add_line(m, "  ####  ");
  rotate_module(m);

  m = new_module();
  m->category = MODULE_ROOM;
  m->rarity = 2;
  add_line(m, " ## nnn ## ");
  add_line(m, " #.......# ");
  add_line(m, "  .#####.  ");
  add_line(m, " w.#   #.e ");
  add_line(m, " w.#   #.e ");
  add_line(m, " w.#   #.e ");
  add_line(m, "  .#####.  ");
  add_line(m, " #.......# ");
  add_line(m, " ## sss ## ");

  m = new_module();
  m->category = MODULE_ROOM;
  m->rarity = 2;
  add_line(m, " ## nnn ## ");
  add_line(m, " #.......# ");
  add_line(m, "  .......  ");
  add_line(m, " w.......e ");
  add_line(m, " w.......e ");
  add_line(m, " w.......e ");
  add_line(m, "  .......  ");
  add_line(m, " #.......# ");
  add_line(m, " ## sss ## ");

  m = new_module();
  m->category = MODULE_ROOM;
  m->rarity = 8;
  add_line(m, " ## nn ## ");
  add_line(m, " #......# ");
  add_line(m, "  .F..F.  ");
  add_line(m, " w......e ");
  add_line(m, " w......e ");
  add_line(m, "  .F..F.  ");
  add_line(m, " #......# ");
  add_line(m, " ## ss ## ");

  m = new_module();
  m->category = MODULE_ROOM;
  m->rarity = 2;
  add_line(m, " ## nn ## ");
  add_line(m, " #......# ");
  add_line(m, "  ......  ");
  add_line(m, " w......e ");
  add_line(m, " w......e ");
  add_line(m, "  ......  ");
  add_line(m, " #......# ");
  add_line(m, " ## ss ## ");

  m = new_module();
  m->category = MODULE_ROOM;
  m->rarity = 2;
  add_line(m, " ## nn ");
  add_line(m, "w.....#");
  add_line(m, "w..... ");
  add_line(m, " .....e");
  add_line(m, "#.....e");
  add_line(m, " ss ## ");

  m = new_module();
  m->category = MODULE_ROOM;
  m->rarity = 4;
  add_line(m, " ## nnn ## ");
  add_line(m, " #.......# ");
  add_line(m, "  .##X##.  ");
  add_line(m, " w.#...#.e ");
  add_line(m, " w.X.E.X.e ");
  add_line(m, " w.#...#.e ");
  add_line(m, "  .##X##.  ");
  add_line(m, " #.......# ");
  add_line(m, " ## sss ## ");

  m = new_module();
  m->category = MODULE_ROOM;
  m->rarity = 4;
  add_line(m, " ## nnn ## ");
  add_line(m, " #.......# ");
  add_line(m, "  .#####.  ");
  add_line(m, " w.#...#.e ");
  add_line(m, " w.#.E.#.e ");
  add_line(m, " w.#...#.e ");
  add_line(m, "  .##-##.  ");
  add_line(m, " #.......# ");
  add_line(m, " ## sss ## ");
  rotate_module(m);

  m = new_module();
  m->category = MODULE_ROOM;
  m->rarity = 4;
  add_line(m, "### nnn ### ");
  add_line(m, "#.........# ");
  add_line(m, "#.........# ");
  add_line(m, " ..#####..  ");
  add_line(m, "w..#####..e ");
  add_line(m, "w..#####..e ");
  add_line(m, "w..#####..e ");
  add_line(m, " ..#####..  ");
  add_line(m, "#.........# ");
  add_line(m, "#.........# ");
  add_line(m, "### sss ### ");
  rotate_module(m);

  m = new_module();
  m->category = MODULE_CELLS;
  add_line(m, "     n ");
  add_line(m, "#####.#");
  add_line(m, "#...#.#");
  add_line(m, "#.E.|.#");
  add_line(m, "#...#.#");
  add_line(m, "#####.#");
  add_line(m, "     s ");
  rotate_module(m);

  m = new_module();
  m->category = MODULE_CELLS;
  add_line(m, "    n ");
  add_line(m, "####.#");
  add_line(m, "#..#.#");
  add_line(m, "#E.|.#");
  add_line(m, "#..#.#");
  add_line(m, "####.#");
  add_line(m, "    s ");
  rotate_module(m);

  m = new_module();
  m->category = MODULE_CELLS;
  add_line(m, "  nnn  ");
  add_line(m, " ##X## ");
  add_line(m, " #...# ");
  add_line(m, " #...# ");
  add_line(m, "  sss  ");
  rotate_module(m);

  m = new_module();
  m->category = MODULE_CELLS;
  add_line(m, "  nnn  ");
  add_line(m, " #...# ");
  add_line(m, " ###-# ");
  add_line(m, " w...# ");
  add_line(m, "  sss  ");
  rotate_module(m);

  m = new_module();
  m->category = MODULE_CELLS;
  add_line(m, "  nnn ");
  add_line(m, " #...#");
  add_line(m, " #...#");
  add_line(m, " ###-#");
  add_line(m, "w....#");
  add_line(m, "w....#");
  add_line(m, " #####");
  rotate_module(m);

  m = new_module();
  m->category = MODULE_CELLS;
  add_line(m, "  nnn  ");
  add_line(m, " ..... ");
  add_line(m, "w.....e");
  add_line(m, " ##.## ");
  add_line(m, "   s   ");
  rotate_module(m);

  m = new_module();
  m->category = MODULE_CELLS;
  m->rarity = 4;
  add_line(m, "  nnnnnnn  ");
  add_line(m, " #.......# ");
  add_line(m, " #.#####.# ");
  add_line(m, "  .#...#.  ");
  add_line(m, " w.X.E.X.e ");
  add_line(m, "  ##...##  ");
  add_line(m, "   #####   ");
  rotate_module(m);

  m = new_module();
  m->category = MODULE_CELLS;
  m->rarity = 10;
  add_line(m, " ### nnn ### ");
  add_line(m, " #..#...#..# ");
  add_line(m, " #..#.E.#..# ");
  add_line(m, " ##X#...#X## ");
  add_line(m, "w...........e");
  add_line(m, " ##X##/##X## ");
  add_line(m, " #...#.#...# ");
  add_line(m, " #...#.#...# ");
  add_line(m, " #####.##### ");
  add_line(m, "      s      ");
  rotate_module(m);

  m = new_module();
  m->category = MODULE_CITY;
  add_line(m, " ### nnn ### ");
  add_line(m, " #aaa...aaa# ");
  add_line(m, " #aaa...aaa# ");
  add_line(m, " #aaa...aaa# ");
  add_line(m, "w...........e");
  add_line(m, " #.##...##.# ");
  add_line(m, " #.##...##.# ");
  add_line(m, " #.........# ");
  add_line(m, " #####.##### ");
  add_line(m, "      s      ");
  rotate_module(m);

  m = new_module();
  m->category = MODULE_CITY;
  add_line(m, " ### nnn ### ");
  add_line(m, " #.........# ");
  add_line(m, "  ..aaaaa...e");
  add_line(m, " w..aaaaa..# ");
  add_line(m, " w..aaaaa..# ");
  add_line(m, " w..aaaaa..# ");
  add_line(m, "  ..aaaaa..# ");
  add_line(m, " #.........# ");
  add_line(m, " #######.### ");
  add_line(m, "        s    ");
  rotate_module(m);

  m = new_module();
  m->category = MODULE_CITY;
  add_line(m, "    n    ");
  add_line(m, " ###.### ");
  add_line(m, " ......# ");
  add_line(m, " ..aa...e");
  add_line(m, "w..aa..# ");
  add_line(m, "w..aa..# ");
  add_line(m, "w......# ");
  add_line(m, " ####.## ");
  add_line(m, "     s   ");
  rotate_module(m);

  m = new_module();
  m->category = MODULE_CITY;
  add_line(m, "    n     ");
  add_line(m, " ###.#### ");
  add_line(m, " #......# ");
  add_line(m, "  ..aa..# ");
  add_line(m, " w..aaa..e");
  add_line(m, " w...aaa# ");
  add_line(m, " w....aa# ");
  add_line(m, "  ###.### ");
  add_line(m, "     s    ");
  rotate_module(m);

  m = new_module();
  m->category = MODULE_CITY;
  add_line(m, "     nnn      ");
  add_line(m, "    #...#     ");
  add_line(m, "    #...##### ");
  add_line(m, "   ##........e");
  add_line(m, " ###aaaa....# ");
  add_line(m, " #.aaaaaa..## ");
  add_line(m, "w....aa#####  ");
  add_line(m, "w......#      ");
  add_line(m, " ####.##      ");
  add_line(m, "     s        ");
  rotate_module(m);

  m = new_module();
  m->category = MODULE_CELLS;
  m->rarity = 10;
  add_line(m, "     nnn     ");
  add_line(m, " ####...#### ");
  add_line(m, "w...........e");
  add_line(m, " ##X##/##X## ");
  add_line(m, " #...#.#...# ");
  add_line(m, " #...#.#...# ");
  add_line(m, " #####.##### ");
  add_line(m, "      s      ");
  rotate_module(m);

  m = new_module();
  m->category = MODULE_CELLS;
  m->rarity = 10;
  add_line(m, "     nnn     ");
  add_line(m, " ####...#### ");
  add_line(m, "w...........e");
  add_line(m, " #####/##### ");
  add_line(m, "     #.#     ");
  add_line(m, "     #.#     ");
  add_line(m, "     #.#     ");
  add_line(m, "      s      ");
  rotate_module(m);

  m = new_module();
  m->category = MODULE_CELLS;
  m->rarity = 4;
  add_line(m, " ### nnn ");
  add_line(m, " #..#...#");
  add_line(m, " #..#.E.#");
  add_line(m, " ##X#...#");
  add_line(m, "w.......#");
  add_line(m, " ########");
  rotate_module(m);

  m = new_module();
  m->category = MODULE_CAVE;
  add_line(m, "         ### ");
  add_line(m, "     nn  #.# ");
  add_line(m, "  ##.......# ");
  add_line(m, " ##.........e");
  add_line(m, "w.....##....e");
  add_line(m, "w.....##...# ");
  add_line(m, " ##.......## ");
  add_line(m, "  ##....###  ");
  add_line(m, "   #....#    ");
  add_line(m, "     ss      ");
  rotate_module(m);

  m = new_module();
  m->category = MODULE_WIDE_F;
  m->rarity = 2;
  add_line(m, "   nn   ");
  add_line(m, "###..###");
  add_line(m, "#.F..F.#");
  add_line(m, "#.R..R.#");
  add_line(m, "#.R..R.#");
  add_line(m, "#.F..F.#");
  add_line(m, "###..###");
  add_line(m, "   nn   ");
  rotate_module(m);

  m = new_module();
  m->category = MODULE_WIDE;
  add_line(m, "##### ");
  add_line(m, "#....e");
  add_line(m, "#....e");
  add_line(m, "#..## ");
  add_line(m, " ss   ");
  rotate_module(m);

  m = new_module();
  m->category = MODULE_WIDE;
  add_line(m, "#########");
  add_line(m, "#.......#");
  add_line(m, "#.......#");
  add_line(m, "#..###..#");
  add_line(m, "#..###..#");
  add_line(m, "#..###.. ");
  add_line(m, "#.......e");
  add_line(m, "#.......e");
  add_line(m, "##### ss ");
  rotate_module(m);

  m = new_module();
  m->category = MODULE_WIDE;
  add_line(m, " nn #### ");
  add_line(m, "#.......e");
  add_line(m, "#.......e");
  add_line(m, "#..###.. ");
  add_line(m, "#..###..#");
  add_line(m, " ..###..#");
  add_line(m, "w.......#");
  add_line(m, "w.......#");
  add_line(m, " #### ss ");

  m = new_module();
  m->category = MODULE_WIDE;
  add_line(m, " #### nn ");
  add_line(m, "w.......#");
  add_line(m, "w.......#");
  add_line(m, " ..###..#");
  add_line(m, "#..###..#");
  add_line(m, "#..###..#");
  add_line(m, "#.......e");
  add_line(m, "#.......e");
  add_line(m, " ss #### ");

  m = new_module();
  m->category = MODULE_WIDE;
  add_line(m, " # nn ");
  add_line(m, "w....#");
  add_line(m, "w.... ");
  add_line(m, " ....e");
  add_line(m, "#....e");
  add_line(m, " ss # ");

/*  m = new_module();
  m->category = MODULE_WIDE;
  add_line(m, "  ##  ");
  add_line(m, " w..e ");
  add_line(m, " w..e ");
  add_line(m, "  ..  ");
  add_line(m, " #..# ");
  add_line(m, "  ss  ");
  rotate_module(m);*/

  m = new_module();
  m->category = MODULE_WIDE;
  add_line(m, " #### nn #### ");
  add_line(m, "w............e");
  add_line(m, "w............e");
  add_line(m, " #####..##### ");
  add_line(m, "     #..#     ");
  add_line(m, "     #..#     ");
  add_line(m, "      ss      ");
  rotate_module(m);

  m = new_module();
  m->category = MODULE_WIDE;
  add_line(m, "     #######     ");
  add_line(m, " #####.....##### ");
  add_line(m, "w....X.....#....e");
  add_line(m, "w....#.....X....e");
  add_line(m, " #####.....##### ");
  add_line(m, "     #######     ");
  rotate_module(m);

  m = new_module();
  m->category = MODULE_WIDE;
  add_line(m, "     #######     ");
  add_line(m, " #####.....##### ");
  add_line(m, "w....#.....#....e");
  add_line(m, "w....|.....|....e");
  add_line(m, " #####.....##### ");
  add_line(m, "     #######     ");
  rotate_module(m);

  m = new_module();
  m->category = MODULE_WIDE;
  add_line(m, "      #####   ");
  add_line(m, "     w.....e  ");
  add_line(m, "     w.....e  ");
  add_line(m, "      .....   ");
  add_line(m, "     #.....#  ");
  add_line(m, "     #######  ");
  rotate_module(m);

  m = new_module();
  m->category = MODULE_WIDE;
  add_line(m, "      ## nn   ");
  add_line(m, "     w.....#  ");
  add_line(m, "     w.....   ");
  add_line(m, "      .....e  ");
  add_line(m, "     #.....e  ");
  add_line(m, "     ######   ");
  rotate_module(m);

  m = new_module();
  m->category = MODULE_WIDE;
  add_line(m, " ####    nn  ");
  add_line(m, "w...#   #...e");
  add_line(m, "w...#   #...e");
  add_line(m, " ##.#   #.## ");
  add_line(m, "  #.#####.#  ");
  add_line(m, "  #.......#  ");
  add_line(m, "  #########  ");
  rotate_module(m);

  m = new_module();
  m->category = MODULE_WIDE;
  add_line(m, "  nn  ");
  add_line(m, " #..# ");
  add_line(m, " #..# ");
  add_line(m, " #..# ");
  add_line(m, " #..# ");
  add_line(m, " #..# ");
  add_line(m, " #..# ");
  add_line(m, "  ss  ");
  rotate_module(m);

  m = new_module();
  m->category = MODULE_WIDE;
  add_line(m, "  nn  ");
  add_line(m, " #..# ");
  add_line(m, " #..# ");
  add_line(m, " #..# ");
  add_line(m, " #..  ");
  add_line(m, " #..e ");
  add_line(m, " #..e ");
  add_line(m, " #..  ");
  add_line(m, " #..# ");
  add_line(m, " #..# ");
  add_line(m, " #..# ");
  add_line(m, "  ss  ");
  rotate_module(m);

  m = new_module();
  m->category = MODULE_WIDE_ROOM;
  add_line(m, " nn      ");
  add_line(m, "#..#....#");
  add_line(m, "#..X....#");
  add_line(m, "#..#....#");
  add_line(m, "#..#....#");
  add_line(m, " ss      ");
  rotate_module(m);

  m = new_module();
  m->category = MODULE_WIDE_ROOM;
  add_line(m, "      nn      ");
  add_line(m, "#....#..#....#");
  add_line(m, "#....#..X....#");
  add_line(m, "#....X..#....#");
  add_line(m, "#....#..#....#");
  add_line(m, "      ss      ");
  rotate_module(m);

  m = new_module();
  m->category = MODULE_WIDE_ROOM;
  add_line(m, " nn      nn ");
  add_line(m, "#..#....#..#");
  add_line(m, "#..#....#..#");
  add_line(m, "#..#....X..#");
  add_line(m, "#..#....#..#");
  add_line(m, "#..######..#");
  add_line(m, " ss      ss ");
  rotate_module(m);

  m = new_module();
  m->category = MODULE_WIDE_ROOM;
  add_line(m, "      nn      ");
  add_line(m, "#....#..#....#");
  add_line(m, "#....X..X....#");
  add_line(m, "#....#..#....#");
  add_line(m, "      ss      ");
  rotate_module(m);

  m = new_module();
  m->category = MODULE_WIDE;
  m->rarity = 4;
  add_line(m, "###### ");
  add_line(m, "#.....e");
  add_line(m, "#.....e");
  add_line(m, "#..### ");
  add_line(m, "#..#   ");
  add_line(m, "#..#   ");
  add_line(m, " ss    ");
  rotate_module(m);

  m = new_module();
  m->category = MODULE_WIDE;
  m->rarity = 4;
  add_line(m, "######## ");
  add_line(m, "#.......e");
  add_line(m, "#.......e");
  add_line(m, "#..##### ");
  add_line(m, "#..#...# ");
  add_line(m, "#..X...# ");
  add_line(m, "#..#...# ");
  add_line(m, "#..##### ");
  add_line(m, " ss      ");
  rotate_module(m);

  m = new_module();
  m->category = MODULE_WIDE;
  m->rarity = 4;
  add_line(m, "  nn  ");
  add_line(m, " #..# ");
  add_line(m, " #..# ");
  add_line(m, " ##-# ");
  add_line(m, " #..# ");
  add_line(m, " #..# ");
  add_line(m, "  ss  ");
  rotate_module(m);

  m = new_module();
  m->category = MODULE_WIDE;
  m->rarity = 2;
  add_line(m, "  nn nn  ");
  add_line(m, " #..#..# ");
  add_line(m, " #..|..# ");
  add_line(m, "  ss ss  ");
  rotate_module(m);

  m = new_module();
  m->category = MODULE_WIDE;
  m->rarity = 2;
  add_line(m, "  nn ##  ");
  add_line(m, " #..#..e ");
  add_line(m, " #..|..e ");
  add_line(m, "  ss ##  ");
  rotate_module(m);

  m = new_module();
  m->category = MODULE_WIDE;
  m->rarity = 8;
  add_line(m, "  nn nn nn  ");
  add_line(m, " #..#..#..# ");
  add_line(m, " #..|..|..# ");
  add_line(m, "  ss ss ss  ");
  rotate_module(m);

  m = new_module();
  m->category = MODULE_WIDE_F;
  m->rarity = 2;
  add_line(m, " ###### ");
  add_line(m, "w......e");
  add_line(m, "w......e");
  add_line(m, " ##..## ");
  add_line(m, " #FRRF# ");
  add_line(m, " ##..## ");
  add_line(m, "   ss   ");
  rotate_module(m);

  m = new_module();
  m->category = MODULE_WIDE;
  m->rarity = 8;
  add_line(m, " ########## ");
  add_line(m, "w..........e");
  add_line(m, "w..........e");
  add_line(m, " ########## ");
  rotate_module(m);

  m = new_module();
  m->category = MODULE_WIDE_F;
  m->rarity = 8;
  add_line(m, " ########## ");
  add_line(m, " #FRRRRRF## ");
  add_line(m, "w..........e");
  add_line(m, "w..........e");
  add_line(m, " ##FRRRRRF# ");
  add_line(m, " ########## ");
  rotate_module(m);

  m = new_module();
  m->category = MODULE_WIDE_F;
  m->rarity = 8;
  add_line(m, " ### nn ### ");
  add_line(m, " #........# ");
  add_line(m, " #.##..##.# ");
  add_line(m, "  .#FRRF#.  ");
  add_line(m, " w..R..R..e ");
  add_line(m, " w..R..R..e ");
  add_line(m, "  .#FRRF#.  ");
  add_line(m, " #.##..##.# ");
  add_line(m, " #........# ");
  add_line(m, " ### ss ### ");
  rotate_module(m);

  m = new_module();
  m->category = MODULE_WIDE_F;
  m->rarity = 8;
  add_line(m, " ### nn ### ");
  add_line(m, " #.F......# ");
  add_line(m, " #.RFRRRRF# ");
  add_line(m, "  .R....F.  ");
  add_line(m, " w.R....R.e ");
  add_line(m, " w.R....R.e ");
  add_line(m, "  .F....R.  ");
  add_line(m, " #FRRRRFR.# ");
  add_line(m, " #......F.# ");
  add_line(m, " ### ss ### ");
  rotate_module(m);

  m = new_module();
  m->category = MODULE_WIDE_F;
  m->rarity = 3;
  add_line(m, " ###### ");
  add_line(m, "w......e");
  add_line(m, "w......e");
  add_line(m, " #FRRF# ");
  add_line(m, " ##..## ");
  add_line(m, "   ss   ");
  rotate_module(m);

  m = new_module();
  m->category = MODULE_WIDE_F;
  m->rarity = 15;
  add_line(m, "   nn   ");
  add_line(m, " ##..## ");
  add_line(m, " #FRRF# ");
  add_line(m, "w.R..R.e");
  add_line(m, "w.R..R.e");
  add_line(m, " #FRRF# ");
  add_line(m, " ##..## ");
  add_line(m, "   ss   ");
  rotate_module(m);


  m = new_module();
  m->category = MODULE_CAVE;
  add_line(m, " nn            ");
  add_line(m, "#..#    ###    ");
  add_line(m, "#..#  ###.#### ");
  add_line(m, "#..####.......e");
  add_line(m, "#.............e");
  add_line(m, "#...........## ");
  add_line(m, "#...........#  ");
  add_line(m, "##.....####-#  ");
  add_line(m, " #....## #..#  ");
  add_line(m, " #...##  #..#  ");
  add_line(m, " ##..#    ss   ");
  add_line(m, "  ####         ");
  rotate_module(m);

  m = new_module();
  m->category = MODULE_CAVE;
  add_line(m, " ####           ");
  add_line(m, "w...#  ###  nn  ");
  add_line(m, "w...## #.###..##");
  add_line(m, " #...###.......#");
  add_line(m, "##...........###");
  add_line(m, "#.......######  ");
  add_line(m, "##......#####   ");
  add_line(m, " #...........e  ");
  add_line(m, " #..###......e  ");
  add_line(m, "  ss  #######   ");
  rotate_module(m);

  m = new_module();
  m->category = MODULE_CAVE;
  add_line(m, "        nn      ");
  add_line(m, "   ### #..###   ");
  add_line(m, " ###.###....##  ");
  add_line(m, " #...........#  ");
  add_line(m, "w............#  ");
  add_line(m, "w...###.....##  ");
  add_line(m, " #### ###..##   ");
  add_line(m, "         ss     ");
  rotate_module(m);

  m = new_module();
  m->category = MODULE_CAVE;
  add_line(m, "  nn   ####    ");
  add_line(m, " #..## #..#### ");
  add_line(m, " #...###......e");
  add_line(m, "w.....#.......e");
  add_line(m, "w.....|......# ");
  add_line(m, " #....#.....## ");
  add_line(m, " #..#####..##  ");
  add_line(m, " ####    ss    ");
  rotate_module(m);

  m = new_module();
  m->category = MODULE_CAVE;
  add_line(m, "       nn        ");
  add_line(m, "      #..#####   ");
  add_line(m, "    ###......##  ");
  add_line(m, "   ##.........#  ");
  add_line(m, "   #.....##...## ");
  add_line(m, "   ##...####...# ");
  add_line(m, "    #...####....e");
  add_line(m, "   w....###.....e");
  add_line(m, "   w..........## ");
  add_line(m, "    #####..####  ");
  add_line(m, "        ####     ");
  rotate_module(m);

  m = new_module();
  m->category = MODULE_CAVE;
  add_line(m, "   #####      ");
  add_line(m, " ###...##     ");
  add_line(m, "w.......#  nn ");
  add_line(m, "w.......###..#");
  add_line(m, " #..........##");
  add_line(m, " #.......#####");
  add_line(m, "##...........#");
  add_line(m, "#...##.......#");
  add_line(m, "######..##..##");
  add_line(m, "     #### ss  ");
  rotate_module(m);

  m = new_module();
  m->category = MODULE_CAVE;
  add_line(m, "  nn         ");
  add_line(m, "##..##       ");
  add_line(m, "#....##      ");
  add_line(m, "#.....###    ");
  add_line(m, "###.....#### ");
  add_line(m, "  ##.......##");
  add_line(m, "   #####....#");
  add_line(m, "       ###..#");
  add_line(m, "          ss ");
  rotate_module(m);

  m = new_module();
  m->category = MODULE_CAVE;
  add_line(m, "  nn           ");
  add_line(m, " #..##         ");
  add_line(m, " #...#         ");
  add_line(m, "##...#         ");
  add_line(m, "#....##        ");
  add_line(m, "#.....###  ### ");
  add_line(m, "##......####..e");
  add_line(m, " #............e");
  add_line(m, " ##..###....## ");
  add_line(m, "  #..# ######  ");
  add_line(m, "   ss          ");
  rotate_module(m);

  m = new_module();
  m->category = MODULE_CAVE;
  add_line(m, "       nn         ");
  add_line(m, "      #..##       ");
  add_line(m, "    ###...##      ");
  add_line(m, "   ##......#      ");
  add_line(m, "  ##....E..#      ");
  add_line(m, " w.........#      ");
  add_line(m, " w........##      ");
  add_line(m, "  ###....##       ");
  add_line(m, "    ##....##      ");
  add_line(m, "     #.....#####  ");
  add_line(m, "    ##..E......## ");
  add_line(m, "    #............e");
  add_line(m, "   ##..###.......e");
  add_line(m, "   #..## ##..#### ");
  add_line(m, "   #..#   ####    ");
  add_line(m, "    ss            ");
  rotate_module(m);

  m = new_module();
  m->category = MODULE_CAVE;
  add_line(m, "     nn      ");
  add_line(m, "    #..###   ");
  add_line(m, "    #....#   ");
  add_line(m, " ####....##  ");
  add_line(m, "w.........## ");
  add_line(m, "w...........e");
  add_line(m, " #..........e");
  add_line(m, " ##......### ");
  add_line(m, "  ####..##   ");
  add_line(m, "      ss     ");
  rotate_module(m);

  m = new_module();
  m->category = MODULE_CAVE;
  add_line(m, " nn   ");
  add_line(m, "#..## ");
  add_line(m, "##...e");
  add_line(m, " ##..e");
  add_line(m, "w.... ");
  add_line(m, "w...##");
  add_line(m, " #..# ");
  add_line(m, "  ss  ");
  rotate_module(m);

  m = new_module();
  m->category = MODULE_CAVE;
  add_line(m, " nn   ");
  add_line(m, "#..## ");
  add_line(m, "#...# ");
  add_line(m, "##..##");
  add_line(m, "#....#");
  add_line(m, "#...##");
  add_line(m, "##..# ");
  add_line(m, " #..# ");
  add_line(m, "  ss  ");
  rotate_module(m);

  m = new_module();
  m->category = MODULE_CAVE;
  add_line(m, "  nn  ");
  add_line(m, "##..##");
  add_line(m, "#....#");
  add_line(m, "#....#");
  add_line(m, "#....#");
  add_line(m, "##..##");
  add_line(m, "  ss  ");
  rotate_module(m);

  m = new_module();
  m->category = MODULE_CAVE;
  add_line(m, "   n   ");
  add_line(m, "###-###");
  add_line(m, "##...##");
  add_line(m, "#..E..#");
  add_line(m, "##...##");
  add_line(m, "##..###");
  add_line(m, "  ss   ");
  rotate_module(m);

  m = new_module();
  m->category = MODULE_CAVE;
  add_line(m, "        ##  ");
  add_line(m, "      ###.e ");
  add_line(m, "  nn ##...e ");
  add_line(m, "w...##...#  ");
  add_line(m, "w...##..####");
  add_line(m, " #..#...#..#");
  add_line(m, " ##........#");
  add_line(m, "  ###.#  ss ");
  add_line(m, "    ###     ");
  rotate_module(m);

  m = new_module();
  m->category = MODULE_CAVE;
  add_line(m, "     nn              ");
  add_line(m, "   ##..#             ");
  add_line(m, "   #...##            ");
  add_line(m, "   #....#####        ");
  add_line(m, "   ##.......## ##### ");
  add_line(m, "   #.........# #....e");
  add_line(m, "   #.....##..###.##.e");
  add_line(m, "   ##....###.....### ");
  add_line(m, "   ##....# ##..###   ");
  add_line(m, " ###..#..## ####     ");
  add_line(m, "w.....#...#          ");
  add_line(m, "w....##...###        ");
  add_line(m, " ##..#......###      ");
  add_line(m, "  ####........#      ");
  add_line(m, "   ##...###...##     ");
  add_line(m, "   #...## ##...#     ");
  add_line(m, "   #...## ##...##    ");
  add_line(m, "   #....###.....#    ");
  add_line(m, "   ###........###    ");
  add_line(m, "     #.###..###      ");
  add_line(m, "     ### ####        ");
  rotate_module(m);

  m = new_module();
  m->category = MODULE_CAVE;
  add_line(m, "    ####  nn         ");
  add_line(m, "   ##..###..#        ");
  add_line(m, "  ##........##       ");
  add_line(m, "  #..........##      ");
  add_line(m, "  ##....###...#####  ");
  add_line(m, "###....## ##.....##  ");
  add_line(m, "#.......###.......## ");
  add_line(m, "#...........####...##");
  add_line(m, "##..........#  #....#");
  add_line(m, " ##.......###  ###..#");
  add_line(m, "  #.........###   ss ");
  add_line(m, "  ###.........#      ");
  add_line(m, "   ##.........#      ");
  add_line(m, "  ##...###.....e     ");
  add_line(m, " w.....# #.....e     ");
  add_line(m, " w.....###....#      ");
  add_line(m, "  ###...##...##      ");
  add_line(m, "    ##..###.##       ");
  add_line(m, "     #### ###        ");
  rotate_module(m);

  m = new_module();
  m->category = MODULE_CAVE;
  add_line(m, "    nn   ###   ");
  add_line(m, " ###..####.### ");
  add_line(m, " #.....##....##");
  add_line(m, "##.....##.....#");
  add_line(m, "#.............#");
  add_line(m, "##..........###");
  add_line(m, " ##.........## ");
  add_line(m, "  ###........# ");
  add_line(m, "    #........# ");
  add_line(m, "   w........## ");
  add_line(m, "   w...## ss   ");
  add_line(m, "    ####       ");
  rotate_module(m);

  m = new_module();
  m->category = MODULE_CAVE;
  add_line(m, "  nn   #### ####   ");
  add_line(m, "##..####..###..### ");
  add_line(m, "#................##");
  add_line(m, "##................#");
  add_line(m, " ##...............#");
  add_line(m, "  #..............##");
  add_line(m, "###..............# ");
  add_line(m, "#................# ");
  add_line(m, " ss  .............e");
  add_line(m, "     ###..........e");
  add_line(m, "       ####....### ");
  add_line(m, "          ######   ");
  rotate_module(m);

  m = new_module();
  m->category = MODULE_CAVE;
  add_line(m, "  nn               ");
  add_line(m, "##..##    ###..    ");
  add_line(m, "#....######......  ");
  add_line(m, "#..###............#");
  add_line(m, "##.#..............#");
  add_line(m, " #............##.##");
  add_line(m, " ###...........### ");
  add_line(m, "   #.............# ");
  add_line(m, "   ##.............e");
  add_line(m, "    ####..........e");
  add_line(m, "       ##........# ");
  add_line(m, "        #.......## ");
  add_line(m, "     ####.......#  ");
  add_line(m, "  ####..........## ");
  add_line(m, "  #..............# ");
  add_line(m, "   ..............# ");
  add_line(m, " w.............### ");
  add_line(m, " w.............#   ");
  add_line(m, "  ......########   ");
  add_line(m, "  #######          ");
  rotate_module(m);

  m = new_module();
  m->category = MODULE_CAVE;
  add_line(m, "  nn              ");
  add_line(m, " #..#        #### ");
  add_line(m, " #..#       ##...e");
  add_line(m, " #..##      #....e");
  add_line(m, " #...#     ##...# ");
  add_line(m, " #...##   ##...## ");
  add_line(m, " ##...## ##...##  ");
  add_line(m, "  #....###....#   ");
  add_line(m, "  ##.........##   ");
  add_line(m, "   ###.....###    ");
  add_line(m, "    ##....##      ");
  add_line(m, "  ###....##       ");
  add_line(m, "  #......##       ");
  add_line(m, "  ####....####    ");
  add_line(m, "     #.......##   ");
  add_line(m, "   ###........### ");
  add_line(m, "   #....###.....# ");
  add_line(m, "  ##...## ##....# ");
  add_line(m, "  #...##   ##...# ");
  add_line(m, "w.....#       ss  ");
  add_line(m, "w....##           ");
  add_line(m, " #####            ");
  rotate_module(m);

  m = new_module();
  m->category = MODULE_CAVE;
  add_line(m, "  ####          ");
  add_line(m, " w...#      nn  ");
  add_line(m, " w...### ###..# ");
  add_line(m, "  #....###....# ");
  add_line(m, "  ##.........## ");
  add_line(m, "   ##........#  ");
  add_line(m, "    #........#  ");
  add_line(m, "   ##.........e ");
  add_line(m, "   #....###...e ");
  add_line(m, "   #..### ####  ");
  add_line(m, "    ss          ");
  rotate_module(m);

  m = new_module();
  m->category = MODULE_CHAPEL;
  add_line(m, "            ");
  add_line(m, "            ");
  add_line(m, "            ");
  add_line(m, "####        ");
  add_line(m, "#LL##       ");
  add_line(m, "LLLL##      ");
  add_line(m, "LLLL########");
  add_line(m, "LLL.##.....#");
  add_line(m, "...........#");
  add_line(m, "........B..#");
  add_line(m, "...........#");
  add_line(m, "LLL.##.....#");
  add_line(m, "LLLL########");
  add_line(m, "LLLL##      ");
  add_line(m, "#LL##       ");
  add_line(m, "####        ");
  add_line(m, "            ");
  add_line(m, "            ");
  add_line(m, "            ");
      
  m = new_module();
  m->category = MODULE_CHAPEL;
  add_line(m, "#  ##  #  #");
  add_line(m, "#  ##  #  #");
  add_line(m, "#  ##  #  #");
  add_line(m, "#  ##  #  #");
  add_line(m, "###########");
  add_line(m, "###########");
  add_line(m, "###.....###");
  add_line(m, "##.......##");
  add_line(m, "##.........");
  add_line(m, "##...<.....");
  add_line(m, "##.........");
  add_line(m, "##.......##");
  add_line(m, "###.....###");
  add_line(m, "###########");
  add_line(m, "###########");
  add_line(m, "#  ##  #  #");
  add_line(m, "#  ##  #  #");
  add_line(m, "#  ##  #  #");
  add_line(m, "#  ##  #  #");

/*  m = new_module();
  m->category = MODULE_CHAPEL;
  add_line(m, "##########");
  add_line(m, "..........");
  add_line(m, "..........");
  add_line(m, "..........");
  add_line(m, "..........");
  add_line(m, "..........");
  add_line(m, "..........");
  add_line(m, "..........");
  add_line(m, "..........");
  add_line(m, "..........");
  add_line(m, "..........");
  add_line(m, "..........");
  add_line(m, "..........");
  add_line(m, "..........");
  add_line(m, "..........");
  add_line(m, "..........");
  add_line(m, "..........");
  add_line(m, "..........");
  add_line(m, "##########");*/

  m = new_module();
  m->category = MODULE_CHAPEL;
  add_line(m, "#  #");
  add_line(m, "#  #");
  add_line(m, "#  #");
  add_line(m, "#  #");
  add_line(m, "####");
  add_line(m, "LLLL");
  add_line(m, "L##L");
  add_line(m, "L##L");
  add_line(m, "LLLL");
  add_line(m, "....");
  add_line(m, "LLLL");
  add_line(m, "L##L");
  add_line(m, "L##L");
  add_line(m, "LLLL");
  add_line(m, "####");
  add_line(m, "#  #");
  add_line(m, "#  #");
  add_line(m, "#  #");
  add_line(m, "#  #");

  m = new_module();
  m->category = MODULE_CHAPEL;
  add_line(m, "###############");
  add_line(m, "###############");
  add_line(m, "##...........##");
  add_line(m, "##...........##");
  add_line(m, "##...........##");
  add_line(m, "####+#####+####");
  add_line(m, "###...LLL...###");
  add_line(m, "###...LLL...###");
  add_line(m, "......LLL......");
  add_line(m, "......LLL......");
  add_line(m, "......LLL......");
  add_line(m, "###...LLL...###");
  add_line(m, "###...LLL...###");
  add_line(m, "####+#####+####");
  add_line(m, "##...........##");
  add_line(m, "##...........##");
  add_line(m, "##...........##");
  add_line(m, "###############");
  add_line(m, "###############");

  m = new_module();
  m->category = MODULE_CHAPEL;
  add_line(m, "############");
  add_line(m, "............");
  add_line(m, "##.##.##.##.");
  add_line(m, "##.##.##.##.");
  add_line(m, "............");
  add_line(m, "##.##.##.##.");
  add_line(m, "##.##.##.##.");
  add_line(m, "............");
  add_line(m, ".##########.");
  add_line(m, ".##########.");
  add_line(m, ".##########.");
  add_line(m, "............");
  add_line(m, "##.##.##.##.");
  add_line(m, "##.##.##.##.");
  add_line(m, "............");
  add_line(m, "##.##.##.##.");
  add_line(m, "##.##.##.##.");
  add_line(m, "............");
  add_line(m, "############");

  m = new_module();
  m->category = MODULE_CHAPEL;
  add_line(m, "#  ##  ##  #");
  add_line(m, "#  ##  ##  #");
  add_line(m, "#  ##  ##  #");
  add_line(m, "#  ##  ##  #");
  add_line(m, "############");
  add_line(m, "############");
  add_line(m, "##........##");
  add_line(m, "##........##");
  add_line(m, "............");
  add_line(m, "............");
  add_line(m, "............");
  add_line(m, "##........##");
  add_line(m, "##........##");
  add_line(m, "############");
  add_line(m, "############");
  add_line(m, "#  ##  ##  #");
  add_line(m, "#  ##  ##  #");
  add_line(m, "#  ##  ##  #");
  add_line(m, "#  ##  ##  #");

  m = new_module();
  m->category = MODULE_CHAPEL;
  add_line(m, "######");
  add_line(m, "######");
  add_line(m, "######");
  add_line(m, "######");
  add_line(m, "######");
  add_line(m, "######");
  add_line(m, "L.L.L.");
  add_line(m, ".L.L.L");
  add_line(m, "L.L.L.");
  add_line(m, ".L.L.L");
  add_line(m, "L.L.L.");
  add_line(m, ".L.L.L");
  add_line(m, "L.L.L.");
  add_line(m, "######");
  add_line(m, "######");
  add_line(m, "######");
  add_line(m, "######");
  add_line(m, "######");
  add_line(m, "######");

  m = new_module();
  m->category = MODULE_CHAPEL;
  add_line(m, "####");
  add_line(m, "####");
  add_line(m, "####");
  add_line(m, "####");
  add_line(m, "####");
  add_line(m, "####");
  add_line(m, "....");
  add_line(m, "LLLL");
  add_line(m, "LLLL");
  add_line(m, "LLLL");
  add_line(m, "LLLL");
  add_line(m, "LLLL");
  add_line(m, "....");
  add_line(m, "####");
  add_line(m, "####");
  add_line(m, "####");
  add_line(m, "####");
  add_line(m, "####");
  add_line(m, "####");

  m = new_module();
  m->category = MODULE_CHAPEL;
  add_line(m, " ###############");
  add_line(m, "################");
  add_line(m, "................");
  add_line(m, "................");
  add_line(m, "..####LLLL####..");
  add_line(m, "..####LLLL####..");
  add_line(m, "..###LLLLLL###..");
  add_line(m, "..##LLLLLLLL##..");
  add_line(m, "...LLLLLLLLLL...");
  add_line(m, "...LLLLLLLLLL...");
  add_line(m, "...LLLLLLLLLL...");
  add_line(m, "..##LLLLLLLL##..");
  add_line(m, "..###LLLLLL###..");
  add_line(m, "..####LLLL####..");
  add_line(m, "..####LLLL####..");
  add_line(m, "................");
  add_line(m, "................");
  add_line(m, "################");
  add_line(m, " #############  ");

  m = new_module();
  m->category = MODULE_CHAPEL;
  add_line(m, "#  ##  ##  #");
  add_line(m, "#  ##  ##  #");
  add_line(m, "#  ##  ##  #");
  add_line(m, "#  ##  ##  #");
  add_line(m, "############");
  add_line(m, "............");
  add_line(m, ".##..##..##.");
  add_line(m, ".##..##..##.");
  add_line(m, "............");
  add_line(m, "............");
  add_line(m, "............");
  add_line(m, ".##..##..##.");
  add_line(m, ".##..##..##.");
  add_line(m, "............");
  add_line(m, "############");
  add_line(m, "#  ##  ##  #");
  add_line(m, "#  ##  ##  #");
  add_line(m, "#  ##  ##  #");
  add_line(m, "#  ##  ##  #");

  m = new_module();
  m->category = MODULE_CHAPEL;
  add_line(m, "#  #");
  add_line(m, "#  #");
  add_line(m, "#  #");
  add_line(m, "####");
  add_line(m, "LLLL");
  add_line(m, "L##L");
  add_line(m, "L##L");
  add_line(m, "LLLL");
  add_line(m, "....");
  add_line(m, "....");
  add_line(m, "....");
  add_line(m, "LLLL");
  add_line(m, "L##L");
  add_line(m, "L##L");
  add_line(m, "LLLL");
  add_line(m, "####");
  add_line(m, "#  #");
  add_line(m, "#  #");
  add_line(m, "#  #");

  m = new_module();
  m->category = MODULE_CHAPEL;
  add_line(m, "#  #");
  add_line(m, "#  #");
  add_line(m, "#  #");
  add_line(m, "####");
  add_line(m, "LLLL");
  add_line(m, "L##L");
  add_line(m, "L###");
  add_line(m, "LFF#");
  add_line(m, ".RR.");
  add_line(m, ".RR.");
  add_line(m, ".RR.");
  add_line(m, "LFF#");
  add_line(m, "L###");
  add_line(m, "L##L");
  add_line(m, "LLLL");
  add_line(m, "####");
  add_line(m, "#  #");
  add_line(m, "#  #");
  add_line(m, "#  #");

  m = new_module();
  m->category = MODULE_CHAPEL;
  add_line(m, "##");
  add_line(m, "##");
  add_line(m, "##");
  add_line(m, "##");
  add_line(m, "##");
  add_line(m, "##");
  add_line(m, "FF");
  add_line(m, "RR");
  add_line(m, "RR");
  add_line(m, "RR");
  add_line(m, "RR");
  add_line(m, "RR");
  add_line(m, "FF");
  add_line(m, "##");
  add_line(m, "##");
  add_line(m, "##");
  add_line(m, "##");
  add_line(m, "##");
  add_line(m, "##");

  m = new_module();
  m->category = MODULE_CHAPEL;
  add_line(m, "#  #");
  add_line(m, "#  #");
  add_line(m, "#  #");
  add_line(m, "####");
  add_line(m, "LL##");
  add_line(m, "L##L");
  add_line(m, "LLLL");
  add_line(m, "..LL");
  add_line(m, ".LL.");
  add_line(m, "....");
  add_line(m, "..LL");
  add_line(m, "LLLL");
  add_line(m, "#L#L");
  add_line(m, "###L");
  add_line(m, "##LL");
  add_line(m, "####");
  add_line(m, "#  #");
  add_line(m, "#  #");
  add_line(m, "#  #");

  m = new_module();
  m->category = MODULE_CHAPEL;
  add_line(m, "#  #");
  add_line(m, "#  #");
  add_line(m, "#  #");
  add_line(m, "####");
  add_line(m, "####");
  add_line(m, "####");
  add_line(m, "##LL");
  add_line(m, "LLLL");
  add_line(m, "LL..");
  add_line(m, "....");
  add_line(m, "...L");
  add_line(m, "L.LL");
  add_line(m, "L.LL");
  add_line(m, "L##L");
  add_line(m, "LL##");
  add_line(m, "####");
  add_line(m, "#  #");
  add_line(m, "#  #");
  add_line(m, "#  #");

  m = new_module();
  m->category = MODULE_CHAPEL;
  add_line(m, "#  ###");
  add_line(m, "#  ###");
  add_line(m, "#  ###");
  add_line(m, "######");
  add_line(m, "#L##LL");
  add_line(m, "LLLLLL");
  add_line(m, "LL##LL");
  add_line(m, "..##LL");
  add_line(m, ".....L");
  add_line(m, "......");
  add_line(m, "...LL.");
  add_line(m, "L.LL..");
  add_line(m, "LLL..L");
  add_line(m, "LL###L");
  add_line(m, "LL# ##");
  add_line(m, "### ##");
  add_line(m, "#   ##");
  add_line(m, "#   ##");
  add_line(m, "#   ##");

  m = new_module();
  m->category = MODULE_CHAPEL;
  add_line(m, "#  ##");
  add_line(m, "#####");
  add_line(m, "#L###");
  add_line(m, "#LL##");
  add_line(m, "LLL#L");
  add_line(m, "LLLLL");
  add_line(m, "LLLLL");
  add_line(m, "L..LL");
  add_line(m, ".....");
  add_line(m, "...LL");
  add_line(m, ".L..L");
  add_line(m, "LLLLL");
  add_line(m, "LLLLL");
  add_line(m, "LLL##");
  add_line(m, "##L##");
  add_line(m, "#####");
  add_line(m, "#   #");
  add_line(m, "#   #");
  add_line(m, "#   #");

  m = new_module();
  m->category = MODULE_CHAPEL;
  add_line(m, "#####");
  add_line(m, ".....");
  add_line(m, ".E.E.");
  add_line(m, ".....");
  add_line(m, "##X##");
  add_line(m, "#L.LL");
  add_line(m, "LL..L");
  add_line(m, "L..LL");
  add_line(m, ".....");
  add_line(m, ".....");
  add_line(m, ".L..L");
  add_line(m, "LL.LL");
  add_line(m, "LL..L");
  add_line(m, "L..##");
  add_line(m, "##X##");
  add_line(m, ".....");
  add_line(m, ".E.E.");
  add_line(m, ".....");
  add_line(m, "#####");

  m = new_module();
  m->category = MODULE_CHAPEL;
  add_line(m, "#####");
  add_line(m, ".....");
  add_line(m, ".E.E.");
  add_line(m, ".....");
  add_line(m, "##X##");
  add_line(m, "#L.##");
  add_line(m, "LL..L");
  add_line(m, "LLL.L");
  add_line(m, "L....");
  add_line(m, "...LL");
  add_line(m, "..LLL");
  add_line(m, "L.LLL");
  add_line(m, "L..LL");
  add_line(m, "L#.LL");
  add_line(m, "##X##");
  add_line(m, ".....");
  add_line(m, ".E.E.");
  add_line(m, ".....");
  add_line(m, "#####");

  m = new_module();
  m->category = MODULE_CHAPEL;
  add_line(m, "#  #");
  add_line(m, "#  #");
  add_line(m, "#  #");
  add_line(m, "####");
  add_line(m, "####");
  add_line(m, "LLL#");
  add_line(m, "LLLL");
  add_line(m, ".LLL");
  add_line(m, "....");
  add_line(m, "..LL");
  add_line(m, "L..L");
  add_line(m, "LLLL");
  add_line(m, "LLLL");
  add_line(m, "L##L");
  add_line(m, "####");
  add_line(m, "####");
  add_line(m, "#  #");
  add_line(m, "#  #");
  add_line(m, "#  #");

/*  m = new_module();
  m->category = MODULE_CHAPEL;
  add_line(m, " ############ ");
  add_line(m, "w............e");
  add_line(m, "w............e");
  add_line(m, "w.##..##..##.e");
  add_line(m, "w.##..##..##.e");
  add_line(m, "w............e");
  add_line(m, "w............e");
  add_line(m, "w............e");
  add_line(m, "w............e");
  add_line(m, "w............e");
  add_line(m, "w............e");
  add_line(m, "w............e");
  add_line(m, "w............e");
  add_line(m, "w............e");
  add_line(m, "w.##..##..##.e");
  add_line(m, "w.##..##..##.e");
  add_line(m, "w............e");
  add_line(m, "w............e");
  add_line(m, " ############ ");

  m = new_module();
  m->category = MODULE_CHAPEL;
  add_line(m, " ####              #### ");
  add_line(m, "w..##              ##..e");
  add_line(m, "w..##              ##..e");
  add_line(m, "w..##              ##..e");
  add_line(m, "w..##              ##..e");
  add_line(m, "w..##              ##..e");
  add_line(m, "w..##              ##..e");
  add_line(m, "w..##              ##..e");
  add_line(m, "w..##              ##..e");
  add_line(m, "w..##              ##..e");
  add_line(m, "w..##              ##..e");
  add_line(m, "w..##              ##..e");
  add_line(m, "w..##              ##..e");
  add_line(m, "w..##              ##..e");
  add_line(m, "w..##################..e");
  add_line(m, "w..##################..e");
  add_line(m, "w......................e");
  add_line(m, "w......................e");
  add_line(m, " ###################### ");*/


/*  m = new_module();
  m->category = MODULE_CHAPEL;
  add_line(m, " n  nn  n ");
  add_line(m, "#.##..##.#");
  add_line(m, "#.##..##.#");
  add_line(m, "#........#");
  add_line(m, "#.##..##.#");
  add_line(m, "#.##..##.#");
  add_line(m, " s  ss  s ");
  rotate_module(m);*/


  return;
} /* init_modules */

