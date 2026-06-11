#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "main.h"
#include "stuff.h"
#include "wingame.h"
#include "options.h"
#include "game.h"
#include "message.h"
#include "losegame.h"
#include "ui.h"


void win_game(const unsigned int ending)
{
  queue_msg("You ascend to demigodhood.");
  msgflush_wait();
  
  game->died = time(NULL);
  game->game_over = true;
  game->won = true;
  
  if (options.morgue)
  {
    morgue_dump("ascended to demigodhood");
  }

  game_over("You have won!\n", true);
  shutdown_everything();
  
  exit(0);
} /* win_game */
