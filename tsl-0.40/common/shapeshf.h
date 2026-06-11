/*
  shapeshf.h

  Things related to shapeshifting (aka polymorphing, etc).

  We accomplish player shapeshifting by repointing the global player
  variable to another, temporary, creature. The "real" player creature
  is saved in player_ext->native, to be recalled once we shift back
  with shapeshift_native(). We always replace the _current_ (not the
  native) player creature, so if we have already shapeshifted into
  something we can shapeshift safely again.
*/

#ifndef _SHAPESHF_H_
#define _SHAPESHF_H_

#include "main.h"
#include "magic.h"

blean_t shapeshift_random(creature_t * creature, item_t * source, signed int param);
blean_t shapeshift_wolf(creature_t * creature, item_t * source, signed int param);
blean_t shapeshift_native(creature_t * creature, item_t * source, signed int param);
creature_t * shapeshift_anyone(creature_t * creature,
			       const monster_t new_shape);
blean_t player_shapeshifted(void);

#endif
