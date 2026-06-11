/*
  balls.h
*/

#ifndef _BALLS_H_
#define _BALLS_H_

blean_t fireball(creature_t * caster, item_t * source, signed int param);
blean_t mudball(creature_t * caster, item_t * source, signed int param);
blean_t force_bolt(creature_t * caster, item_t * source, signed int param);

blean_t ball_bolt(creature_t * caster, item_t * source, attr_index_t type, signed int param);

#endif
