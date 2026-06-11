#include <stdlib.h>
#include <stdio.h>

#include "main.h"
#include "stuff.h"
#include "backstab.h"
#include "message.h"
#include "inventory.h"
#include "actions.h"
#include "equip.h"



/*
  Tries to make ATTACKER backstab TARGET. Returns TRUE if the backstab
  succeeded, otherwise FALSE. Even if the roll succeeded there is no
  guarantee the target was killed, it can also have been a mimic that
  was merely revealed.
*/
blean_t backstab(creature_t * attacker, creature_t * target)
{
  unsigned int chance;
  unsigned int weapon;
  unsigned int bonus;
  char line[100];

  if ((attacker == NULL) || (target == NULL))
    return false;
  
  /* Only the player can backstab. */
  if (is_player(attacker) == false)
    return false;

  /* The player cannot be backstabbed, that's just way too unfair. */
  if (is_player(target))
    return false;

  if (reveal_mimic(target))
    return true;

  /* Some other creatures just cannot be backstabbed */
  if (attr_current(target, attr_backstab_immunity) > 0)
    return false;

  /* Blind players cannot backstab. */
  if (is_blinded(attacker))
    return false;

  /* Creatures that have seen the player cannot be backstabbed. */
  if (is_helpless(target) == false)
    return false;
/*  if (target->ai_state == ai_offensive)
    return false;*/

  bonus = attr_current(attacker, attr_backstab_bonus);
  weapon = get_backstab_weapon(attacker);
  chance = weapon + bonus;

  /* We can only backstab with backstab-enabled weapons. */
  if (weapon == 0)
    return false;

  if (tslrnd() % 100 >= chance)
    return false; /* Roll failed! */

  /* Roll succeeded. */
  sprintf(line, "You stab %s in its back!", target->name_the);
  queue_msg(line);
  
  /*
    We use this rather than creature_death() since we
    print our custom kill message above.
  */
  kill_creature(target, true);
  
  return true;
} /* backstab */



/*
  Returns the best chance of backstabbing percentage of the weapons
  CREATURE has wielded. Could be 0%.
*/
unsigned int get_backstab_weapon(creature_t * creature)
{
  unsigned int best;
  unsigned i;
  unsigned t;
  item_t * item;

  if (creature == NULL)
    return 0;

  best = 0;

  i = count_item_type(creature, item_type_m_weapon, true);

  while (i > 0)
  {
    i--;

    item = get_equipped(creature, item_type_m_weapon, i);

    t = get_item_mod(item, attr_i_backstab);

    if (t > best)
      best = t;
  }

  return best;
} /* get_backstab_weapon */
