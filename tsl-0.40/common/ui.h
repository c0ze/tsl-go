#ifndef _UI_H_
#define _UI_H_

#include "main.h"
#include "tiles.h"
#include "gent.h"
#include "message.h"
#include "input.h"
#include "area.h"

int view_top;
int view_left;

int board_size_y;
int board_size_x;

int scroll_limit_y;
int scroll_limit_x;

#define MESSAGE_BAR_WIDTH 39
#define MESSAGE_LINES 4

#define INSERT_GENT (2)

unsigned int status_page;
void flip_status(void);
void display_stats(const creature_t * creature);
void status_first(const creature_t * creature);
void status_second(const creature_t * creature);
void status_melee(const creature_t * creature);
void status_ranged(const creature_t * creature);
void status_box(const char * title, const char * text, const unsigned int h);
void generic_box(const unsigned int t, const unsigned int l,
		 const unsigned int h, const unsigned int w,
		 const char * title, const char * text);
void st_upid_wrap(const unsigned int t,
		  const unsigned int l,
		  const unsigned int w,
		  const char * text);

void display_text(const char * text);

char * ai_state(const unsigned int state);

void draw_level(void);

/* Map viewport */
void center_view_y(const level_t * level, const int y);
void center_view_x(const level_t * level, const int x);
blean_t visible_on_board(const int y, const int x);
void recenter_if_needed(void);

enum missile_fx_t
{
  missile_explosion,
  missile_spell,
  missile_lightning,
  missile_frost,
  missile_poison,
  missile_fire,
  missile_web,
  missile_arrow,
  missile_flash
};
typedef enum missile_fx_t missile_fx_t;

void draw_missile(const unsigned int y, const unsigned int x,
		  const missile_fx_t missile_fx, const signed int move_y,
		  const signed int move_x);

unsigned int area_of_effect(creature_t * observer, area_t * area, missile_fx_t fx);

void wrap_text(const char * text);
void constrained_wrap(const unsigned int t, const unsigned int l,
		      const unsigned int w,
		      const char * text);

#define ST_WHAT  0
#define ST_HLINE 1
#define ST_VLINE 2
#define ST_RTEE  3
#define ST_LTEE  4
#define ST_TTEE  5
#define ST_BTEE  6
#define ST_CROSS 7
#define ST_SE    8
#define ST_SW    9
#define ST_NE    10
#define ST_NW    11
#define ST_FLOOR 12
#define ST_CURSOR 31


/*
  These are implementation-specific functions. _All_ screen I/O must
  be done through these. They follow the corresponding curses-function
  quite closely. With a terminal interface, they are little more than
  wrappers. With a graphical interface, they must emulate the same
  functionality.

  mb_* only affect the message area, st_* the status window, map_* the
  map and scr_* when the whole screen needs to be used.

  All sections of the screen do not support all actions.

  *_erase - clears to black
  *_addch, *_addstr - prints a string or a single character
  *_move - sets the y/x coordinates for the next output
  *_rev - turn "reverse" text mode on and off.
  *_special, *_gent - prints special characters or graphical entities
  *_getyx - 
  *_flush - copies everything written onto the screen
*/
void init_ui(void);
void shutdown_ui(void);
key_token_t get_keypress(void);

void game_over(const char * msg, const blean_t won);

/* UI module should set this to as many characters wide a gent is. */
extern const unsigned int gent_width;

void st_erase(void);
void st_flush(void);
void st_addch(const char c);
void st_addstr(const char * s);
void st_special(const unsigned int c);
void st_gent(const gent_t gent);
void st_move(const unsigned int y, const unsigned int x);
void st_rev(const blean_t new_state);
void st_getyx(unsigned int * y, unsigned int * x);
void st_dim(const blean_t new_state);

void mb_erase(void);
void mb_flush(void);
void mb_move(const unsigned int y, const unsigned int x);
void mb_addstr(const char * s);
void mb_more(void);
void mb_cursor(const blean_t new_state);

/* */
#define MAP_NONE    0
#define MAP_REVERSE 1
#define MAP_DIM     2
#define MAP_SLEEP   4
#define MAP_FLOOR   8  /* Force floor underneath; GUI only */

void scr_erase(void);
void scr_getyx(unsigned int * y, unsigned int * x);
void scr_move(const unsigned int y, const unsigned int x);
void scr_addgent(const gent_t gent, const unsigned int attr);
void scr_addch(const char c);
void scr_addstr(const char * s);
void scr_special(const unsigned int c);
void scr_flush(void);
void scr_rev(const blean_t new_state);

void map_erase(void);
void map_flush(void);

/* Shows/hides the map cursor */
void map_cursor(const blean_t new_state);

/*
  Only used for cursor positioning. map_put requires explicit
  coordinates each call, this will not affect where things end up.
*/
void map_move(const unsigned int y, const unsigned int x);
void map_put(const unsigned int y, const unsigned int x, const gent_t gent, const unsigned char attr);



enum anim_type_t
{
  anim_type_damage,
  anim_type_attention,
  anim_type_deathspell,
  anim_type_alert
};
typedef enum anim_type_t anim_type_t;



void add_anim(const int t, const int p0, const int p1, const int p2, const int p4);
void damage_popup(creature_t * creature, const int amount);


void set_glyph_mode(const blean_t new_mode);

#endif
