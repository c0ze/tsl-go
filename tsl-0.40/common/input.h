#ifndef _INPUT_H_
#define _INPUT_H_

#include "main.h"
#include "item.h"

enum key_token_t
{
  kt_escape = 600,
  kt_backspace,
  kt_delete,
  kt_page_up,
  kt_page_down,

  kt_dir_up,
  kt_dir_right,
  kt_dir_left,
  kt_dir_down,
  kt_shift_dir_up,
  kt_shift_dir_right,
  kt_shift_dir_left,
  kt_shift_dir_down,

  kt_f1,

  kt_np0,
  kt_np1,
  kt_np2,
  kt_np3,
  kt_np4,
  kt_np5,
  kt_np6,
  kt_np7,
  kt_np8,
  kt_np9,

  kt_shift_np0,
  kt_shift_np1,
  kt_shift_np2,
  kt_shift_np3,
  kt_shift_np4,
  kt_shift_np5,
  kt_shift_np6,
  kt_shift_np7,
  kt_shift_np8,
  kt_shift_np9,

  kt_joy0,
  kt_joy1,
  kt_joy2,
  kt_joy3,
  kt_joy4,
  kt_joy5,
  kt_joy6,
  kt_joy7,
  kt_joy8,
  kt_joy9,

  kt_joy_n,
  kt_joy_ne,
  kt_joy_e,
  kt_joy_se,
  kt_joy_s,
  kt_joy_sw,
  kt_joy_w,
  kt_joy_nw,

  kt_max
};
typedef enum key_token_t key_token_t;

enum action_t
{
  action_undefined,
  action_quit,
  action_n,
  action_s,
  action_nw,
  action_sw,
  action_e,
  action_w,
  action_ne,
  action_se,
  action_fire_n,
  action_fire_s,
  action_fire_nw,
  action_fire_sw,
  action_fire_e,
  action_fire_w,
  action_fire_ne,
  action_fire_se,
  action_rest,
  action_inventory,
  action_inspect,
  action_version,
  action_cancel,
  action_pickup,
  action_equip,
  action_remove,
  action_drop,
  action_debug,
  action_wait,
  action_select,
  action_stairs,
  action_close,
  action_redraw,
  action_fire,
  action_throw,
  action_ability,
  action_ability_1,
  action_ability_2,
  action_ability_3,
  action_ability_4,
  action_ability_5,
  action_ability_6,
  action_ability_7,
  action_ability_8,
  action_ability_9,
  action_ability_10,
  action_ability_config,
  action_interact,
  action_next,
  action_previous,
  action_drink,
  action_read,
  action_use,
  action_filter,
  action_history,
  action_quiver,
  action_status,
  action_label,
  action_recenter,
  action_eat,
  action_flip,
  action_help,
  action_save,
  action_options,
  action_apply
};
typedef enum action_t action_t;

#define ACTIONS 90

#define get_action() key_to_action(get_keypress())
action_t key_to_action(const int key);
action_t key_to_action_browse(const key_token_t input);
action_t string_to_action(const char * name);
signed int action_to_ability(const action_t action);

signed int read_number(const char * prompt);
void read_string(const char * msg, char * target, const unsigned int maxlen);

void wait_continue(void);

blean_t get_position_on_map(const level_t * level,
			    unsigned int * y,
			    unsigned int * x);
dir_t get_direction(void);

void key_to_label(char * dest, const blean_t apostrophes, const unsigned int key);
void key_help(char * dest, const blean_t apostrophes, const action_t action);

#endif
