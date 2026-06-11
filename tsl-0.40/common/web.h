/*
  web.h
 */

#ifndef _WEB_H_
#define _WEB_H_

void web(creature_t * target, const blean_t message);
blean_t sticky_web(creature_t * caster, item_t * source, signed int param);
void struggle_web(creature_t * creature);

#endif
