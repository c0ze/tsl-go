/*
  breath.c - breath weapons.
*/

#ifndef _BREATH_H_
#define _BREATH_H_

#include "main.h"
#include "creature.h"

blean_t noxious_breath(creature_t * caster, item_t * source, signed int param);
blean_t breathe_fire(creature_t * caster, item_t * source, signed int param);
blean_t breath_weapon(creature_t * caster, item_t * source, blean_t fire);

#endif
