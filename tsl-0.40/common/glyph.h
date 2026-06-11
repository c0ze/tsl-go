/*
  glyph.h

  Glyphs are our interface for displaying map data in a terminal.

  For each graphical entity in gent.h, there should be a corresponding
  line in init_glyph_map() to decide what to display it as.
*/

#ifndef _GLYPH_H_
#define _GLYPH_H_

#ifdef TSL_CONSOLE
#include <curses.h>
#else
#define ACS_BLOCK '?'
//#define chtype char
typedef char chtype;

#define A_REVERSE 0
#define A_NORMAL 0
#define ACS_CKBOARD 0
#define ACS_VLINE 0
#define ACS_HLINE 0
#define ACS_ULCORNER 0
#define ACS_URCORNER 0
#define ACS_LLCORNER 0
#define ACS_LRCORNER 0
#define ACS_LTEE 0
#define ACS_RTEE 0
#define ACS_BTEE 0
#define ACS_TTEE 0
#define ACS_PLUS 0
#endif

#include "main.h"
#include "tiles.h"
#include "gent.h"

/* Glyph mappings */
chtype glyph_map[gent_max];

/* Glyph attributes */
int glyph_attr[gent_max];

char glyph_convert[gent_max][5];

void init_glyph_map(void);

void set_glyph(const unsigned int gent, const chtype glyph, const int attr);
void ext_glyph(const unsigned int gent, const chtype glyph, const int attr, const char * convert);

#ifdef TSL_CONSOLE
void put_glyph(WINDOW * w,
	       const unsigned int y, const unsigned int x,
	       const unsigned int gent);

void put_custom(WINDOW * w, const unsigned int y,
		const unsigned int x,
		chtype c, const unsigned int a);
#endif

#endif
