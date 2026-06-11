/*
  facet.h
*/

#ifndef _FACET_H_
#define _FACET_H_

#include "main.h"
#include "creature.h"

enum facet_t
{
  aug_strength,
  aug_arms,
  aug_stealth,
  aug_swimming,
  aug_poison_res,
  aug_vision,
  aug_skin,
  aug_claws,
  aug_datajack,
  aug_phase,
  aug_energy,
  aug_dualwield,
  facet_none,
  facet_dragon,
  facet_necro,
  facet_spellbook,
  aug_generic
};
typedef enum facet_t facet_t;

#define FACET_SLOTS 30

facet_t pop_facet(game_t * the_game);
void randomize_facets(game_t * the_game);
item_t * define_facet(const facet_t facet_index);
void apply_facet(item_t * facet);
void list_attributes(const item_t * facet);

void capsule(creature_t * creature, const unsigned int capsule_y, const unsigned int capsule_x);

#endif
