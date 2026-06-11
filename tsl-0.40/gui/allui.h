#ifndef TSL_GUI
#error TSL_GUI not set
#endif

#ifndef _ALLUI_H_
#define _ALLUI_H_

#include <allegro5/allegro.h>
#include <allegro5/allegro_image.h>
#include <allegro5/allegro_font.h>

#include "../common/main.h"
#include "../common/gent.h"
#include "../common/game.h"
#include "../common/ui.h"
#include "../common/options.h"
#include "../common/stuff.h"
#include "../common/glyph.h"
#include "../common/clipbrd.h"
#include "../common/anim.h"

int terminal[80 * 24];
int terminal_flags[80 * 24];

#define TERM_MAP(y, x)  ((y) * TERM_W + (x))

#define TERM_NONE       0
#define TERM_GENT       1
#define TERM_EXTENDED   2
#define TERM_REVERSE    4
#define TERM_DIM        8
#define TERM_FLOOR      16

#define TERM_H 24
#define TERM_W 80

blean_t mouse_pos(int * d_x, int * d_y);
void select_text(void);
void copy_text(void);

struct term_sel_t
{
  blean_t active;
  int t;
  int l;
  int b;
  int r;
  int down_x;
  int down_y;
  int up_x;
  int up_y;
} term_sel;


blean_t st_reverse;
blean_t st_d;
unsigned int st_y;
unsigned int st_x;

blean_t mb_c;
unsigned int mb_y;
unsigned int mb_x;

/*
int mb_t;
#define MB_CURSOR_CYCLE 50
#define MB_CURSOR_BLINK 5
*/

blean_t map_c;
unsigned int map_y;
unsigned int map_x;

blean_t scr_reverse;
unsigned int scr_y;
unsigned int scr_x;

#define FONT_W  10
#define FONT_H  20

#define TILE_W  20
#define TILE_H  20

#define ST_LEFT 400
#define MB_TOP  400

ALLEGRO_BITMAP * disp;
ALLEGRO_DISPLAY * realdisp;
ALLEGRO_BITMAP * tileset;
ALLEGRO_BITMAP * tileset_rev;
ALLEGRO_BITMAP * tileset_dim;
ALLEGRO_BITMAP * mainfont;
ALLEGRO_BITMAP * mainfont_rev;
ALLEGRO_BITMAP * mainfont_dim;
ALLEGRO_FONT * small_font;

ALLEGRO_EVENT_QUEUE * event_queue = NULL;
ALLEGRO_TIMER * timer = NULL;

struct rect_t
{
  int h;
  int w;
  int t;
  int l;
};
typedef struct rect_t rect_t;

blean_t load_fonts(void);
blean_t load_graphics(void);

void render_terminal(void);

void gent_rect(const gent_t gent, rect_t * rect);

blean_t set_resolution(const blean_t use_fullscreen);

blean_t load_bitmap(ALLEGRO_BITMAP ** dest, const char * file);

blean_t make_alt_bitmaps(void);
void configure_scaler(void);

float scaler_ratio;
int scaler_x_offset;
int scaler_y_offset;


void remap_glyph(const gent_t gent, int * glyph, int * attr);


#define SCREEN_W 800
#define SCREEN_H 480



int x_move;
int y_move;
int joy_press;
int x_held;
int y_held;


#endif
