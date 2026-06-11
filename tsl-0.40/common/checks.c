#include <stdlib.h>

#include "main.h"
#include "stuff.h"
#include "creature.h"
#include "effect.h"
#include "game.h"

/*
  See creature.h for prototypes.
*/

/*
  Returns true if B is an enemy of A, otherwise false. This does not
  need to be symmetrical, e.g. a charmed monster might not be hostile
  towards the player, but the player would still consider it a threat.
*/
blean_t enemies(const creature_t * a, const creature_t * b)
{
  if ((a == NULL) || (b == NULL))
    return false;

  if (a->id == monster_slime && b->id == monster_slime)
    return false;

  if (is_blinded(a))
  {
    if (is_player(b) == false)
      return true;
  }
  else if (attr_current(a, attr_player_ally) ^ attr_current(b, attr_player_ally))
  {
    return true;
  }

  return false;
} /* enemies */



/*
  Returns true if CREATURE is unable to defend itself.
*/
blean_t is_helpless(const creature_t * creature)
{
  if (creature == NULL)
    return false;

  if (is_player(creature) == true)
    return false;
  
  if (is_awake(creature) == false ||
      is_stunned(creature) ||
      get_effect_by_id(creature, effect_web) ||
      is_blinded(creature))
  {
    return true;
  }

  return false;
} /* is_helpless */



/*
  Returns true if CREATURE is of a humanoid shape.
*/
blean_t is_humanoid(const creature_t * creature)
{
  if (creature == NULL)
    return false;

  return creature->humanoid;
} /* is_humanoid */



/*
  Returns true if CREATURE is a unique creature (i.e. only one will
  ever be spawned during a whole game).
*/
blean_t is_unique(const creature_t * creature)
{
  if (creature == NULL)
    return false;

  if (creature->id > unique_first && creature->id < unique_last)
    return true;
  else
    return false;
} /* is_unique */



/*
  Returns true if CREATURE is under player control.
*/
blean_t is_player(const creature_t * creature)
{
  if (creature == NULL)
    return false;

  if (creature == game->player)
    return true;

  return false;
} /* is_player */



/*
  Returns TRUE if CREATURE is blinded for any reason, otherwise false.
*/
blean_t is_blinded(const creature_t * creature)
{
  if (creature == NULL)
    return false;

  if (attr_current(creature, attr_blindness) > 0)
    return true;
  else
    return false;
} /* is_blinded */



/*
  Returns TRUE if CREATURE is stunned.
*/
blean_t is_stunned(const creature_t * creature)
{
  if (creature == NULL)
    return false;

  if (get_effect_by_id(creature, effect_stun))
    return true;

  return false;
} /* is_stunned */



/*
  Returns TRUE if CREATURE is awake or false if it is asleep.
*/
blean_t is_awake(const creature_t * creature)
{
  if (creature == NULL)
    return false;

  if (get_effect_by_id(creature, effect_sleep))
    return false;

  return true;
} /* is_awake */



/*
  Returns TRUE if CREATURE is confused.
*/
blean_t is_confused(const creature_t * creature)
{
  if (creature == NULL)
    return false;

  if (get_effect_by_id(creature, effect_confusion))
    return true;

  return false;
} /* is_confused */



/*
  Returns true if CREATURE is a mimic that hasn't been revealed.
*/
blean_t is_mimic(const creature_t * creature)
{
  if (creature == NULL)
    return false;

  if (creature->ai_state == ai_mimic)
    return true;

  return false;
} /* is_mimic */
