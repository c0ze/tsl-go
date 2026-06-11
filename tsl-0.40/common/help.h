/*
  help.h
*/

#ifndef _HELP_H_
#define _HELP_H_

#include "ui.h"

#define HELP_KEYBOARD    0
#define HELP_LEGEND      1
#define HELP_MOVEMENT    2
#define HELP_INVENTORY   3
#define HELP_EQUIPMENT   4
#define HELP_ITEMS       5
#define HELP_CHARACTER   6
#define HELP_HAZARDS     7
#define HELP_ADVANCEMENT 8
#define HELP_COMBAT      9
#define HELP_DEATH       10
#define HELP_BACKSTAB    11

#define HELP_PAGES       11

#define HELP_STATUS      20


/* The help page currently being viewed */
unsigned int help_page;


void help(void);
void key_reference(void);
void display_legend(void);
void display_help_page(const unsigned int index);
unsigned int list_key(const unsigned int y, unsigned int x,
		      const unsigned int just, const unsigned int action);

#endif
