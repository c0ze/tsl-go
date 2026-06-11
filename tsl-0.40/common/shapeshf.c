#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <ctype.h>

#include "main.h"
#include "stuff.h"
#include "shapeshf.h"
#include "monster.h"
#include "player.h"
#include "inventory.h"
#include "ui.h"
#include "actions.h"
#include "fov.h"
#include "game.h"
#include "altitude.h"
#include "burdened.h"
#include "effect.h"
#include "ability.h"



/*
  Shapeshifts CREATURE into a different shape.
*/
blean_t shapeshift_random(creature_t * creature, item_t * source, signed int param)
{
  unsigned int new_shape;

  new_shape = tslrnd() % monster_max;
  
  if (new_shape == creature->id)
    new_shape = (new_shape + 1) % monster_max;

  shapeshift_anyone(creature, new_shape);

  return true;
} /* shapeshift_random */



/*
  Shapeshifts CREATURE into a wolf.
 */
blean_t shapeshift_wolf(creature_t * creature, item_t * source, signed int param)
{
  shapeshift_anyone(creature, monster_moon_wolf);

  return true;
} /* shapeshift_wolf */



/*
  Returns the player to its native shape. CREATURE must be the player.
*/
blean_t shapeshift_native(creature_t * creature, item_t * source, signed int param)
{
  unsigned int old_carry;
  unsigned int new_carry;
  effect_t * eff;

  if (game->player == NULL ||
      game->native == game->player)
  {
    return false;
  }

  if (attr_current(game->player, attr_unchanging) > 0)
    return true;

  old_carry = attr_current(game->player, attr_carrying_capacity);

  attach_creature(get_current_level(), game->native);

  set_creature_coordinates(game->native, game->player->y, game->player->x);

  /* See shapeshift_anyone() for an explanation on this. */
  /* No actually, don't. */
  set_attr(game->native, attr_health, attr_base(game->player, attr_health));

  /* Native inherits effect list from the shapeshifted creature */
  game->native->first_effect = game->player->first_effect;
  game->player->first_effect = NULL;

  for (eff = game->native->first_effect; eff != NULL; eff = eff->next_effect)
    eff->affecting = game->native;

  switch_inventory(game->player, game->native);

  del_creature(game->player);
  game->player = game->native;
  queue_msg("You return to your native shape.");

  /* Does the native shape have a different carrying capacity than the old? */
  new_carry = attr_current(game->native, attr_carrying_capacity);

  if (new_carry > old_carry)
    unburdened(game->player, new_carry - old_carry);
  else
    stagger(game->player, old_carry);
  
  change_altitude(game->player);
  build_fov(game->player);

  unmapped_abilities(NULL);

  return true;
} /* shapeshift_native */



/*
  Returns false if the player is in its native form, true if it has
  shapeshifted into something else.
*/
blean_t player_shapeshifted()
{
  if (game->player != game->native)
    return true;
  else
    return false;
} /* player_shapeshifted */



/*
  Shapeshifts CREATURE into a NEW_SHAPE (see main.h).
*/
creature_t * shapeshift_anyone(creature_t * creature,
			       const monster_t new_shape)
{
  char line[80];
  creature_t * new_creature;
  unsigned int old_carry;
  unsigned int new_carry;
  blean_t player;
  effect_t * eff;
  
  if (creature == NULL)
    return NULL;
  
  if (creature == game->player)
    player = true;
  else
    player = false;

  if (attr_current(creature, attr_unchanging) > 0)
    return creature;

  old_carry = attr_current(creature, attr_carrying_capacity);

  /*
    Build a new creature of the specified type and replace the current
    creature with it.
  */
  new_creature = build_monster(new_shape);

  if (new_creature == NULL)
    return creature;

  if (attach_creature(creature->location, new_creature) != NULL)
  {
    del_creature(new_creature);
    return creature;
  }

  set_creature_coordinates(new_creature, creature->y, creature->x);
  detach_creature(creature);
  
  /*
    We've replaced the creature with a new creature - now proceed with
    copying necessary values from the old structure to the new.
  */

  new_creature->detected = creature->detected;
/*  new_creature->alignment = creature->alignment;*/
  new_creature->ai_state = creature->ai_state;

  if (player)
  {
    /* Make sure we can control this new creature */
    new_creature->lifetime = SHAPESHIFT_DURATION;
/*    new_creature->turn_func = &player_control;*/
  }
  else
  {
    new_creature->lifetime = creature->lifetime;
/*    new_creature->turn_func = creature->turn_func;*/
  }

  /*
    New creature inherits values from old creature.
  */
  set_attr(new_creature, attr_health, attr_base(creature, attr_health));

  /*
    This must be preserved, or monsters won't attack a shapeshifted
    player and other undesired things.
  */
  set_attr(new_creature, attr_player_ally, attr_base(creature, attr_player_ally));

  /* New creature inherits effect list from the old creature */
  new_creature->first_effect = creature->first_effect;
  creature->first_effect = NULL;
  
  for (eff = new_creature->first_effect; eff != NULL; eff = eff->next_effect)
    eff->affecting = new_creature;

  switch_inventory(creature, new_creature);

  if (player)
  {
    sprintf(line, "You transform into %s!", new_creature->name_one);
    queue_msg(line);
  }
  else if (can_see_creature(game->player, new_creature))
  {
    sprintf(line, "transforms into %s!", new_creature->name_one);
    msg_one(creature, line);
  }

  /* Can we hold an inventory? */
  if (new_creature->first_item != NULL &&
      attr_base(new_creature, attr_carrying_capacity) == 0)
  {
    if (player)
      queue_msg("You drop all your possessions!");
    else if (can_see_creature(game->player, new_creature))
      msg_one(new_creature, "drops all its possessions!");
    
    while (new_creature->first_item != NULL)
      drop_item(new_creature, new_creature->first_item, true);
  }

  /* Does the new shape have a different carrying capacity than the old? */
  new_carry = attr_current(new_creature, attr_carrying_capacity);

  if (new_carry > old_carry)
    unburdened(new_creature, new_carry - old_carry);
  else
    stagger(new_creature, old_carry);
  
  change_altitude(new_creature);
  build_fov(new_creature);

  if (player)
  {
    /*
      If the current player creature was *not* the native form, the
      player is already shapeshifted. We must remove the old
      shapeshifted creature.
    */
    
    if (game->native != game->player)
    {
      del_creature(game->player);
    }
    
    game->player = new_creature;

    draw_level();
  }
  else
  {
    /* Monster shapeshift is permanent, just remove the old creature. */
    del_creature(creature);
  }

  if (is_player(new_creature))
    unmapped_abilities(NULL);
  
  return new_creature;
} /* shapeshift_anyone */
