#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include "main.h"
#include "stuff.h"
#include "game.h"
#include "ai.h"
#include "combat.h"
#include "level.h"
#include "actions.h"
#include "inventory.h"
#include "magic.h"
#include "ui.h"
#include "fov.h"
#include "creature.h"
#include "altitude.h"
#include "stacks.h"
#include "ability.h"
#include "missile.h"
#include "effect.h"
#include "player.h"
#include "area.h"
#include "sleep.h"
#include "doors.h"
#include "find.h"
#include "equip.h"
#include "elements.h"
#include "web.h"



/*
  The "basic AI" uses what I believe is called a finite state
  machine. It has a number of states (see ai_status_t in creature.h)
  and certain conditions which will cause it to move between different
  states.
*/
void basic_ai(creature_t * creature)
{
/*  char line[80];*/
  signed int move_y;
  signed int move_x;
  blean_t player_ally;
/*  unsigned int stealth;*/
  unsigned int perception;
  unsigned int i;
  blean_t found;

  if (creature == NULL ||
      creature->location == NULL)
  {
    return;
  }

  if (enemies(creature, game->player))
    player_ally = false;
  else
    player_ally = true;

  /*
    Tentacles belong to the Lurker. If it isn't around anymore,
    tentacles should die too.
  */
  if (creature->id == monster_tentacle)
  {
    found = false;
    
    for (i = 0; i < creature->location->creatures; i++)
    {
      if (creature->location->creature[i] != NULL &&
	  creature->location->creature[i]->id == unique_lurker)
      {
	found = true;
      }
    }

    if (found == false && maybe())
    {
      if (can_see_creature(game->player, creature))
	msg_one(creature, "twitches and disappears.");

      kill_creature(creature, false);
    }
  }
  
  /*
    Modify stealth and perception if the player is close.
    
    For each turn the enemy can see the player:

    - its stealth is decremented, until it's lower than the players
    perception, at which point it becomes detected

    - its perception is incremented, until it's greater than the
    players stealth, at which point it detects the player
  */
  if (can_see_creature(creature, game->player))
  {
/*    stealth = attr_base(creature, attr_stealth);
    if (stealth > 0)
      set_attr(creature, attr_stealth, --stealth);
*/    

    perception = attr_base(creature, attr_perception);
    
    set_attr(creature, attr_perception, ++perception);
    
    /* Has the player been detected? */
    if (attr_current(creature, attr_perception) > attr_current(game->player, attr_stealth))
    {
      if (enemies(game->player, creature))
	aggravate(creature);
    }
  }

  /* Is there any reason to panic? */
  if (wanna_panic(creature))
    return;

  /*
    Creatures that are blinded will try to fight anyone adjacent and
    should never attempt ranged attacks.
  */
  if (is_blinded(creature))
  {
    if (offensive(creature))
      return;
    else if (maybe())
      stumble(creature);

    return;
  }
  
  /* Just keep doing this until the function returns. */
  /* TODO: when adding states and transition conditions, we must make
   * sure we can't get stuck. Perhaps we could replace this while()
   * loop with a for() with only a limited number of "tries". */
  while (true)
  {
    /* Check which state we're currently in. */
    switch (creature->ai_state)
    {
      case ai_mimic:
	/* Mimics do nothing at all */
	return;

      case ai_idle:
	/*
	  Idle creatures don't do much. There's nothing interesting
	  going on. There's a small chance the creature will fall
	  asleep, if it needs (can) sleep.
	*/
	if (attr_current(creature, attr_p_sleep) == 0 && roll(1, 100) == 1)
	{
	  creature_sleep(creature, true);
	  return;
	}
	else if (roll(1, 20) == 1)
	{
	  /* Wow, we are bored today. Wander randomly. */
	  move_y = -1 + (tslrnd() % 3);
	  move_x = -1 + (tslrnd() % 3);
	  
	  move_creature(creature, move_y, move_x, false);
	}

	return;


	
      case ai_retreat:
	/* Have we healed enough to resume the battle? */
	/*	if ((attr_base(creature, attr_fatigue) <
	     attr_base(creature, attr_fatigue_limit) * HP_RESUME_BATTLE))
	{
	  creature->ai_state = ai_offensive;
	  continue;
	  }*/
	
	/* If the creature can blink, make it blink away */
	/*
	  RFE: Add more sophisticated escape methods, like creatures
	  teleporting back into their (fortified) lairs.
	*/
	if (try_to_invoke_ability(creature, attr_m_blink))
	  return;
	
	/* Do we have any means of healing? */
/*	if (attr_base(creature, attr_fatigue) > roll(1, 6))
	{
	  if (try_to_heal_self(creature))
	    return;
	    }*/
	
	/* RFE: If we're at least 1 tile away try to use a ranged attack. */

	/* We're still wounded. */

	if (player_ally)
	{
	  /* Fleeing allies gather around the player. */
	  if (pursue(creature, game->player->y, game->player->x))
	    return;
	}
	else
	{
	  /* Fleeing enemies try to get away from the player. */
	  if (retreat(creature, game->player->y, game->player->x))
	    return;
	}

	if ((game->player->y <= creature->y + 1) &&
	    (game->player->x <= creature->x + 1) &&
	    (game->player->y >= creature->y - 1) &&
	    (game->player->x >= creature->x - 1))
	{
	  /*
	    We're backed into a corner and there's not much else to
	    do than go berserk.
	  */
	  creature->ai_state = ai_offensive;
	  creature->retreats = false;
	  continue;
	}

	return;
	
	
	
      case ai_target:
	/* Have we reached our target? */
	if (creature->y == creature->target_y &&
	    creature->x == creature->target_x)
	{
	  creature->ai_state = ai_idle;
	  continue;
	}
	else
	{
	  if (pursue(creature, creature->target_y, creature->target_x))
	    return;
	}
	return;


	
      case ai_offensive:
	/* Determine who we should fight. */
	find_target(creature);
	
	if (attr_base(creature, attr_health) < roll(2, 6) )
	{
	  if (try_to_heal_self(creature))
	    return;
	}

	if (use_beneficial_item(creature))
	{
	  return;
	}
	else if (/*(attr_base(creature, attr_fatigue) < 3) &&*/
		 (get_effect_by_id(creature, effect_haste) == NULL) &&
		 (roll(1, 4) > 1) &&
		 try_to_invoke_ability(creature, attr_a_dash))
	{
	  return;
	}
 
	/* Are we capable of fleeing? */
	if (creature->retreats == true)
	{
	  /* Are we in danger of being killed? */
	  if (attr_base(creature, attr_health) <= 1)
	  {
	    if (can_see_creature(game->player, creature) && creature->detected)
	    {
	      msg_one(creature, "turns to flee!");
	    }
	    
	    /* Change to the "retreat" state */
	    creature->ai_state = ai_retreat;
	    creature->retreats = false;
	    continue;
	  }
	}
	
	if (offensive(creature))
	  return;

	/* We can't hit anyone from here. */

	if (player_ally)
	{
	  if (pursue(creature, game->player->y, game->player->x))
	    return;
	}
	
	if (pursue(creature, creature->target_y, creature->target_x))
	  return;

	return;
	
      default:
	return;
    }
  } /* while true */
  
  return;
} /* basic_ai */



/*
  Determines in which direction CREATURE should fire to hit the
  player. RANGE is the maximum length the projectile is allowed to
  travel. The result (-1 <= y <= 1, -1 <= x <= 1) is stored at the
  addresses pointed to by MOVE_Y and MOVE_X.
*/
/*
  TODO: Modify this to allow targeting of *any* creature, not just the
  player.
*/
/*
  TODO: Write another one, that returns the direction where a spell
  would do the most damage (e.g. use a weighting system like player =
  10 points, ally = -1 point, hostile creature = 4 points, etc.
*/
blean_t target_player(const creature_t * creature,
		      signed int * move_y, signed int * move_x,
		      const unsigned int range)
{
  level_t * level;
  unsigned int i;
  unsigned int l;
  signed int new_y;
  signed int new_x;
  
  /* Check that we got valid pointers */
  if (creature == NULL ||
      move_y == NULL ||
      move_x == NULL ||
      (level = creature->location) == NULL)
  {
    return false;
  }
  
  /* The directions are stored in the global move_mat (see main.c) */

  /* For each direction... */
  for (i = 1; i < DIRECTIONS; i++)
  {
    /* ... trace a line of at most length RANGE... */
    for (l = 1; l < range; l++)
    {
      /* What coordinates are we at now? */
      new_y = creature->y + (move_mat[i][0] * l);
      new_x = creature->x + (move_mat[i][1] * l);

      /* ... until we hit an obstacle */
      if (is_walkable(creature->location, false, new_y, new_x) == false)
      {
	/* ... or reach the player */
	if (find_creature(level, new_y, new_x) == game->player)
	{
	  /* we hit the player */
	  *move_y = move_mat[i][0];
	  *move_x = move_mat[i][1];
	  
	  return true;
	}
	else
	{
	  /* we collided with a wall or NPC, we won't hit the player in this direction... */
	  break; /* try another direction */
	}
      }
    } /* for l */
  } /* for i */

  /* We didn't hit the player */
  return false;
} /* target_player */



/*
  Determines in which direction CREATURE should fire to hit an enemy.
  RANGE is the maximum length the projectile is allowed to travel.
*/
/* RFE: This could be merged with cone_direction()!? */
dir_t target_best_direction(const creature_t * creature, unsigned int range)
{
  level_t * level;
  unsigned int i;
  unsigned int l;
  unsigned int best_value;
  unsigned int best_direction;
  signed int new_y;
  signed int new_x;
  creature_t * target;
  
  /* Check that we got valid pointers */
  if (creature == NULL ||
      (level = creature->location) == NULL)
  {
    return dir_none;
  }
  
  range--;

  /* The directions are stored in the global move_mat (see main.c) */

  best_value = 0;
  best_direction = 0;

  /* For each direction... */
  for (i = dir_n; i <= dir_nw; i++)
  {
    /* ... trace a line of at most length RANGE... */
    for (l = 1; l <= range; l++)
    {
      /* What coordinates are we at now? */
      new_y = creature->y + (move_mat[i][0] * l);
      new_x = creature->x + (move_mat[i][1] * l);

      target = find_creature(level, new_y, new_x);
      
      /* ... until we hit an obstacle */
      if ((is_flyable(level, new_y, new_x) == false) &&
	  (target == NULL))
	break; /* Break this line */

      if ((target == NULL) ||
	  (target == creature)) /* RFE: This should never happen. Can it be removed? */
	continue;

      /* ... or reach an enemy. */
      if (enemies(creature, target) == false)
      {
	/* It was a friend or someone else we don't care to attack. */
	break;
      }
      else
      {
	/* It was an enemy. */

	if (best_value == 2)
	  break;

	if (is_player(target))
	{
	  best_value = 2;
	  best_direction = i;
	}
	else if ((best_value == 0) ||
		 (roll(1, 4) != 1))
	{
	  best_value = 1;
	  best_direction = i;
	}
	
	break;
      }
    } /* for l */
  } /* for i */
  
  if (best_value > 0)
  {
    return best_direction;
  }
  else
    return dir_none;
} /* target_best_direction */



/*
  CREATURE will try to teleport by any means available. Returns true
  on success, false if they're stuck.
*/
blean_t try_to_teleport(creature_t * creature)
{
  item_t * item;

  if (creature == NULL)
    return false;
  
  if (try_to_invoke_ability(creature, attr_m_blink))
    return true;
  
  item = find_item_in_inventory(creature, treasure_s_blink);
  
  if (item)
  {
    use_item(creature, item);
    return true;
  }
  
  return false;
} /* try_to_teleport */



/*
  Checks if CREATURE is on hazardous terrain and tries to act to save
  itself.  Returns true if the turn was spent. If it returns false
  there was no reason to panic, or nothing to do about it.
*/
blean_t wanna_panic(creature_t * creature)
{
  item_t * item;
  blean_t panic;
  tile_t tile;

  if (creature == NULL)
    return false;

  panic = false;

  tile = get_tile(creature->location, creature->y, creature->x);
  
  /* If the creature is webbed it should struggle to break free. */
  if (get_effect_by_id(creature, effect_web))
  {
    struggle_web(creature);
    return true;
  }
  
  if (is_floating(creature) == false)
  {
    /* If we're in lava or in water and can't swim */

    if (tile == tile_lava)
      panic = true;
    else if (tile == tile_water && attr_current(creature, attr_free_swim) == 0)
      panic = true;
  }

  if (panic)
  {
    /* We need to get out. */

    /* Try to drink a potion of levitation. */
    item = find_item_in_inventory(creature, treasure_p_levitation);
    
    if (item)
    {
      use_item(creature, item);
      return true;
    }

    /* Try to escape by teleporting. */
    if (try_to_teleport(creature))
      return true;

    /*
      This is deliberately stupid. It's more fun and rewarding for the
      player if they just stumble around randomly and die.
    */
    if (stumble(creature))
      return true;
  }

  return false;
} /* wanna_panic */



/*
  Makes CREATURE move in a random direction (even onto hazardous
  tiles). Returns true if it consumed a turn.
*/
blean_t stumble(creature_t * creature)
{
  dir_t move_dir;
  unsigned int i;
  signed int y_speed;
  signed int x_speed;
  
  if (creature == NULL)
    return false;

  move_dir = tslrnd() % 8;

  for (i = 0; i < 8; i++)
  {
    if (move_dir == 9)
      move_dir = 0;

    move_dir++;
    dir_to_speed(move_dir, &y_speed, &x_speed);

    if (move_creature(creature, y_speed, x_speed, true))
    {
      hazard(creature);
      return true;
    }
  }

  return false;
} /* stumble */


/*
  Makes STALKER try to move as close as possible to {TARGET_Y,
  TARGET_X}.
*/
blean_t pursue(creature_t * stalker,
	       const unsigned int target_y,
	       const unsigned int target_x)
{
  signed int move_y;
  signed int move_x;
  dir_t dir;

  if (stalker == NULL ||
      stalker->location == NULL)
  {
    return false;
  }

  dir = find_path(stalker->location,
		  stalker->y, stalker->x,
		  target_y, target_x,
		  PATHFINDING, true, attr_current(stalker, attr_free_swim));

  if (dir == dir_none)
    return false;

  dir_to_speed(dir, &move_y, &move_x);
  
  if (move_creature(stalker, move_y, move_x, false))
    return true;
  else if (stalker->location->map[stalker->y + move_y][stalker->x + move_x] == tile_door_closed &&
	   open_door(stalker, stalker->y + move_y, stalker->x + move_x))
  {
    return true;
  }

  return false;
} /* pursue */



blean_t retreat(creature_t * creature,
		const unsigned int flee_y,
		const unsigned int flee_x)
{
  float best_distance;
  unsigned int best_dist_index;

  float new_distance;

  unsigned int i;
  unsigned int dist_y;
  unsigned int dist_x;
  signed int new_y;
  signed int new_x;
  
  if (creature == NULL ||
      on_map(creature->location, flee_y, flee_x) == false)
  {
    return false;
  }

  /*
    Try to get to {Y, X}. Try all 8 directions and see which
    one leads furthest away.
  */

  /* Our best bet so far is our current location. */
  dist_y = abs(flee_y - creature->y);
  dist_x = abs(flee_x - creature->x);
  best_distance = sqrt((dist_y * dist_y) + (dist_x * dist_x));
  best_dist_index = 0;

  for (i = dir_n; i <= dir_nw; i++)
  {
    new_y = (creature->y + move_mat[i][0]);
    new_x = (creature->x + move_mat[i][1]);

    if (on_map(creature->location, new_y, new_x) == false)
      continue;

    /* Can we move in this direction? */
    if (is_walkable(creature->location, false, new_y, new_x) == false)
    {
      if (find_creature(creature->location, new_y, new_x) != NULL)
	continue;
      
      if (is_wall(creature->location, new_y, new_x) == true)
	continue;
    }

    /* Calculate the current */
    dist_y = abs(flee_y - new_y);
    dist_x = abs(flee_x - new_x);
    new_distance = sqrt((dist_y * dist_y) + (dist_x * dist_x));

    /* Are we trying to get closer to or away from the player? */
    if (new_distance > best_distance)
    {
      /* It's better to move away. */
      best_distance = new_distance;
      best_dist_index = i;
    }
  }

  /* Move the creature */
  if (move_creature(creature,
		    move_mat[best_dist_index][0],
		    move_mat[best_dist_index][1],
		    false))
  {
    return true;
  }

  return false;
} /* retreat */



/*
  Tries to build a walkable path on LEVEL from {START_Y, START_X} to
  {END_Y, END_X}. It will trace at most LIMIT steps. Doors do not
  count as an obstacle. If IGNORE_CREATURES is true, creatures will
  not be considered an obstacle. If TRAVERSE_WATER is true, water will
  not be considered an obstacle.
*/
dir_t find_path(level_t * level,
		const unsigned int start_y,
		const unsigned int start_x,
		const unsigned int end_y,
		const unsigned int end_x,
		unsigned int limit,
		const blean_t ignore_creatures,
		const blean_t traverse_water)
{
  uint8_t path[200][200]; /* RFE: This isn't good. */
  blean_t t_changed;
  blean_t b_changed;
  blean_t l_changed;
  blean_t r_changed;
  blean_t any_changed;
  blean_t found;

  signed int y;
  signed int x;
  unsigned int t;
  unsigned int b;
  unsigned int l;
  unsigned int r;
  unsigned int lowest;

  dir_t ret[8];
  unsigned int count;

  if (level == NULL)
    return dir_none;

  /*
    Build a map of walls and other obstacles. We only care if
    something is walkable (value 254) or not (value 255).
  */

  for (y = 0; y < level->size_y; y++)
  {
    for (x = 0; x < level->size_x; x++)
    {
      if (is_walkable(level, false, y, x) ||
	  is_door(level, y, x) ||
	  (y == start_y && x == start_x) ||
	  (ignore_creatures && tile_info[level->map[y][x]]->walkable == true) ||
	  (traverse_water && level->map[y][x] == tile_water))
      {
	path[y][x] = 254;
      }
      else
      {
	path[y][x] = 255;
      }
    }
  }

  /*
    This is the point we are trying to reach, it has the lowest
    possible cost (it takes 1 step to get there).
  */
  path[end_y][end_x] = 1;

  /*
    We will run a floodfill on the map, starting at the end position,
    setting all adjacent walkable tiles to cost + 1. We will then
    repeat this for a limited number of times. End conditions are:
 
    - no tile was changed (if the start and end are completely isolated),
    - the destination was found,
    - we strayed too far from the start point (limit).

    We only need to operate on a small section of the map, so we set
    up a window bounded by {T, L} and {B, R}. Initially this will be
    only the 3x3 squares around the start point. If we touch any of
    the window edges, the corresponding *_changed flag will be set and
    the window will be increased at the start of the next iteration. 
  */

  any_changed = true;
  found = false;

  t_changed = b_changed = l_changed = r_changed = false;

  t = end_y - 1;
  b = end_y + 1;
  l = end_x - 1;
  r = end_x + 1;

  while (any_changed && !found && limit--)
  {
    if (t_changed) t--;
    if (b_changed) b++;
    if (l_changed) l--;
    if (r_changed) r++;

    t_changed = b_changed = l_changed = r_changed = false;
    any_changed = false;

    /*
      To avoid crashing (or very complex if statements) we will never
      grow the window to the edge of the map (we don't need to find
      any paths there anyway).
    */
    t = MAX(1, t);
    b = MIN(b, level->size_y - 2);
    l = MAX(1, l);
    r = MIN(r, level->size_x - 2);
    
    /* For each tile on the map... */
    for (y = t; y <= b; y++)
    {
      for (x = l; x <= r; x++)
      {
	/*
	  We can skip it entirely if it the square isn't walkable or
	  we have already determined a cost for it.
	*/
	if (path[y][x] == 255 ||
	    path[y][x] < 254)
	  continue;

	/* Let's assume the best direction is "infinite". */
	lowest = 254;
	
	/*
	  Check each of the eight adjacent tiles for the one with
	  lowest cost. This is the one we will propagate outwards.
	*/
	if (path[y - 1][x    ] < lowest) lowest = path[y - 1][x    ];
	if (path[y - 1][x + 1] < lowest) lowest = path[y - 1][x + 1];
	if (path[y    ][x + 1] < lowest) lowest = path[y    ][x + 1];
	if (path[y + 1][x + 1] < lowest) lowest = path[y + 1][x + 1];
	if (path[y + 1][x    ] < lowest) lowest = path[y + 1][x    ];
	if (path[y + 1][x - 1] < lowest) lowest = path[y + 1][x - 1];
	if (path[y    ][x - 1] < lowest) lowest = path[y    ][x - 1];
	if (path[y - 1][x - 1] < lowest) lowest = path[y - 1][x - 1];

	if (lowest < 254)
	{
	  /* We found a reasonable cost. */
	  path[y][x] = lowest + 1;

	  any_changed = true;

	  /* Did we affect any edge tiles? */
	  if (y == t) t_changed = true;
	  if (y == b) b_changed = true;
	  if (x == l) l_changed = true;
	  if (x == r) r_changed = true;

	  /*
	    Did we find what we were looking for? We'll fulfill the
	    iteration anyway, there could be two paths of equal cost
	    leading to the same place (forked corridors, etc).
	  */
	  if (y == start_y && x == start_x)
	    found = true;
	}
      }
    }
  }

  /* We have completed the floodfill. Did we find anything? */
  if (found == false)
    return dir_none; /* Too bad. */

  /*
    Find the square adjacent to the start position with the lowest
    cost. If there are several we want to choose between them without
    bias; this is what ret[] and count are for.
  */
  lowest = 253;
  count = 0;

  for (r = dir_n; r <= dir_nw; r++)
  {
    dir_to_speed(r, &y, &x);

    t = path[start_y + y][start_x + x];

    if (t <= lowest)
    {
      /*
	We will save every candidate direction in ret. If we discover
	one that is strictly LOWER, we'll reset the counter and start
	over.
      */
      if (t < lowest)
	count = 0;

      ret[count] = r;
      count++;
      lowest = t;
    }
  }
  
  if (count > 0)
  {
    /* Pick any of the candidate directions. */
    return ret[tslrnd() % count];
  }
  else
  {
    return dir_none; /* Too bad. */
  }
} /* find_path */



/*
  Makes CREATURE carry out some offensive action to a nearby
  non-aligned creature. Returns TRUE if the action was carried out,
  otherwise FALSE.
*/
blean_t offensive(creature_t * creature)
{
  signed int move_y;
  signed int move_x;
  item_t * missile;
  dir_t dir;
  
  if (creature == NULL)
    return false;

  if (is_blinded(creature) == false)
  {
    if (maybe() && try_to_invoke_ability(creature, attr_a_leap))
      return true;
    
    if (maybe() && try_to_invoke_ability(creature, attr_m_sticky_web))
      return true;
    
    if (maybe() && try_to_invoke_ability(creature, attr_m_frost_ray))
      return true;
    
    if (maybe() && try_to_invoke_ability(creature, attr_m_force_bolt))
      return true;
    
    if (maybe() && try_to_invoke_ability(creature, attr_m_mudball))
      return true;
    
    if (maybe() && try_to_invoke_ability(creature, attr_m_noxious_breath))
      return true;
    
    /* RFE: We shouldn't cast it if there's a risk of damaging ourselves. */
    if (maybe() && try_to_invoke_ability(creature, attr_m_fireball))
      return true;
    
    if (maybe() && try_to_invoke_ability(creature, attr_a_aimed_shot))
      return true;
  }
  
/*    if ((roll(1, 4) == 1) &&
      try_to_invoke_ability(creature, attr_m_bone_crush))
	return true;*/
  
  if (maybe() && try_to_invoke_ability(creature, attr_m_shock))
    return true;
    
  /* We've exhausted all offensive magic. Are we allowed to fight? */
  if (creature->pacifist)
    return false; /* We're not. */
  
  /*
    Is there an enemy in any adjacent square? This uses range 2 since
    range 1 would mean attacking our own square. Yeah, I know, it's
    complicated.
  */
  if (next_weapon(creature) != NULL)
  {
    dir = target_best_direction(creature, 2);
    
    if (dir != dir_none)
    {
      creature_t * target;
      
      dir_to_speed(dir, &move_y, &move_x);
      
      target = find_creature(creature->location,
			     creature->y + move_y,
			     creature->x + move_x);
      
      if (target != NULL)
      {
	attack(creature, target);
	return true;
      }
    }
  }
  else
  {
    msg_one(creature, "can't fight!");
  }

  /* Blinded creatures don't attempt ranged attacks. */
  if (is_blinded(creature))
  {
    return false;
  }
        
  /* Do we have any missile weapon equipped? */
  if ((get_equipped(creature, item_type_ammo, 0) != NULL) &&
      (get_equipped(creature, item_type_r_weapon, 0) != NULL))
  {
    unsigned int launcher_range;

    launcher_range = get_weapon_range(get_equipped(creature,
						   item_type_r_weapon,
						   0));
    
    missile = get_item_from_stack(get_equipped(creature,
					       item_type_ammo, 0));
    
    /* Is there anyone nearby we can shoot a missile at? */
    dir = target_best_direction(creature, launcher_range);

    if (dir != dir_none)
    {
      fire_missile(creature, dir,
		   get_equipped(creature, item_type_r_weapon, 0),
		   detach_item(missile), false, 
		   launcher_range);
      
      return true;
    }
  }
  
  if (creature->id != monster_chainsaw_ogre)
  {
    /* Do we have any loose item that we can throw? */
    missile = get_item_from_stack(get_throw_item(creature));
    
    if (missile != NULL)
    {
      unsigned int range;

      range = attr_current(creature, attr_throw_range);
      
      /* Is there anyone nearby we can throw it at? */
     
      dir = target_best_direction(creature, range + 1);

      if (dir != dir_none)
      {
	fire_missile(creature, dir, NULL,
		     detach_item(missile), false, range);
	return true;
      }
    }
  }
  
  return false;
} /* offensive */



/*
  Picks an item from CREATUREs inventory to be thrown at the player.
*/
item_t * get_throw_item(creature_t * creature)
{
  item_t * temp;

  if (creature == NULL)
    return NULL;

  for (temp = creature->first_item; temp != NULL; temp = temp->next_item)
  {
    if (temp->equipped == false)
    {
      if (0
	  || (temp->item_number == treasure_p_pain)
	  || (temp->item_number == treasure_p_slowing)
	  || (temp->item_number == treasure_p_sleep)
	  || (temp->item_number == treasure_p_poison)
	  || (temp->item_type == item_type_ammo)
	  || (temp->item_type == item_type_m_weapon)
	)
      {
	return temp;
      }
    }
  }

  /* Couldn't find any suitable item */
  return NULL;
} /* throw_item */



/*
  Returns any item from CREATUREs inventory with item number NUMBER,
  otherwise NULL. Note: this returns an entire stack.
*/
item_t * get_specific_item(creature_t * creature, const unsigned int number)
{
  item_t * item;

  if (creature == NULL)
    return NULL;

  for (item = creature->first_item; item != NULL; item = item->next_item)
  {
    if (item->item_number == number)
      return item;
  }

  return NULL;
} /* get_specific_item */



/*
  Makes CREATURE try to use any healing items (or abilities) it
  has. Returns true if a turn was spent.
*/
blean_t try_to_heal_self(creature_t * creature)
{ 
  item_t * item;

  if (creature == NULL)
    return false;

  if ((item = get_specific_item(creature, treasure_p_instant_healing)) != NULL)
  {
    use_item(creature, item);
    return true;
  }
  else if ((item = get_specific_item(creature, treasure_p_healing)) != NULL)
  {
    use_item(creature, item);
    return true;
  }

  return false;
} /* try_to_heal_self */



blean_t use_beneficial_item(creature_t * creature)
{
  item_t * item;

  if (creature == NULL)
    return false;

  if ((roll(1, 4) == 1) &&
      (item = get_specific_item(creature, treasure_p_polymorph)) != NULL)
  {
    use_item(creature, item);
    return true;
  }
  else if ((get_effect_by_id(creature, effect_haste) == NULL) &&
	   ((item = get_specific_item(creature, treasure_p_speed)) != NULL))
  {
    use_item(creature, item);
    return true;
  }

  return false;
} /* use_beneficial_item */



/*
  
 */
dir_t cone_direction(const creature_t * creature,
		     unsigned int range,
		     const blean_t ray)
{
  unsigned int best_dir;
  signed int best_score;
  unsigned int i;
  signed int temp_score;
  level_t * level;
  creature_t * target;
  area_t * area;
  area_t * temp_area;

  if (creature == NULL)
    return dir_none;

  level = creature->location;

  if (level == NULL)
    return dir_none;

  /*
    This is what we will return if we don't find any better direction
    for our cone.
  */
  best_dir = dir_none;
  best_score = 0;

  /* Test each direction */
  for (i = dir_n; i < dir_nw; i++)
  {
    /* Generate an area set for a cone in this direction. */
    area = new_area(creature->y, creature->x);
    area_cone(area, level, creature->y, creature->x, i, range, ray);

    /* Sum the score of creatures that would be affected by such a cone. */
    temp_score = 0;
    temp_area = area->next;

    while (temp_area != NULL)
    {
      target = find_creature(level, temp_area->y, temp_area->x);
      
      temp_area = temp_area->next;

      if (target == NULL)
	continue;

      if (is_player(target))
	temp_score += 4;
/*      else if (target->alignment == creature->alignment)
	temp_score--;*/
      else if (enemies(creature, target))
	temp_score++;
    }

    /* We're done calculating this direction. */
    del_area(area);

    /*
      Compare scores with previous. If it's strictly better we'll
      always use the new direction. If it's equivalent, there's only a
      random chance we will replace it (but only if it's nonzero -
      otherwise enemies will be confuse and cast in random
      directions).
    */
    if (temp_score > best_score ||
	(temp_score > 0 && temp_score == best_score && roll(1, 4) == 1))
    {
      best_dir = i;
      best_score = temp_score;
    }    
  }

  return best_dir;
} /* cone_direction */



void find_target(creature_t * creature)
{
  level_t * level;
  unsigned int i;
  creature_t * target;

  if (creature == NULL)
    return;

  level = creature->location;

  if (level == NULL)
    return;

  if (enemies(creature, game->player))
  {
    creature->target_y = game->player->y;
    creature->target_x = game->player->x;
  }

  for (i = 0; i < level->creatures; i++)
  {
    target = level->creature[i];

    if (target == NULL)
      continue;

    if (enemies(creature, target))
    {
      if (can_see_creature(creature, target) ||
	  is_player(target))
      {
	creature->target_y = target->y;
	creature->target_x = target->x;
      }
    }
  }

  return;
} /* find_target */
