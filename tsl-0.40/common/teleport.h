/*
  teleport.h
*/

#ifndef _TELEPORT_H_
#define _TELEPORT_H_

#include "main.h"
#include "actions.h"
#include "item.h"
#include "level.h"
#include "creature.h"

blean_t cast_teleport(creature_t * caster, item_t * source, signed int param);
blean_t teleport_creature(creature_t * caster, const unsigned int target_y, const unsigned int target_x);
blean_t cast_mark(creature_t * caster, item_t * source, signed int param);
blean_t cast_recall(creature_t * caster, item_t * source, signed int param);
blean_t cast_blink(creature_t * target, item_t * source, signed int param);
blean_t phase(creature_t * caster, item_t * source, signed int param);

blean_t unsafe_teleport(const gent_t gent);

#endif
