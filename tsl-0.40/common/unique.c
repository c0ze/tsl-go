#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>

#include "unique.h"
#include "rules.h"
#include "item.h"
#include "treasure.h"
#include "monster.h"
#include "inventory.h"
#include "creature.h"
#include "places.h"
#include "vweapon.h"



/*
  Builds the unique creature with index ID. Each creature must only be
  built once; if an already spawned unique is requested, NULL is
  returned. Also, uniques not present in the current game won't be
  built.
*/
creature_t * build_unique(const monster_t id)
{
  creature_t * local;

  if (id >= unique_last ||
      id <= unique_first ||
      (unique_status[id] != unique_status_available))
  {
    return NULL;
  }

  local = alloc_creature();
  local->id = id;
  
/*  local->alignment = alignment_enemy;*/

  /* Register that this unique has entered the game */
  unique_status[id] = unique_status_spawned;

  switch (id)
  {
    case unique_dragon:
      set_creature_name(local, "the Dragon", "the Dragon", "the Dragon");

      local->corpse = artifact_dragon_scales;

      local->gent = gent_dragon;
      
      set_attr(local, attr_health, 55);
      set_attr(local, attr_attack, 95);
      set_attr(local, attr_stealth, 35);
      set_attr(local, attr_perception, 25);
      set_attr(local, attr_unchanging, 1);
      set_attr(local, attr_speed, 130);
      set_attr(local, attr_fire_resistance, DRAGON_FIRE_RESISTANCE);
      set_attr(local, attr_ep_max, 300);
      set_attr(local, attr_m_breathe_fire, 1);

      local->unarmed = virtual_claws;
      break;

    case unique_argor:
      set_creature_name(local, "Argor", "Argor", "Argor");

      set_attr(local, attr_health, 20);
      set_attr(local, attr_speed, 14);
      set_attr(local, attr_ep_max, 300);

      del_item(attach_item_to_creature(local, random_treasure(item_table_scroll)));
      del_item(attach_item_to_creature(local, random_treasure(item_table_scroll)));

      local->unarmed = virtual_cold_touch;
      break;

    case unique_necromancer:
      set_creature_name(local, "the Necromancer", "the Necromancer", "the Necromancer");
      local->gent = gent_necromancer;

      set_attr(local, attr_health, 30);
      set_attr(local, attr_unchanging, 1);
      set_attr(local, attr_speed, 10);
      set_attr(local, attr_ep_max, 200);
      set_attr(local, attr_m_bone_crush, 1);
      set_attr(local, attr_m_force_bolt, 1);
      set_attr(local, attr_m_fireball, 1);
      set_attr(local, attr_m_frost_ray, 1);

      del_item(attach_item_to_creature(local, random_treasure(item_table_scroll)));
      del_item(attach_item_to_creature(local, random_treasure(item_table_scroll)));

      local->unarmed = virtual_cold_touch;
      break;

    case unique_king_of_worms:
      set_creature_name(local, "the King of Worms", "the King of Worms", "the King of Worms");
      local->gent = gent_king_of_worms;

      set_attr(local, attr_health, 30);
      set_attr(local, attr_speed, 10);
      set_attr(local, attr_ep_max, 200);
      set_attr(local, attr_unchanging, 1);
      set_attr(local, attr_p_sleep, 1);
      break;

    case unique_caeltzan:
      set_creature_name(local, "Cael'Tzan the Undertaker", "Cael'Tzan the Undertaker", "Cael'Tzan the Undertaker");
      local->gent = gent_caeltzan;

      set_attr(local, attr_speed,  3);

      set_attr(local, attr_health, 30);
      set_attr(local, attr_ep_max, 14);

      set_attr(local, attr_cold_resistance,   0);

      set_attr(local, attr_p_sleep, 1);

      local->unarmed = virtual_claws;
      break;

    case unique_ghrazghaar:
      set_creature_name(local, "Ghrazghaar", "Ghrazghaar", "Ghrazghaar");

      set_attr(local, attr_health, 70);

      local->unarmed = virtual_claws;
      break;

    case unique_gaoler:
      set_creature_name(local, "the Gaoler", "the Gaoler", "the Gaoler");
      local->gent = gent_gaoler;

      set_attr(local, attr_health, 26);
      set_attr(local, attr_speed, 88);

      set_attr(local, attr_acid_resistance, 2);
      set_attr(local, attr_cold_resistance, 2);
      set_attr(local, attr_fire_resistance, 2);

      use_or_destroy(local, build_item(artifact_whip_of_thorns));
      use_or_destroy(local, random_treasure(item_table_armor));
      use_or_destroy(local, random_treasure(item_table_armor));
      use_or_destroy(local, random_treasure(item_table_weapon));
      break;

    case unique_lurker:
      set_creature_name(local, "the Lurker", "the Lurker", "the Lurker");
      local->gent = gent_lurker;

      local->altitude = altitude_swimming;

      set_attr(local, attr_unchanging, 1);
      set_attr(local, attr_free_swim, 1);
      set_attr(local, attr_permaswim, 1);
      set_attr(local, attr_p_sleep, 1);
      set_attr(local, attr_health, 22);
      set_attr(local, attr_attack, 95);
      set_attr(local, attr_ep_max, 10);
      set_attr(local, attr_dodge, 0);
      set_attr(local, attr_speed, 120);
      set_attr(local, attr_perception, 4);
      set_attr(local, attr_vision, 8);

      set_attr(local, attr_m_noxious_breath, 1);

      local->unarmed = virtual_poison_fangs;
      break;

    case unique_sulkor:
      set_creature_name(local, "Sulkor the Devourer", "Sulkor the Devourer", "Sulkor the Devourer");

      set_attr(local, attr_health, 30);

      local->unarmed = virtual_fangs;
      break;

    case unique_chaajd:
      set_creature_name(local, "Cha'ajd the Mad", "Cha'ajd the Mad", "Cha'ajd the Mad");

      set_attr(local, attr_health, 25);

      local->unarmed = virtual_fangs;
      break;

    case unique_lognac:
      set_creature_name(local, "Sir Lognac", "Sir Lognac", "Sir Lognac");

      set_attr(local, attr_health, 25);
      set_attr(local, attr_speed, 90);

      use_or_destroy(local, random_treasure(item_table_armor));
      use_or_destroy(local, random_treasure(item_table_weapon));
      break;

    case unique_ybznek:
      set_creature_name(local, "Ybznek", "Ybznek", "Ybznek");

      set_attr(local, attr_health, 50);

      set_attr(local, attr_speed, 80);

      local->unarmed = virtual_claws;

      use_or_destroy(local, random_treasure(item_table_treasure));
      use_or_destroy(local, random_treasure(item_table_armor));
      use_or_destroy(local, random_treasure(item_table_weapon));
      break;

    default:
      del_creature(local);
      return NULL;
  }

  /* Make sure all creatures start out at max EP + HP - this way, we
     won't have to set both max and current for each creature. */
  set_attr(local, attr_ep_current, attr_current(local, attr_ep_max));

  return local;
} /* build_unique */



/* 
   Initiates the list of uniques.
*/
void init_uniques()
{
  unsigned int i;

  for (i = 0; i < monsters; i++)
  {
    unique_status[i] = unique_status_available;
  }

  return;
} /* init_uniques */
