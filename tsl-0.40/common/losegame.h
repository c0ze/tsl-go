/*
  losegame.h

  What happens after we die?

  Not much, we print a message and some statistics to a file.
*/

#ifndef _LOSEGAME_H_
#define _LOSEGAME_H_

#include "main.h"
#include "creature.h"
#include "magic.h"

void check_for_player_death(const char * reason);
void morgue_dump(const char * cause_of_death);
char * build_character_dump(void);
char * get_death_message(const char * reason, const blean_t goodbye);

#endif
