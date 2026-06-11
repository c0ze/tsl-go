#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "main.h"
#include "stuff.h"
#include "help.h"
#include "ui.h"
#include "rules.h"
#include "item.h"
#include "options.h"

#ifdef TSL_CONSOLE
#include "glyph.h"
#endif


/*  sprintf(page, "Dual-Wield lets you wield two weapons at the same time. For every attack, you will attack once with each weapon. You may only use dual-wield weapons that require one hand (so staves and some axes, clubs and swords are excluded) and you may not use a shield. You receive a %+d to-hit penalty for both weapons when dual-wielding.",*/


/*
  Opens the help system.
*/
void help()
{
  unsigned int key;
  action_t action;
  unsigned int i;
  char title[81];
  char scrollmsg[80];
  char label_next[6];
  char label_previous[6];
  char label_cancel[6];

  key_help(label_next,     false, action_next);
  key_help(label_previous, false, action_previous);
  key_help(label_cancel,   false, action_cancel);

  sprintf(scrollmsg, "(browse with %s/%s, %s exits)",
	  label_next, label_previous, label_cancel);

  while (1)
  {
    sprintf(title, "PAGE %2d/%2d: ", help_page + 1, HELP_PAGES);

    switch (help_page)
    {
      case HELP_KEYBOARD:    strcat(title, "KEYBOARD REFERENCE"); break;
      case HELP_LEGEND:      strcat(title, "LEGEND"); break;
      case HELP_MOVEMENT:    strcat(title, "MOVEMENT"); break;
      case HELP_INVENTORY:   strcat(title, "INVENTORY"); break;
      case HELP_EQUIPMENT:   strcat(title, "EQUIPMENT"); break;
      case HELP_ITEMS:       strcat(title, "ITEMS"); break;
      case HELP_CHARACTER:   strcat(title, "YOUR CHARACTER"); break;
      case HELP_ADVANCEMENT: strcat(title, "ADVANCEMENT"); break;
      case HELP_DEATH:       strcat(title, "HOW IT ENDS"); break;
      case HELP_HAZARDS:     strcat(title, "ENVIRONMENTAL HAZARDS"); break;
      case HELP_COMBAT:      strcat(title, "BASIC COMBAT"); break;
      case HELP_BACKSTAB:    strcat(title, "ADVANCED COMBAT: BACKSTABBING"); break;
      default: strcat(title, "BUG???"); break;
    }

    /* Fill the string to 80 spaces, attach scroll message at the end. */
    i = strlen(title);
    
    while (i < 80)
      title[i++] = ' ';

    title[80] = '\0';

    i = 80 - strlen(scrollmsg);
    strcpy(title + i, scrollmsg);

    scr_erase();
    scr_rev(true);
    scr_move(0, 0);
    scr_addstr(title);
    scr_rev(false);

    display_help_page(help_page);
    scr_flush();

    while (1)
    {
      key = get_keypress();
      action = key_to_action_browse(key);

      if (action == action_cancel)
      {
	return;
      }
      else if (action == action_next && help_page < HELP_PAGES - 1)
      {
	help_page++;
	break;
      }
      else if (action == action_previous && help_page > 0)
      {
	help_page--;
	break;
      }
    }
  }
} /* help */



void display_help_page(const unsigned int index)
{
  char line[1000];
  char label1[6];
  char label2[6];
  char label3[6];

/*  unsigned int y;
    unsigned int x;*/

  scr_move(2, 0);

  switch (index)
  {
    case HELP_KEYBOARD:
      key_reference();
      break;

    case HELP_LEGEND:
      display_legend();
      break;

    case HELP_MOVEMENT:
      key_help(label1, true, action_flip);
      key_help(label2, true, action_cancel);
      sprintf(line, "The right of the screen displays information about your character. Use %s to switch between different statistics. The bottom left relays messages what is going on. [MORE] means there is more to display than will fit on the screen, or an important message has paused the game. Press %s to continue. \n \n (%s will cancel actions and escape almost any menu.) \n \n ", label1, label2, label2);
      wrap_text(line);

      sprintf(line, "MOVE around the map with vi-keys (\"hjkl\") or numpad. \n \n Movement and ranged combat are closely integrated. Press shift+direction to perform a ranged attack. This defaults to firing your weapon, zapping a wand you are holding or throwing if you only have ammunition equipped. \n \n ");
      wrap_text(line);

      key_help(label1, true, action_fire);
      key_help(label2, true, action_throw);
      key_help(label3, true, action_use);
      sprintf(line, "You can also use %s explicitly to FIRE, %s to THROW and %s to zap wands (you can also throw wands, but that's probably not what you want). \n \n ", label1, label2, label3);
      wrap_text(line);
      
      key_help(label1, true, action_quiver);
      sprintf(line, "You can cycle through available ammo with %s (\"quiver\" command). \n \n ", label1);
      wrap_text(line);

      key_help(label1, true, action_wait);
      key_help(label2, true, action_rest);
      sprintf(line, "You can WAIT by pressing %s if you want to pass in-game time for any reason. Resting with %s will pass time until bad status effects expire or an enemy comes within view. \n \n ", label1, label2);
      wrap_text(line);

      break;


    case HELP_HAZARDS:
      sprintf(line, "TRAPS %c%d%c are activated when stepped upon. \n \n", INSERT_GENT, gent_trap_generic, INSERT_GENT);
      wrap_text(line);

      sprintf(line, "WEBS %c%d%c hold you stationary for a while. \n \n", INSERT_GENT, gent_web, INSERT_GENT);
      wrap_text(line);

      sprintf(line, "Moving through LAVA %c%d%c and WATER %c%d%c will damage you. The Swimming skill gives you a number of \"free\" turns in water before you take damage. Amphibians and creatures that do not breathe can remain in water indefinitely.\n \n", INSERT_GENT, gent_lava, INSERT_GENT, INSERT_GENT, gent_water, INSERT_GENT);
      wrap_text(line);

      sprintf(line, "FORCEFIELDS %c%d%c disintegrate most objects that pass through them. To disable a forcefield, you must disable (by hacking) or destroy (with explosives; small-arms fire is not enough) one of its GENERATORS %c%d%c. \n \n", INSERT_GENT, gent_forcefield, INSERT_GENT, INSERT_GENT, gent_generator, INSERT_GENT);
      wrap_text(line);

      sprintf(line, "TERMINALS %c%d%c can be hacked to retrieve information. Use a dataprobe %c%d%c or the \"Interface\" ability to connect. \n \n", INSERT_GENT, gent_terminal, INSERT_GENT, INSERT_GENT, gent_dataprobe, INSERT_GENT);
      wrap_text(line);

      break;



    case HELP_INVENTORY:
      key_help(label1, true, action_inventory);
      key_help(label2, true, action_flip);
      sprintf(line, "Press %s to bring up your INVENTORY. Use the directional keys to browse or an index letter to select a specific item. %s will bring up a submenu and let you perform common actions. \n \n ", label1, label2);
      wrap_text(line);

      key_help(label1, true, action_pickup);
      key_help(label2, true, action_drop);
      sprintf(line, "Press %s to PICK UP an item. Items on the ground are also indexed by letters. \n \n Your carrying capacity is limited. Being \"burdened\" means you are carrying too much and will slow you down. Press %s to DROP an item. \n \n", label1, label2);
      wrap_text(line);

      key_help(label1, true, action_label);
      sprintf(line, "Most items are unknown to you from the beginning and must be IDENTIFIED (or used with uncertain effects!). Some have \"random\" appearances; a \"bubbly potion\" will usually not have the same effect in two different games. \n \n Press %s to LABEL an item (you can use it to take notes).\n ", label1);
      wrap_text(line);

      /*
	strcpy(line,
	"] clothes or armor\n"
	"[ boots\n"
	"\" cloak\n"
	") weapon\n"
	"' tool\n"
	"; light source\n"
	"~ magical wand\n"
	"? scroll\n"
	"! potion\n"
	"{ ranged weapon\n"
	": ammunition\n"
	"% food, corpse");
      */	
      
      break;


    case HELP_EQUIPMENT:
      key_help(label1, true, action_equip);
      key_help(label2, true, action_remove);
      sprintf(line, "EQUIP an item with %s. \n \n REMOVE (take off) an item with %s. \n \n ", label1, label2);
      wrap_text(line);

      sprintf(line,
	      "MELEE WEAPONS %c%d%c can be wielded and will be used automatically when \"bumping\" into enemies. Without a melee weapon you fight barehanded. \n \n ",
	      INSERT_GENT, gent_axe, INSERT_GENT);
      wrap_text(line);

      sprintf(line,
	      "RANGED WEAPONS %c%d%c require compatible AMMUNITION %c%d%c (bows use arrows, crossbows use darts, shotguns use shells). Without a ranged weapon equipped, firing will resort to throwing your ammunition. Using a weapon improves your precision, range and damage. \n \n",
	      INSERT_GENT, gent_bow, INSERT_GENT,
	      INSERT_GENT, gent_arrow, INSERT_GENT);
      wrap_text(line);

      sprintf(line,
	      "CLOTHES and ARMOR %c%d%c reduce damage you receive but will eventually break. The first point of damage is always passed through. After that damage is absorbed up to the protection value of the armor (and deducted from its durability). RESISTANCES protect against specific types of damage. \n \n",
	      INSERT_GENT, gent_light_armor, INSERT_GENT);
      wrap_text(line);

      sprintf(line,
	      "BOOTS %c%d%c and CLOAKS %c%d%c provide resistances and other benefits. \n \n",
	      INSERT_GENT, gent_boots, INSERT_GENT,
	      INSERT_GENT, gent_cloak, INSERT_GENT);
      wrap_text(line);
      
/*      sprintf(line,
	      "You can equip one item of each type: "
	      " %c%d%c "
	      " clothes or armor "
	      " \n "
	      " %c%d%c "
	      " cloak "
	      " \n "
	      " %c%d%c "
	      " boots "
	      " \n "
	      " %c%d%c "
	      " melee weapon "
	      " \n "
	      " %c%d%c "
	      " ranged weapon "
	      " \n "
	      " %c%d%c "
	      " wand "
	      " \n "
	      " %c%d%c "
	      " ammunition "
	      " \n "
	      ,
	      INSERT_GENT, gent_whip, INSERT_GENT,
	      INSERT_GENT, gent_bow, INSERT_GENT,
	      INSERT_GENT, gent_wand, INSERT_GENT,
	      INSERT_GENT, gent_shell, INSERT_GENT
	);
	wrap_text(line);*/

      break;


    case HELP_ITEMS:
      sprintf(line, "LIGHT SOURCES %c%d%c extend your field of view. The dungeon is a very dark place so it might be wise to stock up on any means of illumination you find. \n A light source is lit when equipped and put out when removed. They have a limited duration and disappear from your inventory when spent. \n \n", INSERT_GENT, gent_lantern, INSERT_GENT);
      wrap_text(line);

      key_help(label1, true, action_drink);
      sprintf(line, "POTIONS %c%d%c can be drunk (quaffed). They have a variety of effects and can boost your abilites for a limited time. Some are harmful and can be utilised in an offensive manner (by throwing). Press %s to drink a potion. \n \n ", INSERT_GENT, gent_potion, INSERT_GENT, label1);
      wrap_text(line);

      key_help(label1, true, action_read);
      sprintf(line, "SCROLLS %c%d%c hold magical powers and are consumed upon activation. Some scrolls prompt for an item, others have more general (and sometimes subtle) effects. BOOKS %c%d%c can teach you abilities permanently. Press %s to read. \n \n ", INSERT_GENT, gent_scroll, INSERT_GENT, INSERT_GENT, gent_book, INSERT_GENT, label1);
      wrap_text(line);

      key_help(label1, true, action_use);
      sprintf(line, "WANDS %c%d%c hold multiple activations of some magical power. They must always be aimed in a direction. Wands can be equipped and used like a ranged weapon. \n \n ", INSERT_GENT, gent_wand, INSERT_GENT);
      wrap_text(line);

      key_help(label1, true, action_use);
      sprintf(line, "Press %s to zap a wand or apply a tool. \n ", label1);
      wrap_text(line);

/*      wrap_text("FOOD is available in many forms. You feel no hunger and will not starve, but eating will remove fatigue. You can sometimes eat what you kill. \n \n");*/
      break;



    case HELP_DEATH:
      wrap_text("DEATH is always close by. It is also permanent - when you die, the game is over. There is no way to bring a dead character back. \n \n");

      key_help(label1, true, action_quit);
      sprintf(line, "You can quit (forfeit) your game with %s. Giving up, huh? \n \n ", label1);
      wrap_text(line);

      key_help(label1, true, action_save);
      sprintf(line, "You can SAVE your game with %s and resume it at a later time. At start-up TSL will look for a previously saved game and load it. \n \n ", label1);
      wrap_text(line);

      wrap_text("Once you have loaded a saved game, the savefile will be deleted. While it is possible to make a backup copy, it is considered cheating and is strongly discouraged. \n \n");

      wrap_text("(The SAVEFILE will be called " SAVE_FILENAME " and located in your home directory. Savefiles can not be moved between different OS/architecture combinations, different versions of TSL, maybe not even between different builds of the same source code.) \n ");

      break;
    

    case HELP_CHARACTER:
      wrap_text("Your character is defined by a set of basic attributes. \n \n ");

      sprintf(line, "HEALTH represents your overall well-being and strictly how much damage you can take before you die. Getting shot, burned and maimed doesn't help. Collect medkits %c%d%c to gain health. There is no max cap for health. \n \n ", INSERT_GENT, gent_medkit, INSERT_GENT);
      wrap_text(line);

      wrap_text("ENERGY POINTS (EP) are required and spent when using skills or magical powers. They replenish over time. Just wait and see. \n \n ");

/*      wrap_text("MAGIC - how quickly you regain EP, willpower. \n ");*/

      wrap_text("SPEED is rather intuitive: it determines how often you get to move. If you're fast, you can inflict more pain than they do to you. Make them suffer. \n \n ");

      wrap_text("ATTACK and DODGE are your chances (in percent) to hit targets in melee and ranged combat, or evade monster attacks, projectiles and sprung traps. \n \n ");

      wrap_text("STEALTH lets you move unnoticed and unheard, while PERCEPTION is the acuity of your senses and helps you detect monsters, traps and sounds. VISION is how far you can see. \n \n ");

      break;



    case HELP_ADVANCEMENT:
      key_help(label1, true, action_status);
      sprintf(line, "AUGMENTATIONS are your primary means of self improvement. You can alter or replace parts of your biology with technology, improve your performance and gain abilities. Find and enter augmentation CAPSULES %c%d%c. Once upgrades have been installed they will stay with you for the rest of the game. %s will display what you have installed.\n \n ", INSERT_GENT, gent_capsule, INSERT_GENT, label1);
      wrap_text(line);

      wrap_text("FACETS work like augmentations but are acquired through achievements. \n \n ");

      /*key_help(label1, true, action_ability);*/
      key_help(label2, true, action_ability_config);
      sprintf(line, "ABILITIES are special actions you can perform. Most abilities consume energy. Abilities are automatically mapped to shortcut keys. To see what an ability does, or bind it to a shortcut, press %s. \n \n ", label2);
      wrap_text(line);

      /*A menu can be accessed with %s. */

      break;



    case HELP_STATUS:
      wrap_text("");
      break;


    case HELP_COMBAT:
      wrap_text("In the underworld you will run into other beings. Most will not be very friendly. You have numerous tactics at your disposal: engaging them in melee, using ranged or magical attacks, setting traps, poisoning. It is often best to avoid combat. \n \n ");

      wrap_text("Some weapons have additional effects that are applied when a successful attack is made. STUNNING will prevent a creature from moving. WOUNDED creatures will take damage for each step. POISON will make the victims life seep away over time (poison resistance slows the process). \n \n ");


      break;


    case HELP_BACKSTAB:
      wrap_text("Certain weapons (e.g. daggers) have a \"chance of backstab\" in percent. You can only backstab enemies unaware of your presence. Some enemies can not be backstabbed at all. Backstab rolls are automatically made when launching a preemptive strike; success results in instant and silent destruction of the target. \n \n");
      wrap_text("Some character facets or equipment provide bonuses to your backstab skill. Note that this is only a bonus; cumulative with an already backstab-enabled weapon, but it will not allow backstabbing with just any weapon. Weapons do not stack; when using two weapons with backstab chance the highest will be used.");
      break;
  }

  return;
} /* display_help_page */



void display_legend(void)
{
  unsigned int y;
  unsigned int x;
  unsigned int t;
  /*unsigned int l;*/
  unsigned int w;
  unsigned int mw;
  /*unsigned int h;*/
  unsigned int temp;
  unsigned int attr;
  gent_t gent;
  char line[1000];
  char map[400];
  char label_inspect[6];
  char label_close[6];
  char label_recenter[6];
  char label_stairs[6];
  blean_t console;

/*  t = 2;
  l = 0;
  w = 11;
  h = 21;*/

  scr_addstr("             \n");
  scr_addstr("             player (this is you)\n");
  scr_addstr("             a door (closed)\n");
  scr_addstr("             a trap, lava\n");
  scr_addstr("             a door (open)\n");
  scr_addstr("             a medkit\n");
  scr_addstr("             an augmentation capsule\n");
  scr_addstr("             stairs to another level\n");
  scr_addstr("             \n");
  scr_addstr("             \n");
  scr_addstr("             \n");
  scr_addstr("             \n");
  scr_addstr("             water\n");
  scr_addstr("             \n");
  scr_addstr("             \n");
  scr_addstr("             this is outside\n");
  scr_addstr("             your field of view\n");
  scr_addstr("             \n");
  scr_addstr("             an obstacle you have\n");
  scr_addstr("             felt but not yet seen\n");
  scr_addstr("             (detected while blinded)\n");

  #ifdef TSL_CONSOLE
  console = true;
  #else
  console = false;
  #endif
  
  if (console || options.glyph_mode)
  {
    strcpy(map,
	   "  1-------2"
	   "  |....@..|"
	   "  |.......X"
	   "  |..^....|"
	   "  |.......="
	   "  |.+.....|"
	   "  |.....0.|"
	   "  |...%...|"
	   "  |.......|"
	   "  |.......|"
	   "  |.......|"
	   "  |......_|"
	   "  |....___|"
	   "  |.....__|"
	   "  3--2....|"
	   "  1--4:..:|"
	   "  |:::::::|"
	   "--4:::##::#"
	   "::::::#####"
	   "  :::##    "
	   "   ###     ");
    w = 11;
    mw = 1;
    t = 2;
  }
  else
  {
    strcpy(map,
	   "1---2"
	   "|..@|"
	   "|...X"
	   "|^.L|"
	   "|...="
	   "|+..|"
	   "|..0|"
	   "|.%.|"
	   "|...|"
	   "|...|"
	   "|...|"
	   "|.._|"
	   "|.__|"
	   "|.._|"
	   "32..|"
	   "14:.|"
	   "|:::|"
	   "4::##"
	   ":::##"
	   "::## "
	   "###  ");
    w = 5;
    mw = 2;
    t = 2;
  }

  for (y = 0; y < 21; y++)
  {
    for (x = 0; x < w; x++)
    {
      temp = map[y * w + x];
      attr = MAP_NONE;

      switch (temp)
      {
	case '.': gent = gent_floor; break;
	case 'X': gent = gent_door_closed; break;
	case '=': gent = gent_door_open; break;
	case '|': gent = gent_wall_v; break;
	case '-': gent = gent_wall_h; break;
	case '+': gent = gent_medkit; break;
	case '0': gent = gent_capsule; break;
	case '1': gent = gent_wall_es; break;
	case '2': gent = gent_wall_sw; break;
	case '3': gent = gent_wall_ne; break;
	case '4': gent = gent_wall_nw; break;
	case '_': gent = gent_water; break;
	case 'L': gent = gent_lava; break;
	case '@': gent = gent_player; break;
	case '^': gent = gent_trap_generic; break;
	case '%': gent = gent_stairs; break;
	case '#': gent = gent_obstacle; attr = MAP_DIM; break;
	case ':': gent = gent_floor; attr = MAP_DIM; break;
	default: continue;
      }

      scr_move(y + t, x * mw);
      scr_addgent(gent, attr | MAP_FLOOR);
    }
  }

  key_help(label_inspect,   true, action_inspect);
  key_help(label_stairs,    true, action_stairs);
  key_help(label_close,     true, action_close);
  key_help(label_recenter,  true, action_recenter);
  sprintf(line, "The left half of the screen holds the level map. Map features such as walls, doors and traps will be memorized, but creatures and items are only displayed inside your field of view. \n \n Press %s to inspect a tile (describe what is there) or scroll the map view. \n \n DOORS are opened by \"bumping\" into them. Some doors require keys, or you can attempt to break them. Close a door with %s. \"Bump\" into walls to search for secret doors (once is enough). \n \n STAIRS will take you to another level. Press %s to climb. \n \n", label_inspect, label_close, label_stairs);
  constrained_wrap(2, 40, 40, line);

  return;
} /* display_legend */



void key_reference()
{
  int y;
  int x;

  int key;
  int action;

  char layout[100 * 25];

  layout[0] = '\0';

//strcat(layout, "********************************************************************************");
  strcat(layout, "                                   |                                            ");
  strcat(layout, "       $rt.. $cm.. $ln..           | $rb.. - cancel (or clear message bar)      ");
  strcat(layout, "            <  |  /                | $rc.. - confirm/select/do what I mean      ");
  strcat(layout, "      $rs.. -     - $lo..          | $ra.. - this help screen                   ");
  strcat(layout, "            /  |  <                | $rd.. - save and quit                      ");
  strcat(layout, "       $rr.. $cq.. $lp..           | $re.. - forfeit game                       ");
  strcat(layout, "                                   | $rf.. - recenter view                      ");
  strcat(layout, "$su..- wait (pass) one turn        | $rg.. - redraw the screen                  ");
  strcat(layout, "$sv..- traverse stairs             | $rh.. - message history                    ");
  strcat(layout, "$sx..- close adjacent door         | $ri.. - flip status display                ");
  strcat(layout, "                                   | $rj.. - character summary                  ");
  strcat(layout, "$sy..- browse inventory            | $rk.. - inspect a tile                     ");
  strcat(layout, "$sB..- use item                    | $rl.. - interact with environment          ");
  strcat(layout, "$sz..- pick up   $sA..- drop       | $rJ.. - change options                     ");
  strcat(layout, "$sC..- equip     $sD..- remove     | $rX.. - display version                    ");
  strcat(layout, "$sE..- drink     $sF..- eat        |                                            ");
  strcat(layout, "$sG..- label (for taking notes)    |  Fire in direction                         ");
  strcat(layout, "                                   |  $rT.. $cM.. $lN..   $sU..- fire missile   ");
  strcat(layout, "$sH..- use ability (spell, skill)  |       <  |  /        $sV..- throw an item  ");
  strcat(layout, "$sI..- assign ability shortcuts    | $rS.. -     - $lO..  $sW..- cycle ammo     ");
  strcat(layout, "       Ability shortcuts:          |       /  |  <               (free action)  ");
  strcat(layout, "       1 2 3 4 5 6 7 8 9 0         |  $rR.. $cQ.. $lP..                         ");
  strcat(layout, "                                   |                                            ");

  for (y = 0; y < 23; y++)
  {
    for (x = 0; x < 80; x++)
    {
      scr_move(y + 1, x);

      switch (layout[y * 80 + x])
      {
	case '|':
	  scr_special(ST_VLINE);
	  break;
	
	case '<':
	  scr_addch('\\');
	  break;
	
	default:
	  scr_addch(layout[y * 80 + x]);
	  break;
      }
    }  
  }

 for (y = 0; y < 23; y++)
  {
    for (x = 0; x < 80; x++)
    {
      if (layout[y * 80 + x] == '$')
      {
	key = layout[y * 80 + x + 2];

	switch (key)
	{
	  case 'a': action = action_help; break;
	  case 'b': action = action_cancel; break;
	  case 'c': action = action_select; break;
	  case 'd': action = action_save; break;
	  case 'e': action = action_quit; break;
	  case 'f': action = action_recenter; break;
	  case 'g': action = action_redraw; break;
	  case 'h': action = action_history; break;
	  case 'i': action = action_flip; break;
	  case 'j': action = action_status; break;
	  case 'k': action = action_inspect; break;
	  case 'l': action = action_interact; break;
	  case 'm': action = action_n; break;
	  case 'n': action = action_ne; break;
	  case 'o': action = action_e; break;
	  case 'p': action = action_se; break;
	  case 'q': action = action_s; break;
	  case 'r': action = action_sw; break;
	  case 's': action = action_w; break;
	  case 't': action = action_nw; break;
	  case 'u': action = action_wait; break;
	  case 'v': action = action_stairs; break;
	  case 'x': action = action_close; break;
	  case 'y': action = action_inventory; break;
	  case 'z': action = action_pickup; break;
	  case 'A': action = action_drop; break;
	  case 'B': action = action_use; break;
	  case 'C': action = action_equip; break;
	  case 'D': action = action_remove; break;
	  case 'E': action = action_drink; break;
	  case 'F': action = action_eat; break;
	  case 'G': action = action_label; break;
	  case 'H': action = action_ability; break;
	  case 'I': action = action_ability_config; break;
	  case 'J': action = action_options; break;
	  case 'M': action = action_fire_n; break;
	  case 'N': action = action_fire_ne; break;
	  case 'O': action = action_fire_e; break;
	  case 'P': action = action_fire_se; break;
	  case 'Q': action = action_fire_s; break;
	  case 'R': action = action_fire_sw; break;
	  case 'S': action = action_fire_w; break;
	  case 'T': action = action_fire_nw; break;
	  case 'U': action = action_fire; break;
	  case 'V': action = action_throw; break;
	  case 'W': action = action_quiver; break;
	  case 'X': action = action_version; break;
	  default: continue;
	}

	scr_move(y + 1, x);
	scr_addstr("     ");

	scr_move(y + 1, x);

	if (layout[y * 80 + x + 1] == 'c')
	  list_key(y + 1, x + 2, 1, action);
	else if (layout[y * 80 + x + 1] == 'r')
	  list_key(y + 1, x + 4, 2, action);
	else if (layout[y * 80 + x + 1] == 's')
	  list_key(y + 1, x + 4, 3, action);
	else
	  list_key(y + 1, x, 0, action);
      }
    }  
  }

  return;
} /* key_reference */


/*
 */
unsigned int list_key(const unsigned int y, unsigned int x,
		      unsigned int just, const unsigned int action)
{
  char label[6];
  unsigned int len;

  if (action >= ACTIONS)
    return 0;

  key_help(label, false, action);
  len = strlen(label);

  if (just == 1)
  {
    if (len >= 4)
      x -= 2;
    else if (len >= 2)
      x -= 1;
  }
  else if (just == 2)
  {
    x -= len - 1;
  }
  else if (just == 3)
  {
    x -= len;
  }

  scr_move(y, x);
  scr_addstr(label);

  return len;
} /* list_key */
