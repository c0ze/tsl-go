/*
  options.h
*/

#ifndef _OPTIONS_H_
#define _OPTIONS_H_

blean_t parse_config_file(void);
void set_default_options(void);
void options_menu(void);


#define OPTION_NAME             0
#define OPTION_MORGUE           1
#define OPTION_AUTOEQUIP        2
#define OPTION_AUTOCENTER       3
#define OPTION_SAFE_TELEPORT    4
#define OPTION_GLYPH_MODE       5
#define OPTION_DIAGONALS        6
#define OPTION_ITEM_CATEGORIES  7

#define OPTION_MENU_LEN         15


/*
  Some options that can be set from command line or config file.
*/
struct
{
  blean_t autocenter;    /* Recenter automatically each step */
  blean_t glyph_mode;    /* Recenter automatically each step */
  blean_t fullscreen;    /* Should we try to go fullscreen? Graphical only. */
  blean_t large_cursor;  /* Should we use "very visible" cursor? */
  blean_t morgue;        /* Should we dump a morgue.txt at player death? */
  blean_t dotfloors;     /* Display floors as dots */
  blean_t autoequip;     /* Equip items automatically when picked up */
  char * default_name;   /* What default name should we use? */
  blean_t forcegetname;  /* Should we force the user to supply * a
			    name? Might be useful in a multi-user
			    environment, once we have scoreboards,
			    etc. */
  blean_t safe_teleport; /* Should we warn about unsafe teleportation? */
  //blean_t item_categories; /* Display categories in inventory? */
  int diagonals; /* Mode to enter diagonals */

  /* Internal use */
  blean_t debug_fov;
} options;

#define DIAGONALS_OFF     0
#define DIAGONALS_SLOPPY  1
#define DIAGONALS_STEP    2


#endif
