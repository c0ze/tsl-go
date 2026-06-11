/*
  identify.h

  Item identification. This is implemented in two layers:

  a) a bitmask for each item that specifies what properties are known
  about it.

  b) item_known[] in game_t, that globally identifies known_name for
  all items with a certain id.
*/

#ifndef _IDENTIFY_H_
#define _IDENTIFY_H_

#include "main.h"

enum id_status_t
{
  known_none = 0,
  known_name = 2,
  known_charges = 8,
  known_all = 255
};
typedef enum id_status_t id_status_t;

id_status_t identified(const item_t * item);
void id_if_not_known(item_t * item);
void identify(item_t * item);
void make_number_known(const unsigned int number);
void make_item_known(const item_t * item);
void auto_id(creature_t * owner, item_t * item);
void try_to_id_weapon(creature_t * owner, item_t * weapon);

#endif
