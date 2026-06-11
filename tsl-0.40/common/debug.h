/*
  debug.h - some ugly hacks, mostly for testing and manipulating
  internal game states, or cheating.
*/
/* TODO: Not #defining TSL_DEBUG at compile time should leave the
 * entire debug system out. */

#ifndef _DEBUG_H_
#define _DEBUG_H_

#include "main.h"

void display_memory_usage(void);
void display_map_test_data(void);
void item_gen_test(void);
void debug_menu(void);
void perform_map_test(void);
void reveal_level(level_t * level);
void inspect_creature(void);
void kill_all_creatures(level_t * level);
void mislead_enemies(void);

void cloud_test(void);
void path_test(void);

void summary_test(void);

void undetect_everyone(void);

void module_browser(void);

void world_jump(void);

void set_attribute(void);
void set_alignment(void);
void print_memory_usage(void);
void display_raw_map(void);

char * generate_map_test_summary(void);
char * generate_memory_usage_summary(void);

void print_modules(void);

#endif
