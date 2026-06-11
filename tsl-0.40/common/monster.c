#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "stuff.h"
#include "monster.h"
#include "item.h"
#include "treasure.h"
#include "game.h"
#include "ai.h"
#include "places.h"
#include "inventory.h"
#include "fov.h"
#include "vweapon.h"



/*
  Builds a creature of type MONSTER.
*/
creature_t * build_monster(const monster_t monster)
{
  creature_t * creature;
  item_t * item;

  creature = alloc_creature();

  creature->id = monster;

/*  creature->alignment = alignment_enemy;*/

  set_attr(creature, attr_stealth, 0);
  set_attr(creature, attr_perception, 0);

  switch (monster)
  {
    case monster_electric_snake:
      set_creature_name(creature, "an electric snake", "the electric snake", "electric snake");
      creature->gent = gent_electric_snake;
      creature->parts = part_eyes;

      set_attr(creature, attr_health, 3);
      set_attr(creature, attr_speed, 35);
      set_attr(creature, attr_p_open_doors, 1);
      set_attr(creature, attr_p_pick_up, 1);

      set_attr(creature, attr_m_shock, 1);
      set_attr(creature, attr_m_recharge, 1);

      if (maybe() && maybe())
	creature->corpse = treasure_carcass;

      creature->unarmed = virtual_fangs;
      break;

    case monster_graveling:
      set_creature_name(creature, "a graveling", "the graveling", "graveling");
      creature->gent = gent_graveling;
      creature->parts =
	part_head | part_eyes | part_throat | part_ribs | part_neck |
	part_bowel | part_teeth | part_skull | part_tail;

      set_attr(creature, attr_health, 5);
      set_attr(creature, attr_ep_max, 10);
      set_attr(creature, attr_speed, 60);
      set_attr(creature, attr_perception, sroll("1d6"));
      set_attr(creature, attr_p_open_doors, 1);
      set_attr(creature, attr_p_read, 1);
      set_attr(creature, attr_a_leap, 1);
      set_attr(creature, attr_carrying_capacity, LOW_CARRYING_CAPACITY);

      if (maybe() && maybe())
	creature->corpse = treasure_carcass;

      if (roll(1, 4) == 1)
	use_or_destroy(creature, random_treasure(item_table_armor));

      if (roll(1, 8) == 1)
	attach_item_to_creature(creature, build_item(treasure_torch));

      creature->unarmed = virtual_claws;
      break;


    case monster_sludge_dweller:
      set_creature_name(creature, "a sludge dweller", "the sludge dweller", "sludge dweller");
      set_death_msg(creature, "is destroyed!");
      creature->gent = gent_sludge_dweller;
      creature->parts =
	part_head | part_eyes | part_throat | part_ribs | part_neck |
	part_bowel | part_teeth | part_skull;

      // creature->corpse = treasure_corpse;

      set_attr(creature, attr_health, 2);
      set_attr(creature, attr_speed, 60);
      set_attr(creature, attr_m_mudball, 1);

      creature->retreats = false;
      break;

    case monster_ratman:
      set_creature_name(creature, "a ratman", "the ratman", "ratman");
      creature->gent = gent_ratman;
      creature->parts =
	part_head | part_eyes | part_throat | part_ribs | part_neck |
	part_bowel | part_teeth | part_skull;

      set_attr(creature, attr_health, 2);
      set_attr(creature, attr_speed, 90);
      set_attr(creature, attr_perception, sroll("1d3"));
      set_attr(creature, attr_throw_range, 5);
      set_attr(creature, attr_carrying_capacity, LOW_CARRYING_CAPACITY);

      if (maybe() && maybe())
	creature->corpse = treasure_corpse;

      if (maybe())
	use_or_destroy(creature, build_item(treasure_filthy_rags));
      else if (maybe())
	use_or_destroy(creature, build_item(treasure_leather));
      else if (maybe())
	use_or_destroy(creature, random_treasure(item_table_armor));

      if (roll(1, 10) == 1)
	attach_item_to_creature(creature, build_item(treasure_cheese));

      if (roll(1, 6) == 1)
	attach_item_to_creature(creature, build_item(treasure_d_poison_pile));

      if (roll(1, 20) == 1)
	attach_item_to_creature(creature, build_item(treasure_p_speed));
      else if (roll(1, 20) == 1)
	attach_item_to_creature(creature, build_item(treasure_p_polymorph));
      else if (roll(1, 10) == 1)
	attach_item_to_creature(creature, build_item(treasure_p_healing));

      creature->unarmed = virtual_claws;
      break;

    case monster_gnoblin:
      set_creature_name(creature, "a gnoblin", "the gnoblin", "gnoblin");
      creature->gent = gent_gnoblin;
      creature->parts =
	part_head | part_eyes | part_throat | part_ribs | part_neck |
	part_bowel | part_teeth | part_skull;

      if (maybe() && maybe())
	creature->corpse = treasure_corpse;

      set_attr(creature, attr_health, 4);
      set_attr(creature, attr_speed, 85);
      set_attr(creature, attr_perception, sroll("1d3"));
      set_attr(creature, attr_ep_max, 100);
      set_attr(creature, attr_vision, 2);
      set_attr(creature, attr_carrying_capacity, LOW_CARRYING_CAPACITY);

      if (roll(1, 4) == 1)
      {
	set_creature_name(creature, "a gnoblin archer", "the gnoblin archer", "gnoblin archer");
	set_attr(creature, attr_a_aimed_shot, 1);

	if (maybe())
	{
	  if (maybe())
	    use_or_destroy(creature, build_item(treasure_d_poison_pile));
	  else
	    use_or_destroy(creature, build_item(treasure_d_small_pile));

	  if (maybe())
	    use_or_destroy(creature, build_item(treasure_crossbow));
	  else if (maybe())
	    use_or_destroy(creature, build_item(treasure_blowgun));
	}
	else
	{
	  use_or_destroy(creature, build_item(treasure_longbow));

	  if (maybe())
	    use_or_destroy(creature, build_item(treasure_a_sharp_pile));
	  else
	    use_or_destroy(creature, build_item(treasure_a_crude_pile));
	}
      }
      else if (roll(1, 4) == 1)
      {
	set_creature_name(creature, "a gnoblin thief", "the gnoblin thief", "gnoblin thief");

	set_attr(creature, attr_stealth, roll(2, 5));
	set_attr(creature, attr_perception, roll(2, 5));

	if (maybe())
	  use_or_destroy(creature, random_treasure(item_table_weapon));
	
	if (maybe())
	  use_or_destroy(creature, random_treasure(item_table_armor));
      }
      else
      {
	if (roll(1, 4) != 1)
	  creature->corpse = treasure_decapitated_head;
	
	if (maybe())
	  use_or_destroy(creature, random_treasure(item_table_weapon));
	
	if (maybe())
	  use_or_destroy(creature, random_treasure(item_table_armor));

	if (maybe())
	  use_or_destroy(creature, random_treasure(item_table_armor));
      }

      if (roll(1, 8) == 1)
	attach_item_to_creature(creature, build_item(treasure_torch));
      else if (roll(1, 10) == 1)
	attach_item_to_creature(creature, build_item(treasure_lantern));

      break;

    case monster_crypt_vermin:
      set_creature_name(creature, "a crypt vermin", "the crypt vermin", "crypt vermin");
      creature->gent = gent_crypt_vermin;

      set_attr(creature, attr_health, 6);
      set_attr(creature, attr_speed, 130);
      set_attr(creature, attr_perception, sroll("1d6+1"));
      set_attr(creature, attr_p_open_doors, 1);
      set_attr(creature, attr_p_pick_up, 1);
      /*set_attr(creature, attr_carrying_capacity, 0);*/

      creature->unarmed = virtual_fangs;
      break;

    case monster_tentacle:
      set_creature_name(creature, "a tentacle", "the tentacle", "tentacle");
      creature->gent = gent_tentacle;

      set_attr(creature, attr_unchanging, 1);
      set_attr(creature, attr_free_swim, 1);
      set_attr(creature, attr_permaswim, 1);
      set_attr(creature, attr_dodge, 0);
      set_attr(creature, attr_backstab_immunity, 1);
      set_attr(creature, attr_p_pick_up, 1);
      set_attr(creature, attr_p_sleep, 1);

      set_attr(creature, attr_health, 6);
      set_attr(creature, attr_perception, 4);
      set_attr(creature, attr_vision, 5);
      set_attr(creature, attr_speed, 60);
      set_attr(creature, attr_carrying_capacity, 0);
      set_attr(creature, attr_poison_resistance, POISON_IMMUNITY);

      creature->altitude = altitude_swimming;

      creature->unarmed = virtual_slap;
      break;

    case monster_burning_skull:
      set_creature_name(creature, "a burning skull", "the burning skull", "burning skull");
      /* set_death_msg(creature, "!");*/
      creature->gent = gent_burning_skull;

      set_attr(creature, attr_health, 2);
      set_attr(creature, attr_speed, 150);
      set_attr(creature, attr_perception, 6);
      set_attr(creature, attr_nonbreathing, 1);
      set_attr(creature, attr_permafloat, 1);
      set_attr(creature, attr_fire_resistance, +3);
      set_attr(creature, attr_p_pick_up, 1);
      set_attr(creature, attr_p_open_doors, 1);
      set_attr(creature, attr_poison_resistance, POISON_IMMUNITY);
      set_attr(creature, attr_p_sleep, 1);

      set_attr(creature, attr_carrying_capacity, 0);

      set_attr(creature, attr_ep_max, 15);
      set_attr(creature, attr_m_breathe_fire, 1);

      creature->retreats = false;
      creature->unarmed = virtual_fangs;

      if (maybe() && maybe())
	creature->corpse = treasure_cranium;
      break;
      
    case monster_severed_hand:
      set_creature_name(creature, "a severed hand", "the severed hand", "severed hand");
      set_death_msg(creature, "is destroyed!");
      creature->gent = gent_severed_hand_m;

      set_attr(creature, attr_health, 4);
      set_attr(creature, attr_speed, 80);
      set_attr(creature, attr_nonbreathing, 1);
      set_attr(creature, attr_permafloat, 1);
      set_attr(creature, attr_backstab_immunity, 1);
      set_attr(creature, attr_p_pick_up, 1);
      set_attr(creature, attr_p_drink, 1);
      set_attr(creature, attr_p_eat, 1);
      set_attr(creature, attr_p_sleep, 1);

      set_attr(creature, attr_carrying_capacity, 0);

      creature->retreats = false;

      creature->unarmed = virtual_slap;

      creature->corpse = treasure_severed_hand;

      /*use_or_destroy(creature, build_any_item("ring"));*/
      break;
      
    case monster_slime:
      set_creature_name(creature, "a slime", "the slime", "slime");
      set_death_msg(creature, "is destroyed!");
      creature->gent = gent_slime;

      set_attr(creature, attr_backstab_immunity, 1);
      set_attr(creature, attr_wound_immunity, 1);
      set_attr(creature, attr_health, 18);
      set_attr(creature, attr_speed, 80);
      set_attr(creature, attr_dodge, 0);
      set_attr(creature, attr_acid_resistance, 10);
      set_attr(creature, attr_nonbreathing, 1);
      set_attr(creature, attr_p_pick_up, 1);
      set_attr(creature, attr_carrying_capacity, 0);
      set_attr(creature, attr_dissolve, 1);
      set_attr(creature, attr_p_drink, 1);
      set_attr(creature, attr_p_eat, 1);
      set_attr(creature, attr_p_sleep, 1);
      set_attr(creature, attr_p_open_doors, 1);
      set_attr(creature, attr_poison_resistance, POISON_IMMUNITY);

      creature->retreats = false;

      creature->unarmed = virtual_slime;
      break;

    case monster_merman:
      set_creature_name(creature, "a merman", "the merman", "merman");
      creature->gent = gent_merman;
      creature->parts =
	part_head | part_throat | part_eyes | part_ribs | part_neck |
	part_bowel | part_teeth | part_skull;

      set_attr(creature, attr_health, 8);
      set_attr(creature, attr_speed, 65);
      set_attr(creature, attr_free_swim, 1);

      if (maybe())
	use_or_destroy(creature, random_treasure(item_table_merman));

      creature->unarmed = virtual_claws;
      break;

    case monster_imp:
      set_creature_name(creature, "an imp", "the imp", "imp");
      creature->gent = gent_imp;
      creature->parts =
	part_head | part_throat | part_eyes | part_ribs |
	part_neck | part_bowel | part_teeth | part_tail | part_skull;

      set_attr(creature, attr_health, 2);
      set_attr(creature, attr_speed, 45);
      set_attr(creature, attr_fire_resistance, 2);
      set_attr(creature, attr_cold_resistance, 2);
      set_attr(creature, attr_m_force_bolt, 1);
      set_attr(creature, attr_carrying_capacity, LOW_CARRYING_CAPACITY);

      creature->unarmed = virtual_claws;
      break;

    case monster_scarecrow:
      set_creature_name(creature, "a scarecrow", "the scarecrow", "scarecrow");
      creature->gent = gent_scarecrow;
      creature->parts = 0;

      set_attr(creature, attr_health, 15);
      set_attr(creature, attr_speed, 80);

      creature->unarmed = virtual_cabbage_hands;
      break;

    case monster_chrome_angel:
      set_creature_name(creature, "a chrome angel", "the chrome angel", "chrome angel");
      creature->gent = gent_chrome_angel;
      creature->parts = 0;

      set_attr(creature, attr_health, 60);
      set_attr(creature, attr_speed, 130);

      set_attr(creature, attr_m_blink, 1);

      creature->unarmed = virtual_flame_hands;
      break;

    case monster_nameless_horror:
      set_creature_name(creature, "a nameless horror", "the nameless horror", "nameless horror");
      creature->gent = gent_nameless_horror;
      creature->parts = 0;

      set_attr(creature, attr_health, 60);
      set_attr(creature, attr_speed, 130);
      set_attr(creature, attr_m_blink, 1);

      creature->unarmed = virtual_flame_hands;
      break;

    case monster_chainsaw_ogre:
      set_creature_name(creature, "a chainsaw ogre", "the chainsaw ogre", "chainsaw ogre");
      creature->gent = gent_chainsaw_ogre;
      creature->parts =
	part_head | part_eyes | part_throat | part_ribs | part_neck |
	part_bowel | part_teeth | part_skull;
      
      set_attr(creature, attr_health, 8);
      set_attr(creature, attr_speed, 120);
      set_attr(creature, attr_vision, 3);
      set_attr(creature, attr_ep_max, 50);
      set_attr(creature, attr_a_aimed_shot, 1);
      set_attr(creature, attr_throw_range, 5);
      set_attr(creature, attr_carrying_capacity,
		    DEFAULT_CARRYING_CAPACITY * 3);

      use_or_destroy(creature, build_item(treasure_grenade));
      if (maybe())
	attach_item_to_creature(creature, build_item(treasure_grenade));
      if (maybe())
	attach_item_to_creature(creature, build_item(treasure_grenade));
      
      if (maybe() && maybe())
	creature->corpse = treasure_ogre_corpse;

      creature->unarmed = virtual_chainsaw;
      break;

    case monster_floating_brain:
      set_creature_name(creature, "a floating brain", "the floating brain", "floating brain");
      creature->gent = gent_floating_brain;

      set_attr(creature, attr_p_pick_up, 1);
      set_attr(creature, attr_carrying_capacity, 0);
      set_attr(creature, attr_health, 15);
      set_attr(creature, attr_ep_max, 400);
      set_attr(creature, attr_nonbreathing, 1);
      set_attr(creature, attr_speed, 70);
      set_attr(creature, attr_permafloat, 1);

      set_attr(creature, attr_m_blink, 1);
      set_attr(creature, attr_m_force_bolt, 1);

      set_attr(creature, attr_p_drink, 1);
      set_attr(creature, attr_p_open_doors, 1);
      set_attr(creature, attr_p_eat, 1);
      set_attr(creature, attr_vision, 2);
      set_attr(creature, attr_p_sleep, 1);

      creature->pacifist = true;

      creature->unarmed = virtual_fists;
      break;

    case monster_mimic:
      set_creature_name(creature, "a mimic", "the mimic", "mimic");

      set_attr(creature, attr_health, 5);
      set_attr(creature, attr_speed, 70);
      set_attr(creature, attr_dodge, 0);
      set_attr(creature, attr_p_move, 1);
      set_attr(creature, attr_p_drink, 1);
      set_attr(creature, attr_p_eat, 1);
      set_attr(creature, attr_p_pick_up, 1);
      set_attr(creature, attr_p_sleep, 1);
      set_attr(creature, attr_perception, 100);
      set_attr(creature, attr_carrying_capacity, 0);

      creature->ai_state = ai_mimic;
      creature->retreats = false;
      creature->detected = true;

      switch (roll(1, 4))
      {
	case 1:
	  item = build_any_item("", false);
	  set_creature_name(creature, item->single_unid_name, item->single_unid_name, item->single_unid_name);
	  creature->corpse = item->item_number;
	  creature->gent = item->gent;
	  del_item(item);
	  break;

	case 2:
	  set_creature_name(creature, "a dart trap", "the dart trap", "dart trap");
	  creature->corpse = treasure_d_small_pile;
	  creature->gent = gent_dart_trap;
	  break;

	case 3:
	  set_creature_name(creature, "a medkit", "the medkit", "medkit");
	  creature->gent = gent_medkit;
	  break;

	case 4:
	  set_creature_name(creature, "a stair", "the stair", "stair");
	  creature->gent = gent_stairs;
	  break;
      }

      creature->unarmed = virtual_mimic_fangs;
      break;

    case monster_gloom_lord:
      set_creature_name(creature, "a gloom lord", "the gloom lord", "gloom lord");
      set_death_msg(creature, "collapses into dust!");
      creature->gent = gent_gloom_lord;
      creature->parts = part_ribs | part_neck | part_skull;

      set_attr(creature, attr_health, 28);
      set_attr(creature, attr_nonbreathing, 1);
      set_attr(creature, attr_speed, 40);
      set_attr(creature, attr_perception, sroll("2d4+2"));
      set_attr(creature, attr_fire_resistance, 1);
      set_attr(creature, attr_cold_resistance, 1);
      set_attr(creature, attr_poison_resistance, POISON_IMMUNITY);
      set_attr(creature, attr_p_sleep, 1);

      use_or_destroy(creature, build_item(treasure_crappy_weapon));

      if (maybe() && maybe())
	creature->corpse = treasure_cranium;

      creature->retreats = false;

      use_or_destroy(creature, random_treasure(item_table_weapon));

      break;

    case monster_ghoul:
      set_creature_name(creature, "a ghoul", "the ghoul", "ghoul");
      set_death_msg(creature, "is destroyed!");
      creature->gent = gent_ghoul;
      creature->parts =
	part_head | part_eyes | part_throat | part_ribs | part_neck | part_skull;

      set_attr(creature, attr_dodge, 0);
      set_attr(creature, attr_health, 10);
      set_attr(creature, attr_nonbreathing, 1);
      set_attr(creature, attr_speed, 90);
      set_attr(creature, attr_perception, 0);
      set_attr(creature, attr_cold_resistance, +2);
      set_attr(creature, attr_p_open_doors, 1);
      set_attr(creature, attr_poison_resistance, 4);
      set_attr(creature, attr_p_sleep, 1);

      creature->corpse = treasure_corpse;

      if (roll(1, 20) == 1)
	use_or_destroy(creature, random_treasure(item_table_armor));
      else if (roll(1, 10) == 1)
	attach_item_to_creature(creature, random_treasure(item_table_armor));

      if (roll(1, 20) == 1)
	use_or_destroy(creature, random_treasure(item_table_weapon));
      else if (roll(1, 10) == 1)
	attach_item_to_creature(creature, random_treasure(item_table_weapon));

      if (roll(1, 10) == 1)
	attach_item_to_creature(creature, random_treasure(item_table_treasure));

      creature->retreats = false;

      creature->unarmed = virtual_claws;
      break;

    case monster_hellhound:
      set_creature_name(creature, "a hellhound", "the hellhound", "hellhound");
      creature->gent = gent_hellhound;
      creature->parts =
	part_head | part_eyes | part_throat | part_ribs | part_neck |
	part_bowel | part_skull;

      set_attr(creature, attr_health, 10);
      set_attr(creature, attr_speed, 110);
      set_attr(creature, attr_ep_max, 6);
      set_attr(creature, attr_perception, sroll("1d6"));
      set_attr(creature, attr_a_leap, 0);
      set_attr(creature, attr_p_pick_up, 0);
      set_attr(creature, attr_p_read, 1);
      set_attr(creature, attr_carrying_capacity, 0);
      set_attr(creature, attr_m_noxious_breath, 1);

      if (maybe() && maybe())
	creature->corpse = treasure_carcass;

      creature->unarmed = virtual_fangs;
      break;

    case monster_drowned_one:
      set_creature_name(creature, "a drowned one", "the drowned one", "drowned one");
      set_death_msg(creature, "is destroyed!");
      creature->gent = gent_drowned_one;
      creature->parts =
	part_head | part_skull;

      creature->corpse = treasure_corpse;

      set_attr(creature, attr_health, 8);
      set_attr(creature, attr_speed, 60);
      set_attr(creature, attr_cold_resistance, +3);
      set_attr(creature, attr_nonbreathing, 1);
      set_attr(creature, attr_p_open_doors, 1);
      set_attr(creature, attr_p_sleep, 1);

      creature->unarmed = virtual_claws;

      creature->retreats = false;
      break;

    case monster_giant_slimy_toad:
      set_creature_name(creature, "a giant slimy toad", "the giant slimy toad", "giant slimy toad");
      set_death_msg(creature, "is killed!");
      creature->gent = gent_giant_slimy_toad;
      creature->parts =
	part_head | part_eyes | part_throat | part_ribs | part_bowel;

      if (maybe() && maybe())
	creature->corpse = treasure_carcass;

      set_attr(creature, attr_health, 16);
      set_attr(creature, attr_ep_max, 60);
      set_attr(creature, attr_speed, 40);
      set_attr(creature, attr_vision, 2);
      set_attr(creature, attr_p_pick_up, 1);
      set_attr(creature, attr_carrying_capacity, 0);
      set_attr(creature, attr_p_open_doors, 1);
      set_attr(creature, attr_poison_resistance, 4);

      set_attr(creature, attr_m_noxious_breath, 1);

      creature->unarmed = virtual_slime;
      break;

    case monster_goatman:
      set_creature_name(creature, "a goat-man", "the goat-man", "goat-man");
      creature->gent = gent_goatman;
      creature->parts =
	part_head | part_eyes | part_throat | part_ribs | part_neck |
	part_bowel | part_teeth | part_skull;

      if (maybe() && maybe())
	creature->corpse = treasure_carcass;

      set_attr(creature, attr_health, 20);
      set_attr(creature, attr_speed, 60);

      if (roll(1, 4) == 1)
	use_or_destroy(creature, random_treasure(item_table_weapon));

      if (roll(1, 4) == 1)
	use_or_destroy(creature, random_treasure(item_table_weapon));
      
      creature->unarmed = virtual_kick;
      break;

      /* "it consumes all" */
    case monster_flame_spirit:
      set_creature_name(creature, "a flame spirit", "the flame spirit", "flame spirit");
      set_death_msg(creature, "collapses into soot!");
      creature->gent = gent_flame_spirit;

      set_attr(creature, attr_wound_immunity, 1);
      set_attr(creature, attr_poison_resistance, POISON_IMMUNITY);
      set_attr(creature, attr_health, 20);
      set_attr(creature, attr_speed, 60);
      set_attr(creature, attr_nonbreathing, 1);
      set_attr(creature, attr_fire_resistance, +10);
      set_attr(creature, attr_permafloat, 1);
      set_attr(creature, attr_p_pick_up, 1);
      set_attr(creature, attr_carrying_capacity, 0);
      set_attr(creature, attr_p_drink, 1);
      set_attr(creature, attr_p_eat, 1);
      set_attr(creature, attr_p_sleep, 1);

      set_attr(creature, attr_m_breathe_fire, 1);

      creature->unarmed = virtual_flame_hands;
      break;

    case monster_frostling:
      set_creature_name(creature, "a frostling", "the frostling", "frostling");
      set_death_msg(creature, "is destroyed!");
      creature->parts =
	part_head | part_neck | part_skull;

      creature->gent = gent_frostling;

      set_attr(creature, attr_health, 7);
      set_attr(creature, attr_ep_max, 20);
      set_attr(creature, attr_nonbreathing, 1);
      set_attr(creature, attr_magic, 8);
      set_attr(creature, attr_speed, 110);
      set_attr(creature, attr_cold_resistance, 3);

      set_attr(creature, attr_m_frost_ray, 1);

      creature->unarmed = virtual_cold_touch;
      break;
      
    case monster_phantasm:
      set_creature_name(creature, "a phantasm", "the phantasm", "phantasm");
      set_death_msg(creature, "is destroyed!");
/*      creature->parts =
	part_head | part_neck | part_skull;*/

      creature->pacifist = true;

      creature->gent = gent_phantasm;

      set_attr(creature, attr_health, 1);
      set_attr(creature, attr_ep_max, 5);
      set_attr(creature, attr_nonbreathing, 1);
      set_attr(creature, attr_magic, 8);
      set_attr(creature, attr_speed, 90);

      set_attr(creature, attr_m_shock, 1);
      break;
      
    case monster_technician:
      set_creature_name(creature, "a technician", "the technician", "technician");
      set_death_msg(creature, "is destroyed!");
      creature->parts =
	part_head | part_neck | part_skull;

      creature->gent = gent_technician;

      set_attr(creature, attr_health, 5);
      set_attr(creature, attr_ep_max, 5);
      set_attr(creature, attr_speed, 85);
      break;
      
    case monster_sentinel:
      set_creature_name(creature, "a sentinel", "the sentinel", "sentinel");
      set_death_msg(creature, "is destroyed!");
/*      creature->parts =
	part_head | part_neck | part_skull;*/

      creature->gent = gent_sentinel;

      set_attr(creature, attr_health, 40);
      set_attr(creature, attr_ep_max, 40);
      set_attr(creature, attr_nonbreathing, 1);
      set_attr(creature, attr_speed, 120);
      set_attr(creature, attr_cold_resistance, 3);

      set_attr(creature, attr_m_reveal_traps, 1);

      creature->unarmed = virtual_fists;
      break;
      
    case monster_floating_demon_genitalia:
      set_creature_name(creature, "a floating demon genitalia", "the floating demon genitalia", "floating demon genitalia");
      set_death_msg(creature, "is destroyed!");

      creature->gent = gent_f_d_g;

      set_attr(creature, attr_health, 20);
      set_attr(creature, attr_nonbreathing, 1);
      set_attr(creature, attr_speed, 80);
      set_attr(creature, attr_permafloat, 1);

      set_attr(creature, attr_ep_max, 6);

      set_attr(creature, attr_m_sticky_web, 1);

      creature->unarmed = virtual_slap;
      break;
      
    case monster_elder_mummylich:
      set_creature_name(creature, "an elder mummylich", "the elder mummylich", "elder mummylich");
      set_death_msg(creature, "is destroyed!");

      creature->gent = gent_elder_mummylich;

      set_attr(creature, attr_health, 60);
      set_attr(creature, attr_nonbreathing, 1);
      set_attr(creature, attr_speed, 130);

      set_attr(creature, attr_m_blink, 1);
      set_attr(creature, attr_m_frost_ray, 1);
      set_attr(creature, attr_m_fireball, 1);

      creature->unarmed = virtual_cold_touch;
      break;
      
    case monster_silver_wolf:
      set_creature_name(creature, "silver wolf", "bah", "bah");

      creature->gent = gent_wolf;

      set_attr(creature, attr_health, 10);
      set_attr(creature, attr_ep_max, 10);
      set_attr(creature, attr_speed, 7);
      set_attr(creature, attr_cold_resistance, 5);
      set_attr(creature, attr_a_leap, 1);

      creature->unarmed = virtual_fangs;
      break;

    case monster_moon_wolf:
      set_creature_name(creature, "moon wolf", "bah", "bah");

      creature->gent = gent_wolf;

      set_attr(creature, attr_health, 6);
      set_attr(creature, attr_ep_max, 6);
      set_attr(creature, attr_speed, 7);
      set_attr(creature, attr_cold_resistance, 1);
      set_attr(creature, attr_a_leap, 1);

      creature->unarmed = virtual_fangs;
      break;

    case monster_undefined:
    default:
      del_creature(creature);
      return NULL;
    }

  set_attr(creature, attr_ep_current,
	   attr_current(creature, attr_ep_max));

  return creature;
} /* build_monster */



/*
  Tries to attach CREATURE to LEVEL and place it as close as possible
  to {Y, X}. If the creature for some reason can't be attached or
  placed properly, it will be deallocated and false will be
  returned. If no error occured, true is returned.
  
  Caution: Unless you check the return value, make sure all
  initialisation of the creature is done *before* put_or_destroy() is
  called - the pointer isn't guaranteed to be valid afterwards!
*/
blean_t put_or_destroy(level_t * level, creature_t * creature,
		       const unsigned int y, const unsigned int x)
{
  if (creature == NULL)
    return false;

  if ((on_map(level, y, x) == false) ||
      (attach_creature(level, creature) != NULL))
  {
    del_creature(creature);
    return false;
  }
  
  find_nearest_free_spot(creature, y, x);
  
  return true;
} /* put_or_destroy */
