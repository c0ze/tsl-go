#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <ctype.h>

#include "main.h"
#include "stuff.h"
#include "ui.h"
#include "inventory.h"
#include "level.h"
#include "effect.h"
#include "creature.h"
#include "item.h"
#include "game.h"
#include "player.h"
#include "traps.h"
#include "combat.h"
#include "shapeshf.h"
#include "fov.h"
#include "altitude.h"
#include "gent.h"
#include "input.h"
#include "ability.h"
#include "area.h"
#include "equip.h"
#include "options.h"



/*
  Draws CURRENT_LEVEL. What parts are displayed or not depends on what
  information is in VISIBILITY (game.h).
*/
void draw_level()
{
  signed int y;
  signed int x;
  unsigned int board_y;
  unsigned int board_x;

  level_t * level;

/*  int switch_value;*/
  gent_t in_memory;

  int i;

  item_t * item;
  trap_t * trap;
  creature_t * creature;

  blean_t seen;
  blean_t blinded;

  gent_t gent;

  level = get_current_level();

  if (level == NULL)
    return;

  map_erase();

  /*map_move(0, 0);*/

  blinded = attr_current(game->player, attr_blindness);
  seen = false;

  for (board_y = 0; board_y < board_size_y; board_y++)
  {
    for (board_x = 0; board_x < board_size_x; board_x++)
    {
      y = board_y + view_top;
      x = board_x + view_left;

      if (on_map(level, y, x) == false)
      {
	map_put(board_y, board_x, gent_blank, false);
	continue;
      }

      if (!blinded)
	seen = can_see(game->player, y, x);
      
      in_memory = level->memory[y][x];
      gent = in_memory;

      if (seen)
      {
	/*
	  This is so we always draw a floor. If we don't and there's a
	  trap in level memory this will retain the black
	  rectangle. This only matters to sdlui.
	*/

	if (level->map[y][x] == tile_floor)
	{
	  map_put(board_y, board_x, gent_floor, MAP_NONE);
	}
	else
	{
	  if (tile_info[level->map[y][x]]->draw_floor)
	    map_put(board_y, board_x, gent_floor, MAP_NONE);

	  map_put(board_y, board_x, gent, MAP_NONE);
	}
      }
      else
      {
	map_put(board_y, board_x, gent, MAP_DIM);
      }
    } /* for x */
  } /* for y */

  /*
    Display traps; these are drawn before items so the items
    appear on top of the traps.
  */
  for (trap = level->first_trap; trap != NULL; trap = trap->next_trap)
  {
    if (trap->hidden)
      continue;

    y = trap->y;
    x = trap->x;

    if (trap->revealed &&
	can_see(game->player, y, x) &&
	visible_on_board(y, x))
    {
      map_put(y - view_top, x - view_left, trap->gent, MAP_NONE);
    }
  } /* for trap */
  
  /*
    Display items; these are drawn before creatures so the creatures
    appear standing on top of the items.
  */
  for (item = level->first_item; item != NULL; item = item->next_item)
  {
    y = item->y;
    x = item->x;

    if (on_map(level, y, x) == false)
      continue;

    /* Items don't show in forcefields. */
    if (get_tile(level, y, x) == tile_forcefield)
      continue;

    if (visible_on_board(y, x) == false)
      continue;

    if (can_see(game->player, y, x))
      map_put(y - view_top, x - view_left, item->gent, MAP_NONE);
  } /* for item */
  
  /* Display creatures */
  for (i = 0; i < level->creatures; i++)
  {
    creature = level->creature[i];
    
    if (creature == NULL)
      continue;
    
    if (creature->detected == false)
      continue;
    
    y = creature->y;
    x = creature->x;

    if (visible_on_board(y, x) == false)
      continue;

    if (can_see_creature(game->player, creature))
    {
      unsigned int a;
      /*chtype glyph;*/
      
      a = 0;

      /*a = glyph_attr[creature->gent];
	glyph = glyph_map[creature->gent];*/
      
/*      if (get_effect_by_id(creature, effect_sleep))
	a = COLOR_PAIR(color_brown);
      else if (attr_current(creature, attr_poisoned))
	a = COLOR_PAIR(color_green);
      else if (is_stunned(creature))
      a = COLOR_PAIR(color_cyan);*/

      /* The creature is visible; display it. */
      
      /* Shapeshifted players should be displayed inverted. */
      if (is_player(creature))
      {
	if (creature->id != monster_player)
	  a |= MAP_REVERSE;
      }
      else if (attr_current(creature, attr_player_ally))
      {
	/*else if (enemies(creature, game->player) == false)*/
	a |= MAP_REVERSE;
      }
      else if (is_helpless(creature))
      {
	a |= MAP_SLEEP;
      }

      map_put(y - view_top, x - view_left, creature->gent, a);
    }
  } /* for i */

  map_flush();

  return;
} /* draw_level */



/*
  This is used by the debug system.
*/
/* TODO: Can probably be merged with or replaced by something else. */
void display_text(const char * text)
{
  st_erase();
  st_addstr(text);
  st_flush();

  return;
} /* display_text */



/*
  Draws a missile at {Y, X} travelling in direction {MOVE_Y, MOVE_X}.
*/
void draw_missile(const unsigned int y, const unsigned int x,
		  const missile_fx_t missile_fx,
		  const signed int move_y, const signed int move_x)
{
  gent_t gent = gent_undefined;

  if (visible_on_board(y, x) == false)
    return;

  switch (missile_fx)
  {
    case missile_frost:
      gent = gent_spell_frost;
      break;

    case missile_web:
      gent = gent_web;
      break;

    case missile_spell:
      if ((move_y == -1) && (move_x ==  0)) gent = gent_spell_n;
      else if ((move_y == -1) && (move_x == +1)) gent = gent_spell_ne;
      else if ((move_y ==  0) && (move_x == +1)) gent = gent_spell_e;
      else if ((move_y == +1) && (move_x == +1)) gent = gent_spell_se;
      else if ((move_y == +1) && (move_x ==  0)) gent = gent_spell_s;
      else if ((move_y == +1) && (move_x == -1)) gent = gent_spell_sw;
      else if ((move_y ==  0) && (move_x == -1)) gent = gent_spell_w;
      else if ((move_y == -1) && (move_x == -1)) gent = gent_spell_nw;
      break;
      
    case missile_explosion:
      gent = gent_explosion;
      break;

    case missile_poison:
      gent = gent_spell_poison;
      break;

    case missile_flash:
      gent = gent_flash;
      break;

    case missile_fire:
      gent = gent_spell_fire;
      break;

    case missile_arrow:
      if ((move_y == -1) && (move_x ==  0)) gent = gent_arrow_n;
      else if ((move_y == -1) && (move_x == +1)) gent = gent_arrow_ne;
      else if ((move_y ==  0) && (move_x == +1)) gent = gent_arrow_e;
      else if ((move_y == +1) && (move_x == +1)) gent = gent_arrow_se;
      else if ((move_y == +1) && (move_x ==  0)) gent = gent_arrow_s;
      else if ((move_y == +1) && (move_x == -1)) gent = gent_arrow_sw;
      else if ((move_y ==  0) && (move_x == -1)) gent = gent_arrow_w;
      else if ((move_y == -1) && (move_x == -1)) gent = gent_arrow_nw;
      break;

    case missile_lightning:
      break;
  }

  map_put(y - view_top, x - view_left, gent, MAP_NONE);

  map_flush();

  /*
    Possible RFE: This would be the ideal place to delay the program
    with a few fractions of a second, to give the missile a slightly
    more "animated" feel. Since no such function exists in neither
    the standard library nor ncurses (AFAIK), I've decided to leave
    it out for now. If added, it should of course be possible to
    disable this feature (or ever better, disabled by default, so
    that the player would have to enable it explicitly).
  */
      
  return;
} /* draw_spell_fx */



/*
  Returns a string representation of STATE. See creature.h.
*/
char * ai_state(const unsigned int state)
{
  switch (state)
  {
    case ai_idle:           return "Idle";
    case ai_offensive:      return "Offensive";
    case ai_retreat:        return "Retreat";
    case ai_target:         return "Target";
    case ai_player:         return "Player";
    case ai_mimic:          return "Mimic";
    default:                return "BUG";
  }
} /* ai_state */



/*
 */
void flip_status()
{
  if (status_page == 1)
    status_page = 0;
  else
    status_page = 1;
  
  return;
} /* flip_status */



/*
  Updates the status window.
*/
void display_stats(const creature_t * creature)
{
  char temp[200];
  char line[80];
/*  char title[80];*/

  st_erase();

  /* Display health and energy in a small box in the upper left */
  strcpy(temp, "");

  sprintf(line, "%6s %2d\n", "Health", attr_base(creature, attr_health));
  strcat(temp, line);

  /*
    RFE: I'm not sure how to handle this, it can display a higher
    current than max.
  */
  sprintf(line, "%6s %2d/%2d", "Energy",
	  attr_base(creature, attr_ep_current), 
	  attr_base(creature, attr_ep_max) +
	  attr_eq_mods(creature, attr_ep_max, true));
  strcat(temp, line);

  generic_box(0, 0,  4, 16, NULL, temp);

  /* Draw a small box in the upper right */
  generic_box(0, 34, 4, 5, NULL, "");

  st_move(3, 0);
  
  /* Draw the status page we're on. */
  if (status_page == 0)
    status_first(creature);
  else
    status_second(creature);

  /* Connect the health/energy box to the rest */
  st_move(3, 0);
  st_special(ST_LTEE);
  st_move(3, 15);
  st_special(ST_BTEE);

  /* Connect the small box */
  st_move(3, 38);
  st_special(ST_RTEE);
  st_move(3, 34);
  st_special(ST_BTEE);

  /*
    sprintf(line, "%ld/%ld\n", tick, game->turns);
    st_addstr(line);
  */

  if (is_player(creature) == false)
  {
    /*waddch(status_win, '\n');*/

    /* wprintw(status_win, "Carrying: %d/%d\n",
       get_carried_weight(creature),
       get_weight_allowance(creature));*/
    
    /*wprintw(status_win, "Aggravated: %d\n", creature->aggravated);*/
    /*    wprintw(status_win, "Detected: %d\n", creature->detected);*/
    /*wprintw(status_win, "Alignment: %d\n", creature->alignment);*/
    /*    wprintw(status_win, "ai_state: %s\n", ai_state(creature->ai_state));
    wprintw(status_win, "(last) target: {%d,%d}\n",
    creature->target_y, creature->target_x);*/
  }

  /* Update screen */
  st_flush();

  return;
} /* display_stats */



void status_first(const creature_t * creature)
{
  char temp[2000];
  unsigned int y;

  if (creature == NULL)
    return;

  /* Display weapon information */
  status_melee(creature);

  /* Display ranged weapon information */
  st_move(8, 0);
  status_ranged(creature);

  st_move(8, 0);
  st_special(ST_LTEE);
  st_move(8, 38);
  st_special(ST_RTEE);

  generic_box(12, 0, 12, 39, NULL, "");

  st_move(12, 0);
  st_special(ST_LTEE);
  st_move(12, 38);
  st_special(ST_RTEE);

  for (y = 13; y < 23; y++)
  {
    st_move(y, 19);
    st_special(ST_VLINE);
  }
  
  st_move(12, 19);
  st_special(ST_TTEE);
 
  /* Display temporary states */
  status_flags(creature, temp, flag_mode_stats);
  st_upid_wrap(13, 1, 18, temp);

  /* Display mapped abilities */
  list_shortcuts(temp, false);
  st_upid_wrap(13, 20, 18, temp);

  st_move(23, 19);
  st_special(ST_BTEE);

  return;
} /* status_first */



/*
  Prints at the current location in the status display a box
  describing what ranged action CREATURE currently has selected.
*/
void status_ranged(const creature_t * creature)
{
  item_t * weapon;
  item_t * ammo;
  item_t * wand;
  char * name;
/*  signed int t;*/
  unsigned int temp;
  char tmp[1000];
  char line[100];

  if (creature == NULL)
    return;

  strcpy(tmp, "");

  /* Does the creature have any wand equipped? */
  wand = get_equipped(creature, item_type_wand, 0);

  if (wand == NULL)
    wand = get_equipped(creature, item_type_tool, 0);

  if (wand != NULL)
  {
    name = get_item_name(wand);
    sprintf(line, "%c) %s", wand->letter, name);
    free(name);
    name = NULL;

    strcat(tmp, line);

    status_box("Ranged", tmp, 5);
    return;
  }
  
  /* No wand equipped, list ranged weapon and/or missiles instead. */
  weapon = get_equipped(creature, item_type_r_weapon, 0);
  
  if (weapon != NULL)
  {
    name = get_item_name(weapon);
    sprintf(line, "%c) %s", weapon->letter, name);
    free(name);
    name = NULL;

    strcat(tmp, line);

    if (identified(weapon) & known_name)
    {
      sprintf(line, "\n   Range: %d",
	      get_weapon_range(weapon));
      strcat(tmp, line);
    }

    strcat(tmp, "\n");
  }

  /*
    Display missile information. Weapon is still the current missile
    weapon, or NULL if we don't have any.
  */
  ammo = get_equipped(creature, item_type_ammo, 0);
  
  if (ammo == NULL)
  {
    if (weapon)
      strcat(tmp, "   (no ammo)");
  }
  else /* Has ammo */
  {
    name = get_item_name(ammo);

    sprintf(line, "%c) %s", ammo->letter, name);
    strcat(tmp, line);

    if (identified(ammo) & known_name)
    {
      temp = get_max_damage(ammo);
      
      if (temp != 0)
      {
	sprintf(line, " (%d-%d)", get_min_damage(ammo), temp);
	strcat(tmp, line);
      }
      else
      {
	temp = get_min_damage(ammo);
	
	if (temp != 0)
	{
	  sprintf(line, " (+%d)", temp);
	  strcat(tmp, line);
	}
      }
    }
    
    /* If we don't have any missile weapon equipped: display how far we can throw this ammo. */
    if (weapon == NULL)
    {
      sprintf(line, "\n   Throw Range: %d", attr_known(creature, attr_throw_range));
      strcat(tmp, line);
    }

    free(name);
    name = NULL;
  }
  
  status_box(NULL, tmp, 5);
  
  return;
} /* status_ranged */



/*
  Prints at the current location in the status display a box
  describing what melee attack CREATURE currently has selected.
*/
void status_melee(const creature_t * creature)
{
  int i;
//  int j;
  int k;
  signed int t;
  item_t * weapon[2];
  blean_t display_letter;
  char * name;
  char tmp[1000];
  char line[2][400];
  effect_t * temp_weapon;
/*  blean_t show_wait;*/

  if (creature == NULL)
    return;

  tmp[0] = '\0';
  line[0][0] = '\0';
  line[1][0] = '\0';

  temp_weapon = get_effect_by_id(creature, effect_temp_weapon);

  /*
    Check if we're using a temporary/magic weapon. This takes
    precedence over anything equipped.
  */
  if (temp_weapon != NULL)
  {
    /* Temporary weapons cannot be indexed by letter */
    display_letter = false;

    weapon[0] = weapon[1] = virtual_weapon[temp_weapon->param[EFFECT_VWEAPON_INDEX]];
  }
  else
  {
    display_letter = true;

    weapon[0] = get_equipped(creature, item_type_m_weapon, 0);
    weapon[1] = get_equipped(creature, item_type_m_weapon, 1);

    if (weapon[1] == NULL)
      weapon[1] = weapon[0];
  }

  /*
    If we don't have any temporary weapon nor anything equipped -
    fall back to the default unarmed attack.
  */
  if (weapon[0] == NULL)
  {
    display_letter = false;

    weapon[0] = weapon[1] = get_unarmed_weapon(creature);
  }
    
  /* Try not to crash. */
  if (weapon[0] == NULL)
    return;

  /*
    Build a descriptive string of the weapons in the two lines.
  */
  for (i = 0; i < 2; i++)
  {
    name = get_item_name(weapon[i]);
    
    if (display_letter)
    {
      sprintf(line[i], "%c) %s", weapon[i]->letter, name);
    }
    else
    {
      sprintf(line[i], "   %s", name);
    }

    free(name);
    name = NULL;

    if (identified(weapon[i]) & known_name)
    {
      /* Backstab */
      t = get_item_mod(weapon[i], attr_i_backstab);
      
      if (t > 0)
      {
	sprintf(line[i] + strlen(line[i]), " (backstab %d", t);
	
	if ((t = attr_base(creature, attr_backstab_bonus)) > 0)
	  sprintf(line[i] + strlen(line[i]), " + %d", t);
	
	if ((t = attr_eq_mods(creature, attr_backstab_bonus, true)) > 0)
	  sprintf(line[i] + strlen(line[i]), " + %d", t);
	
	sprintf(line[i] + strlen(line[i]), "%%)");
      }

      /* Wound */
      t = get_item_mod(weapon[i], attr_i_wound);

      if (t > 0)
      {
	sprintf(line[i] + strlen(line[i]), " (wound %d%%)", t);
      }
      
      /* Stun */
      t = get_item_mod(weapon[i], attr_i_stun);

      if (t > 0)
      {
	sprintf(line[i] + strlen(line[i]), " (stun: %d%%)", t);
      }
	
    }

    /* Newline before damage sequence - we always add this */
    strcat(line[i], "\n");
	
    /*
      Only display the damage sequence if the weapon has been identified.
    */
    if (1) // identified(weapon[i]) & known_name)
    {
      for (k = 0; k < WEAPON_ATKSEQLEN; k++)
      {
	/* Have we reached the end of the sequence? */
	if (weapon[i]->custom[WEAPON_ATKSEQ + k] == 0)
	  break;
	
	/*
	  If the creature is in this sequence position and this weapon
	  does equal or more damage than the other (this is why we
	  need two weapon pointers even if they are the same item)
	  show this with a small bracket.
	*/
	if ((creature->attack_pos == k) &&
	    (weapon[i]->custom[WEAPON_ATKSEQ + k] >= weapon[(i == 0 ? 1 : 0)]->custom[WEAPON_ATKSEQ + k] ||
	     weapon[i]->custom[WEAPON_ATKSEQ_ID + k] == false ||
	     weapon[(i == 0 ? 1 : 0)]->custom[WEAPON_ATKSEQ_ID + k] == false))
	{
	  strcat(line[i], ">");
	}
	else
	{
	  strcat(line[i], " ");
	}
	
	if (weapon[i]->custom[WEAPON_ATKSEQ + k] == -2)
	{
	  strcat(line[i], "  ");
	}
	else if (weapon[i]->custom[WEAPON_ATKSEQ + k] == -1)
	{
	  strcat(line[i], "_ ");
	}
	else
	{
	  if (identified(weapon[i]) & known_name ||
	      weapon[i]->custom[WEAPON_ATKSEQ_ID + k])
	  {
	    sprintf(line[i] + strlen(line[i]), "%-2d", weapon[i]->custom[WEAPON_ATKSEQ + k]);
	  }
	  else
	  {
	    strcat(line[i], "? ");
	    break;
	  }
	}
      }
      
      strcat(tmp, " ");
  
    } /* identified */
  }

  /* If both weapons are the same we only display one of them. */
  if (weapon[0] != weapon[1])
    sprintf(tmp, "%s\n%s", line[0], line[1]);
  else
    sprintf(tmp, "%s", line[0]);

  status_box(NULL, tmp, 6);
  
  return;
} /* status_melee */



void generic_box(const unsigned int t, const unsigned int l,
		 const unsigned int h, const unsigned int w,
		 const char * title, const char * text)
{
  unsigned int len;
  unsigned int pos;
  unsigned int y;
  unsigned int x;
  char temp[100];

  if (text == NULL)
    return;

  y = t;

  st_move(y, l);

  /* Open box */
  st_special(ST_SE);
  st_special(ST_HLINE);

  if (title)
  {
    len = MIN(strlen(title), w - 5);
    strncpy(temp, title, len);
    temp[len] = '\0';
    st_addch(' ');
    st_addstr(temp);
    st_addch(' ');
  }
  else
  {
    len = 0;
    st_special(ST_HLINE);
    st_special(ST_HLINE);
  }

  for (x = 0; x < MAX(0,w - 5 - len); x++)
    st_special(ST_HLINE);
  
  st_special(ST_SW);

  len = strlen(text);
  pos = 0;

  y++;

  st_move(y, l);
  st_special(ST_VLINE);
  st_addch(' ');

  if (len)
  {
    while (pos <= len)
    {
      if (text[pos] == '\n' || text[pos] == '\0')
      {
	st_move(y, l + w - 1);
	st_special(ST_VLINE);
	y++;
	st_move(y, l);
	
	if (text[pos] == '\n')
	{
	  /* Start next row. Add a space between the box and the content. */
	  st_special(ST_VLINE);
	  st_addch(' ');
	}
      }
      else
	st_addch(text[pos]);
      
      pos++;
    }
  }

  while (y < t + h - 1)
  {
    st_move(y, l);
    st_special(ST_VLINE);

    for (x = 0; x < w - 2; x++)
      st_addch(' ');
  
    st_special(ST_VLINE);

    y++;
  }

  st_move(y, l);

  /* Close box */
  st_special(ST_NE);

  for (x = 0; x < w - 2; x++)
    st_special(ST_HLINE);
  
  st_special(ST_NW);

  y++;
  st_move(y, l);

  return;
} /* generic_box */



void status_box(const char * title, const char * text, const unsigned int h)
{
  unsigned int y;
  unsigned int x;

  st_getyx(&y, &x);
  generic_box(y, x, h, 39, title, text);

  return;
} /* status_box */



void status_second(const creature_t * creature)
{
  unsigned int i;
  unsigned int ai = 0;
  signed int base;
  signed int eq_mods;
  signed int total;
  char line[100];
  char temp[2000];

  if (creature == NULL)
    return;

  temp[0] = '\0';
  
  /* Display personal information about the creature */
  /*
    Here we use native->name so we'll always get the real player
    name, not the name of whatever creature it might be shapeshifted
    into right now. When the player is shapeshifted, display the
    name of the creature it is shapeshifted into within
    parentheses.
  */
  if (is_player(game->player))
  {
    if (player_shapeshifted())
      sprintf(line, "You are %s (%s).\n", game->native->name_only, game->player->name_one);
    else
      sprintf(line, "You are %s.\n", game->native->name_only);
  }
  else
  {
    sprintf(line, "This is %s.\n", creature->name_only);
  }

  strcat(temp, line);

  if (is_player(game->player))
  {
    sprintf(line, "You are in %s.\n\n", get_current_level()->name);
    strcat(temp, line);
  }

  strcpy(line, "You recover ");
  switch (attr_current(creature, attr_magic))
  {
    case 1:  strcat(line, "1 EP/10 turns"); break;
    case 2:  strcat(line, "1 EP/9 turns"); break;
    case 3:  strcat(line, "1 EP/8 turns"); break;
    case 4:  strcat(line, "1 EP/7 turns"); break;
    case 5:  strcat(line, "1 EP/6 turns"); break;
    case 6:  strcat(line, "1 EP/5 turns"); break;
    case 7:  strcat(line, "1 EP/4 turns"); break;
    case 8:  strcat(line, "1 EP/3 turns"); break;
    case 9:  strcat(line, "1 EP/2 turns"); break;
    case 10: strcat(line, "1 EP/turn"); break;
    case 11: strcat(line, "2 EP/turn"); break;
    case 12: strcat(line, "3 EP/turn"); break;
    case 13: strcat(line, "4 EP/turn"); break;
    case 14: strcat(line, "5 EP/turn"); break;
    case 15: strcat(line, "6 EP/turn"); break;
    case 16: strcat(line, "7 EP/turn"); break;
    case 17: strcat(line, "8 EP/turn"); break;
    case 18: strcat(line, "9 EP/turn"); break;
    case 19: strcat(line, "10 EP/turn"); break;
    default: strcat(line, "BUG: abnormal value"); break;
  }
  strcat(line, ".\n");

  strcat(temp, line);

  /* Display attributes */
  for (i = 0; i < 50; i++)
  {
    /*
      This sets the index of the attribute the rest of the loop should
      work with.
    */
    switch(i)
    {
      case 0:  ai = attr_speed;                   break;
/*      case 1:  ai = attr_stealth;                 break;
	case 2:  ai = attr_perception;              break;*/
      case 3:  ai = attr_vision;                  break;
/*      case 4:  ai = attr_attack;                  break;
	case 5:  ai = attr_dodge;                   break;*/
      case 10: st_addch('\n'); continue;
      case 20: ai = attr_swimming;                break;
      case 21: ai = attr_throw_range;             break;
      case 25: st_addch('\n'); continue;
      case 40: ai = attr_absorption;              break;
      case 41: ai = attr_fire_resistance;         break;
      case 42: ai = attr_cold_resistance;         break;
      case 43: ai = attr_acid_resistance;         break;
      case 44: ai = attr_electricity_resistance;  break;
      case 45: ai = attr_poison_resistance;       break;

      default: continue;
    }
    
    /* Get the values for attribute[ai] */
    base    = attr_base(creature, ai);
    eq_mods = attr_eq_mods(creature, ai, true);
    
    /*
      This is not "current" - we don't display spell effects or
      unidentified items. We just sum the "visible" components of the
      attribute.
    */
    total = base + eq_mods;
    
    if (total == 0)
      continue;
      
    switch (ai)
    {
      case attr_swimming:
	if (attr_known(creature, attr_free_swim) > 0)
	  strcat(temp, "\nYou can swim indefinitely.");
	else
	  sprintf(temp + strlen(temp), "\nYou can swim %d (%d) turns.", total, base);
	break;
    
      case attr_throw_range:
	sprintf(temp + strlen(temp), "\nYou can throw %d tiles.", total);
	break;
    
      case attr_speed:
	sprintf(temp + strlen(temp), "\nYour speed is %d.", total);
	break;
    
      case attr_vision:
	sprintf(temp + strlen(temp), "\nYou can see %d tiles.", total);
	break;
    
      case attr_poison_resistance:
	if (total >= POISON_IMMUNITY)
	  sprintf(temp + strlen(temp), "\nYou are immune to poison.");
	else
	  sprintf(temp + strlen(temp), "\nPoison Resistance: %d", total);

	break;
	  
      case attr_fire_resistance:
	sprintf(temp + strlen(temp), "\nYou resist %d points of fire damage.", total);
	break;

      case attr_cold_resistance:
	sprintf(temp + strlen(temp), "\nYou resist %d points of cold damage.", total);
	break;

      case attr_acid_resistance:
	sprintf(temp + strlen(temp), "\nYou resist %d points of acid damage.", total);
	break;

      case attr_electricity_resistance:
	sprintf(temp + strlen(temp), "\nYou resist %d points of electrical damage.", total);
	break;
	
      default:
	sprintf(temp + strlen(temp), "\n%13s: %2d (%2d)", attr_info[ai]->name, total, base);
	break;
    } /* switch */

  } /* for */

  generic_box(3, 0, 21, 39, NULL, temp);
  
  return;
} /* status_second */



void center_view_y(const level_t * level, const int y)
{
  if (level == NULL)
    return;

  if (level->size_y <= board_size_y)
    view_top = 0 - MAX(1, board_size_y - level->size_y) / 2;
  else
  {
    view_top = MAX(0,
		   MIN(level->size_y - board_size_y + 1,
		       y - (board_size_y / 2)));
  }

  return;
} /* center_view_y */



void center_view_x(const level_t * level, const int x)
{
  if (level == NULL)
    return;

  if (level->size_x <= board_size_x)
    view_left = 0 - MAX(1, board_size_x - level->size_x) / 2;
  else
  {
    view_left = MAX(0,
		    MIN(level->size_x - board_size_x + 1,
			x - (board_size_x / 2)));
  }

  return;
} /* center_view_x */



/*
  Return whether {Y, X} is within the current map view.
*/
blean_t visible_on_board(const int y, const int x)
{
  if ((y - view_top < 0) ||
      (y - view_top >= board_size_y) ||
      (x - view_left < 0) ||
      (x - view_left >= board_size_x))
  {
    return false;
  }
  else
    return true;
} /* visible_on_board */



/*
  Recenters the view around the player if they have moved closed
  enough to a screen edge.
*/
void recenter_if_needed()
{
  if (game->player == NULL)
    return;

  if (options.autocenter ||
      game->player->y - view_top < scroll_limit_y ||
      game->player->y - view_top >= board_size_y - scroll_limit_y)
  {
    center_view_y(get_current_level(), game->player->y);
  }
  
  if (options.autocenter ||
      game->player->x - view_left < scroll_limit_x ||
      game->player->x - view_left >= board_size_x - scroll_limit_x)
  {
    center_view_x(get_current_level(), game->player->x);
  }

  return;
} /* recenter_if_needed */



unsigned int area_of_effect(creature_t * observer, area_t * area, missile_fx_t fx)
{
  unsigned int ret;
  unsigned int y;
  unsigned int x;
  area_t * temp_area;

  if (area == NULL ||
      observer == NULL)
    return 0;

  ret = 0;

  for (temp_area = area; temp_area != NULL; temp_area = temp_area->next)
  {
    y = temp_area->y;
    x = temp_area->x;

    if (can_see(observer, y, x))
    {
      ret++;
      draw_missile(y, x, fx, 0, 0);
    }
  }

  return ret;
} /* area_of_effect */



void constrained_wrap(const unsigned int t, const unsigned int l, const unsigned int w, const char * text)
{
  char * tmp;
  char * token;
  unsigned int x;
  unsigned int y;
  unsigned int len;
  unsigned int count;

  x = 0;
  y = t;
  tmp = mydup(text);
  token = strtok(tmp, " ");

  scr_move(y, l);

  while (token != NULL)
  {
    len = strlen(token);

    if (token[0] == '\n')
      goto new_line;
    
    /* If this token will fall outside the screen, move to the next row before we print it. */
    if (x + len + 1 >= w)
    {
      y++;
      x = 0;
      scr_move(y, l);
    }

    if (token[0] == INSERT_GENT)
    {
      char * shit;
      count = 0;
      shit = token;
      shit++;

      while (*shit != INSERT_GENT && *shit != '\0')
      {
	count *= 10;
	count += ((*shit) - '0');
	shit++;
      }

      scr_addstr("(");
      scr_addgent(count, MAP_NONE);
      scr_addstr(")");
      x += gent_width + 4;

      if (shit != '\0')
      {
	token = ++shit;
	len = strlen(token);
	/* Go on and print the rest of the token */
      }
      else
      {
	strtok(NULL, " ");
	continue;
      }
    }

    if (len > 0 && token[len - 1] == '\n')
    {
      /*
	The token ends with a newline. Trim the end, print the word
	and move to the next line. Note that this does not handle
	arbitrary newlines, only one at the end.
      */

      token[len - 1] = '\0';
      scr_addstr(token);

      y++;
      x = 0;
      scr_move(y, l);
    }
    else if (token[0] == '\n')
    {
    new_line:
      /* The token starts with a newline. Move to the next line. */
      y++;
      x = 0;
      scr_move(y, l);
    }
    else
    {
      /* It's a regular word. Keep track how far we've written. */
      x += len + 1;
      scr_addstr(token);
      scr_addch(' ');
    }

    token = strtok(NULL, " ");
  }

  free(tmp);

  return;
} /* constrained_wrap */



void wrap_text(const char * text)
{
  unsigned int y;
  unsigned int x;

  scr_getyx(&y, &x);
  constrained_wrap(y, x, 80, text);
  scr_flush();

  return;
} /* wrap_text */



void st_upid_wrap(const unsigned int t,
	    const unsigned int l,
	   const unsigned int w,
	    const char * text)
{
  unsigned int y;
/*  unsigned int x;*/
  unsigned int p;

  if (text == NULL)
    return;

  y = t;
/*  x = l;*/

  st_move(y, l);

  p = 0;

  for (p = 0; text[p] != '\0'; p++)
  {
    if (text[p] == '\n')
      st_move(++y, l);
    else
      st_addch(text[p]);
  }

  return;
} /* st_upid_wrap */


void set_glyph_mode(const blean_t new_mode)
{

#ifdef TSL_CONSOLE

  options.glyph_mode = true;

  board_size_y = 20;
  board_size_x = 40;

#else

  options.glyph_mode = new_mode;

  if (options.glyph_mode)
  {
    board_size_y = 20;
    board_size_x = 40;
  }
  else
  {
    board_size_y = 20;
    board_size_x = 20;
  }
#endif

  return;
}
