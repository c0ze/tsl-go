/*
  reading.h
*/

#ifndef _READING_H_
#define _READING_H_

#include "main.h"

blean_t cannot_read(const creature_t * creature);
blean_t read_scroll(creature_t * creature, item_t * item);
blean_t read_book(creature_t * creature, item_t * item);
void bad_book(creature_t * reader, item_t * book);

#endif
