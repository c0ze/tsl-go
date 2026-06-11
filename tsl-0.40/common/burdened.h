/*
  burdened.h
*/

#ifndef _BURDENED_H_
#define _BURDENED_H_

#include "main.h"
#include "creature.h"
#include "item.h"

void unburdened(const creature_t * creature, const unsigned int weight);
void stagger(const creature_t * creature, const unsigned int old_capacity);
unsigned int get_weight_allowance(const creature_t * creature);
unsigned int get_carried_weight(const creature_t * creature);
blean_t is_burdened(const creature_t * creature);

#endif
