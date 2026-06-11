#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "main.h"
#include "stuff.h"
#include "bestiary.h"
#include "message.h"
#include "ui.h"
#include "browser.h"
#include "game.h"

/*#ifdef TSL_CONSOLE
#include "glyph.h"
#endif*/


/*
  Opens the bestiary screen and scrolls to ID, if it is in the bestiary.
*/
void bestiary(const int id)
{
  menu_item_t ** menu;
  int i;
  int k;
  int total_items;

  total_items = 0;

  /* Count how many monsters we know of */
  for (i = 0; i < monsters; i++)
  {
    if (game->monster_known[i])
      total_items++;
  }

  if (total_items == 0)
  {
    queue_msg("BUG: empty bestiary");
    return;
  }

  menu = malloc(sizeof(menu_item_t *) * total_items);

  if (menu == NULL)
    out_of_memory();

  k = 0;

  for (i = 0; i < monsters; i++)
  {
    if (game->monster_known[i])
    {
      menu[k] = alloc_menu_item();

      if (i == id)
	browser[MENU_BESTIARY].cur_pos = k;
      
      switch (i)
      {
	case monster_ghoul:
	  menu[k]->label = mydup("ghoul");
	  menu[k]->inspect = mydup("GHOUL (Z). \n \n These appear to be parts of different bodies sewn together, brought back to life by sick and twisted experiments. Denied both full life and true death, they feel only hunger.");
	  break;
	  
	case monster_ratman:
	  menu[k]->label = mydup("ratman");
	  menu[k]->inspect = mydup("RATMAN (r). \n \n No one knows if they were men at first, that lost their way and turned to beasts - or if they are escaped lab rats, the result of genetic engineering, scientific pride and folly. They are scavengers, weak in combat but dirty fighters.");
	  break;

	case monster_merman:
	  menu[k]->label = mydup("merman");
	  menu[k]->inspect = mydup("MERMAN (M). \n \n These fish-folk grew weary of the sun and sought a new home in the depths. \n \n Their slender bodies are covered in golden-green scales. They have fins along their spine and kelp-like hair. They communicate in high-pitched shrieks."); /* tide, oceangrave */
	  break;

	case monster_chrome_angel:
	  menu[k]->label = mydup("chrome angel");
	  menu[k]->inspect = mydup("CHROME ANGEL (A). \n \n They fell from the skies, denied the paradise they helped build, deprived of their creation. Their skin is polished metal and their blood is liquid light of every color. ");
	  break;

	case monster_graveling:
	  menu[k]->label = mydup("graveling");
	  menu[k]->inspect = mydup("GRAVELING (g). \n \n A six foot, hulking were-badger. ");
	  break;

	case monster_gloom_lord:
	  menu[k]->label = mydup("gloomlord");
	  menu[k]->inspect = mydup("GLOOM LORD (K). \n \n They mourn a kingdom fallen, for theirs is forever lost.");
	  break;

	case monster_mimic:
	  menu[k]->label = mydup("mimic");
	  menu[k]->inspect = mydup("MIMIC (m). \n \n ");
	  break;

	case monster_gnoblin:
	  menu[k]->label = mydup("gnoblin");
	  menu[k]->inspect = mydup("GNOBLIN (o). \n \n ");
	  break;

	case monster_slime:
	  menu[k]->label = mydup("slime");
	  menu[k]->inspect = mydup("SLIME (x). \n \n ");
	  break;

	case monster_giant_slimy_toad:
	  menu[k]->label = mydup("giant slimy toad");
	  menu[k]->inspect = mydup("GIANT SLIMY TOAD (Y). \n \n ");
	  break;

	case monster_floating_brain:
	  menu[k]->label = mydup("floating brain");
	  menu[k]->inspect = mydup("FLOATING BRAIN (b). \n \n ");
	  break;

	case monster_frostling:
	  menu[k]->label = mydup("frostling");
	  menu[k]->inspect = mydup("FROSTLING (f). \n \n ");
	  break;

	case monster_chainsaw_ogre:
	  menu[k]->label = mydup("chainsaw ogre");
	  menu[k]->inspect = mydup("CHAINSAW OGRE (f). \n \n These brutes have had one arm replaced with a chainsaw and the other with a grenade launcher. Gotta love these guys.");
	  break;

	case unique_necromancer:
	  menu[k]->label = mydup("the Necromancer");
	  menu[k]->inspect = mydup("NECROMANCER, THE (N). \n \n ");
	  break;

	case unique_king_of_worms:
	  menu[k]->label = mydup("the King of Worms");
	  menu[k]->inspect = mydup("KING OF WORMS, THE (W). \n \n ");
	  break;

	case unique_lurker:
	  menu[k]->label = mydup("the Lurker");
	  menu[k]->inspect = mydup("LURKER, THE (L). \n \n ");
	  break;

	default:
	  menu[k]->label = mydup("unknown");
	  menu[k]->inspect = mydup("This text has yet to be written.");
	  break;
      }

      k++;
    }
  }

  queue_msg("Viewing monster description...");
  msgflush_nowait();

  browse(menu, total_items, MENU_BESTIARY, NULL, NULL);
  
  clear_msgbar();

  del_menu(menu, total_items);

  return;
} /* describe_monster */
