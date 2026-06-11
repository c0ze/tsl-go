#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "main.h"
#include "stuff.h"
#include "inscript.h"
#include "message.h"
#include "game.h"
/*#include "ui.h"*/


char * inscr_type[] =
{
  "Something is engraved here.",
  "Something is written in the dust here.",
  "Something has been written here... in blood.",
  "Something has been smeared in blood here.",
  "Something has been hastily scribbled here.",
  "Something is written in runes here.",
  "You come across a cryptic inscription.",
  "You come across a curious inscription."
};


char * inscr_message[] =
{
  "\"YOU ARE HERE\"",
  "\"U STEEL FOOD = U DIE\"",
  "\"I keep the wisdom that you need, the password that you want.\"",
  "\"The runes are ours to use.\"",
  "\"They took away my eyes, my beautiful eyes...\"",
  "\"Sick of you, sick of me, sick and tired of life\"",
  "\"Wherever I go in this world, I fight my war.\"",
  "\"Are we destined to do this forever, you and I? I'll laugh when you admit defeat.\"",
  "\"Never let me go. Hold me, with your everlasting love.\"",
  "\"This is my very last crusade.\"",
  "\"LIFE IS PAIN\"",
  "",
  "",
  "",
  "",
};



void inscription(const unsigned int type, const unsigned int message)
{
  if (type >= INSCR_TYPES)
    queue_msg("BUG: bad type for inscription()");
  else
    queue_msg(inscr_type[type]);

  msgflush_wait();

  if (attr_current(game->player, attr_p_read))
    queue_msg("Unfortunately, you can not read.");
  else if (message >= INSCR_MESSAGES)
    queue_msg("BUG: bad message for inscription()");
  else
    queue_msg(inscr_message[message]);
  
  msgflush_wait();
  clear_msgbar();

  return;
} /* inscription */
