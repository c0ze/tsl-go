/*
  magic.c - spells and other powers.

  There are different spell types:
  - Touch: affects one target adjacent to caster
  - Bolt: missile, affects one target
  - Ball: missile, affects a 3x3 area around a target
  - Breath: generates a cone emanating from caster
  - Ray: affects multiple targets along a line
  - Item: spells that affect items

  See also: breath.h, poison.h, explode.h
*/

/* TODO: magic.c is way too big, it has to be split up. */

#ifndef _MAGIC_H_
#define _MAGIC_H_

#include "main.h"
#include "creature.h"

/* Divination */
blean_t magic_mapping(creature_t * caster, item_t * source, signed int param);
blean_t cast_identify(creature_t * caster, item_t * source, signed int param);
blean_t reveal_traps(creature_t * caster, item_t * source, signed int param);

/* Offensive */
blean_t frost_ray(creature_t * caster, item_t * source, signed int param);
blean_t bone_crush(creature_t * caster, item_t * source, signed int param);
blean_t aimed_shot_prepare(creature_t * creature, item_t * source, signed int param);
blean_t aimed_shot_release(creature_t * creature, item_t * source, signed int param);
blean_t shock(creature_t * creature, item_t * source, signed int param);
blean_t deathspell(creature_t * creature, item_t * source, signed int param);

/* Items */
blean_t cast_recharge(creature_t * caster, item_t * source, signed int param);

/* Misc */
blean_t flash_spell(creature_t * caster, item_t * source, signed int param);
blean_t first_aid(creature_t * creature, item_t * source, signed int param);
blean_t enslave(creature_t * caster, item_t * source, signed int param);
blean_t summon_familiar(creature_t * caster, item_t * source, signed int param);
blean_t magic_weapon(creature_t * caster, item_t * source, signed int param);
blean_t destroy_trap(creature_t * caster, item_t * source, signed int param);
blean_t raise_dead(creature_t * caster, item_t * source, signed int param);
blean_t amnesia(creature_t * caster, item_t * source, signed int param);
blean_t wish(creature_t * caster, item_t * source, signed int param);
blean_t leap(creature_t * creature, item_t * source, signed int param);
blean_t dash(creature_t * creature, item_t * source, signed int param);
blean_t levitate(creature_t * creature);
void charm(creature_t * victim);
blean_t dissolve_items(creature_t * creature, item_t * source, signed int param);
blean_t datajack(creature_t * creature, item_t * source, signed int param);
blean_t hide(creature_t * creature, item_t * source, signed int param);

#endif
