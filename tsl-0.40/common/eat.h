/*
  eat.h
*/

#ifndef _EAT_H_
#define _EAT_H_

#include "main.h"
#include "item.h"

#define EAT_MSG sprintf(line, "You eat %s.", name); queue_msg(line);


blean_t eat(creature_t * creature, item_t * item);

#endif
