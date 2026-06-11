#include "keymap.h"


/*
  Maps a key to a game action.
*/
void bind_key(const action_t action, const int key)
{
  unsigned int b;

  if (action >= ACTIONS || action < 0)
    return;

  unbind_key(key);

  b = 0;

  do
  {
    if (keymap[action][b] == 0)
    {
      keymap[action][b] = key;
      return;
    }
  }
  while (++b < MAX_BINDINGS);

  return;
} /* assign_key */



void unbind_key(const int key)
{
  unsigned int i;
  unsigned int j;

  for (i = 0; i < ACTIONS; i++)
  {
    for (j = 0; j < MAX_BINDINGS; j++)
    {
      if (keymap[i][j] == key)
	keymap[i][j] = 0;
    }
  }

  return;
} /* unbind_key */



/*
  Removes all key bindings.
 */
void clear_keymap()
{
  unsigned int i;
  unsigned int j;

  for (i = 0; i < ACTIONS; i++)
  {
    for (j = 0; j < MAX_BINDINGS; j++)
    {
      keymap[i][j] = 0;
    }
  }

  return;
} /* clear_keymap */




/*
  Sets up keybindings that should be common between QWERTY and
  Dvorak. It's pretty much anything that's not a letter.
*/
void common_keys()
{
  bind_key(action_n,          kt_dir_up);
  bind_key(action_s,          kt_dir_down);
  bind_key(action_e,          kt_dir_right);
  bind_key(action_w,          kt_dir_left);

  bind_key(action_n,          kt_np8);
  bind_key(action_s,          kt_np2);
  bind_key(action_e,          kt_np6);
  bind_key(action_w,          kt_np4);
  bind_key(action_nw,         kt_np7);
  bind_key(action_sw,         kt_np1);
  bind_key(action_ne,         kt_np9);
  bind_key(action_se,         kt_np3);

  bind_key(action_n,          kt_joy_n);
  bind_key(action_ne,         kt_joy_ne);
  bind_key(action_e,          kt_joy_e);
  bind_key(action_se,         kt_joy_se);
  bind_key(action_s,          kt_joy_s);
  bind_key(action_sw,         kt_joy_sw);
  bind_key(action_w,          kt_joy_w);
  bind_key(action_nw,         kt_joy_nw);

  bind_key(action_fire_n,     kt_shift_np8);
  bind_key(action_fire_s,     kt_shift_np2);
  bind_key(action_fire_e,     kt_shift_np6);
  bind_key(action_fire_w,     kt_shift_np4);
  bind_key(action_fire_nw,    kt_shift_np7);
  bind_key(action_fire_sw,    kt_shift_np1);
  bind_key(action_fire_ne,    kt_shift_np9);
  bind_key(action_fire_se,    kt_shift_np3);

  bind_key(action_fire_n,     kt_shift_dir_up);
  bind_key(action_fire_s,     kt_shift_dir_down);
  bind_key(action_fire_e,     kt_shift_dir_right);
  bind_key(action_fire_w,     kt_shift_dir_left);

  bind_key(action_select,     '\n');
  bind_key(action_select,     kt_np5);
  bind_key(action_select,     kt_joy0);

  bind_key(action_cancel,     ' ');
  bind_key(action_cancel,     kt_escape);
  bind_key(action_cancel,     kt_np0);
  bind_key(action_cancel,     kt_joy1);

  bind_key(action_flip,       '\t');
  bind_key(action_flip,       kt_joy2);

  bind_key(action_next,       '+');
  bind_key(action_previous,   '-');
  bind_key(action_next,       kt_page_down);
  bind_key(action_previous,   kt_page_up);

  bind_key(action_stairs,     '>');
  bind_key(action_stairs,     '<');

  bind_key(action_help,       '?');
  bind_key(action_help,       kt_f1);

  bind_key(action_options,    '=');

  bind_key(action_filter,     '*');
  bind_key(action_status,     '@');
  bind_key(action_pickup,     ',');
  bind_key(action_wait,       '.');
  bind_key(action_inspect,    '/');

  bind_key(action_ability_1,  '1');
  bind_key(action_ability_2,  '2');
  bind_key(action_ability_3,  '3');
  bind_key(action_ability_4,  '4');
  bind_key(action_ability_5,  '5');
  bind_key(action_ability_6,  '6');
  bind_key(action_ability_7,  '7');
  bind_key(action_ability_8,  '8');
  bind_key(action_ability_9,  '9');
  bind_key(action_ability_10, '0');

  return;
} /* common_keys */


/*
  Sets up a default keymap.
*/
void default_keymap()
{
  clear_keymap();

  bind_key(action_quit,       'Q');
  bind_key(action_version,    'V');
  bind_key(action_save,       'S');
  bind_key(action_redraw,     'X');
  bind_key(action_recenter,   'C');

  bind_key(action_stairs,     'c');
  bind_key(action_pickup,     'g');
  bind_key(action_inspect,    'I');

  bind_key(action_inspect,    'v');

  bind_key(action_rest,       'T');
  bind_key(action_inventory,  'i');
  bind_key(action_drop,       'd');
  bind_key(action_equip,      'e');
  bind_key(action_remove,     'r');
  bind_key(action_read,       'R');
  bind_key(action_drink,      'D');
  bind_key(action_use,        'a');
  bind_key(action_eat,        'E');
  bind_key(action_label,      'A');
  bind_key(action_apply,      'p');

  bind_key(action_ability,    'Z');
  bind_key(action_ability_config,'z');

  bind_key(action_fire,       'f');
  bind_key(action_quiver,     'W');
  bind_key(action_throw,      't');

  bind_key(action_n,          'k');
  bind_key(action_s,          'j');
  bind_key(action_e,          'l');
  bind_key(action_w,          'h');
  bind_key(action_nw,         'y');
  bind_key(action_sw,         'b');
  bind_key(action_ne,         'u');
  bind_key(action_se,         'n');

  bind_key(action_fire_n,     'K');
  bind_key(action_fire_s,     'J');
  bind_key(action_fire_e,     'L');
  bind_key(action_fire_w,     'H');
  bind_key(action_fire_nw,    'Y');
  bind_key(action_fire_sw,    'B');
  bind_key(action_fire_ne,    'U');
  bind_key(action_fire_se,    'N');

  bind_key(action_close,      'O');

  bind_key(action_history,    'P');

  common_keys();

  return;
} /* default_keymap */



/*
  Sets up a Dvorak-friendly keymap.
*/
void dvorak_keymap()
{
  clear_keymap();

  bind_key(action_quit,       'Q');
  bind_key(action_version,    'V');
  bind_key(action_save,       'S');
  bind_key(action_redraw,     'U');

  bind_key(action_rest,       'R');

  bind_key(action_inventory,  'i');
  bind_key(action_equip,      'e');
  bind_key(action_remove,     'r');
  bind_key(action_drop,       'o');
  bind_key(action_throw,      'j');
  bind_key(action_use,        'u');
  bind_key(action_read,       'a');
  bind_key(action_drink,      'O');
  bind_key(action_eat,        'E');
  bind_key(action_label,      'A');
  bind_key(action_apply,      'p');

  bind_key(action_ability,    'Z');
  bind_key(action_ability_config,'z');

  bind_key(action_fire,       'k');
  bind_key(action_quiver,     'B');

  bind_key(action_n,          't');
  bind_key(action_s,          'h');
  bind_key(action_e,          'n');
  bind_key(action_w,          'd');
  bind_key(action_nw,         'g');
  bind_key(action_sw,         'm');
  bind_key(action_ne,         'c');
  bind_key(action_se,         'w');

  bind_key(action_fire_n,     'T');
  bind_key(action_fire_s,     'H');
  bind_key(action_fire_e,     'N');
  bind_key(action_fire_w,     'D');
  bind_key(action_fire_nw,    'G');
  bind_key(action_fire_sw,    'M');
  bind_key(action_fire_ne,    'C');
  bind_key(action_fire_se,    'W');

  bind_key(action_interact,   'I');
  bind_key(action_close,      'l');

  bind_key(action_history,    'P');

  bind_key(action_recenter,   'L');

  common_keys();

  return;
} /* dvorak_keymap */
