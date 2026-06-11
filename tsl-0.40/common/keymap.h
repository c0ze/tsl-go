/*
 */

#ifndef _KEYMAP_H_
#define _KEYMAP_H_

#include "input.h"

void bind_key(const action_t action, const int key);
void unbind_key(const int key);
void clear_keymap(void);

void common_keys(void);
void default_keymap(void);
void dvorak_keymap(void);

#define MAX_BINDINGS 5

key_token_t keymap[ACTIONS][5];

#endif
