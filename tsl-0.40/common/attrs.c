#include <stdlib.h>
#include <string.h>

#include "main.h"
#include "stuff.h"
#include "attrs.h"
#include "magic.h"
#include "shapeshf.h"
#include "poison.h"
#include "teleport.h"
#include "breath.h"
#include "balls.h"
#include "pushing.h"
#include "game.h"
#include "effect.h"
#include "web.h"


/*
  Builds an attr_info_t structure called NEW_NAME.
*/
attr_info_t * build_attr_info(const char * new_name)
{
  attr_info_t * local;

  if (new_name == NULL)
    return NULL;

  local = malloc(sizeof(attr_info_t));
  if (local == NULL) out_of_memory();
  mem_alloc.attributes++;

  local->name = mydup(new_name);
  if (local->name == NULL) out_of_memory();
  mem_alloc.chars += strlen(local->name) + 1;

  local->max = 10000;
  local->min = 0;
  local->def = 0;
  local->percent = false;
  local->morgue = false;
  local->item_data = false;
  local->inherit = false;
  local->cost = 0;
  local->invoke = NULL;
  local->key = ' ';

  return local;
} /* build_attr_info */



/*
  Frees all attribute info.
*/
void del_all_attr_info()
{
  int i;
    
  for (i = 0; i < ATTRIBUTES; i++)
  {
    if (attr_info[i] != NULL)
    {
      mem_alloc.chars -= strlen(attr_info[i]->name) + 1;
      free(attr_info[i]->name);

      mem_alloc.attributes--;
      free(attr_info[i]);

      attr_info[i] = NULL;
    }
  }

  return;
} /* del_all_attr_info */



/*
  Specifies names and ranges for all creature attributes.
*/
void init_all_attr_info()
{
  attr_info_t * temp;
  int i;

  for (i = 0; i < ATTRIBUTES; i++)
    attr_info[i] = NULL;

  temp = attr_info[attr_killed] = build_attr_info("Killed");
  temp->min = 0;
  temp->max = 1;

  temp = attr_info[attr_carrying_capacity] = build_attr_info("Carrying Capacity");
  temp->min = 0;
  temp->max = 3000;
  temp->def = DEFAULT_CARRYING_CAPACITY;
  temp->item_data = true;

/*  temp = attr_info[attr_fatigue_limit] = build_attr_info("Fatigue Limit");
  temp->item_data = true;
  temp->inherit = true;*/
  /* Default is 0: instant kill */

/*  temp = attr_info[attr_fatigue] = build_attr_info("Fatigue");
  temp->item_data = true;
  temp->inherit = true;*/

  temp = attr_info[attr_health] = build_attr_info("Health");
  temp->item_data = true;
  temp->inherit = true;
  temp->def = 10;
  
  temp = attr_info[attr_ep_current] = build_attr_info("Current EP");
  temp->inherit = true;

  temp = attr_info[attr_ep_max] = build_attr_info("Max EP");
  temp->def = 10;
  temp->item_data = true;
  temp->inherit = true;

  temp = attr_info[attr_damage_total] = build_attr_info("Total Damage Taken");
  temp->min = 0;
  temp->max = 90000;
  temp->morgue = true;
  temp->inherit = true;

  temp = attr_info[attr_gas_immunity] = build_attr_info("Protection from gas attacks");
  temp->item_data = true;

  temp = attr_info[attr_nonbreathing] = build_attr_info("Nonbreathing");

  temp = attr_info[attr_recovery] = build_attr_info("Recovery");

  temp = attr_info[attr_speed] = build_attr_info("Speed");
  temp->min = 1;
  temp->max = 300;
  temp->def = BASE_SPEED;
  temp->morgue = true;
  temp->item_data = true;

  temp = attr_info[attr_swim_fatigue] = build_attr_info("Swim Fatigue");
  temp->inherit = true;

  temp = attr_info[attr_swimming] = build_attr_info("Swimming");
  temp->morgue = true;
  temp->item_data = true;

  temp = attr_info[attr_free_swim] = build_attr_info("Free Swim");

  temp = attr_info[attr_player_ally] = build_attr_info("Player Ally");
  temp->min = 0;
  temp->max = 1;

  temp = attr_info[attr_backstab_bonus] = build_attr_info("Backstab Bonus");
  temp->max = 100;
  temp->percent = true;
  temp->morgue = true;
  temp->item_data = true;

  temp = attr_info[attr_backstab_immunity] = build_attr_info("Backstab Immunity");
  temp->max = 1;

  temp = attr_info[attr_stealth] = build_attr_info("Stealth");
  temp->def = 0;
  temp->morgue = true;
  temp->item_data = true;

  temp = attr_info[attr_perception] = build_attr_info("Perception");
  temp->def = 0;
  temp->morgue = true;
  temp->item_data = true;
  temp->inherit = true;

  temp = attr_info[attr_magic] = build_attr_info("Magic");
  temp->min = 1;
  temp->max = 19;
  temp->def = 0;
  temp->morgue = true;
  temp->item_data = true;

  temp = attr_info[attr_throw_range] = build_attr_info("Throw Range");
  temp->min = BASE_THROW_RANGE;
  temp->def = BASE_THROW_RANGE;
  temp->morgue = true;
  temp->item_data = true;

  temp = attr_info[attr_dodge] = build_attr_info("Dodge");
  temp->min = 0;
  temp->max = 100;
  temp->def = 20;
  temp->morgue = true;
  temp->item_data = true;
  temp->percent = true;

  temp = attr_info[attr_attack] = build_attr_info("Attack");
  temp->min = 0;
  temp->max = 100;
  temp->def = 40;
  temp->morgue = true;
  temp->item_data = true;
  temp->percent = true;

  temp = attr_info[attr_vision] = build_attr_info("Vision");
  temp->def = 1;
  temp->morgue = true;
  temp->item_data = true;

  temp = attr_info[attr_p_pick_up] = build_attr_info("Prevent Pick Up");
  temp->item_data = true;

  temp = attr_info[attr_p_open_doors] = build_attr_info("Prevent Open Doors");
  temp->item_data = true;

  temp = attr_info[attr_p_throw] = build_attr_info("Prevent Throw");
  temp->item_data = true;

  temp = attr_info[attr_p_move] = build_attr_info("Prevent Move");
  temp->item_data = true;

  temp = attr_info[attr_p_eat] = build_attr_info("Prevent Eat");
  temp->item_data = true;

  temp = attr_info[attr_p_drink] = build_attr_info("Prevent Drink");
  temp->item_data = true;

  temp = attr_info[attr_p_invoke] = build_attr_info("Prevent Invoke");
  temp->item_data = true;

  temp = attr_info[attr_p_read] = build_attr_info("Prevent Read");
  temp->item_data = true;

  temp = attr_info[attr_blindness] = build_attr_info("Blindness");
  temp->item_data = true;

  temp = attr_info[attr_levitate] = build_attr_info("Levitate");
  temp->item_data = true;

  temp = attr_info[attr_permafloat] = build_attr_info("Permafloat");

  temp = attr_info[attr_permaswim] = build_attr_info("Permaswim");

  temp = attr_info[attr_unchanging] = build_attr_info("Unchanging");

  temp = attr_info[attr_feet_protected] = build_attr_info("Feet Protected");
  temp->item_data = true;

  temp = attr_info[attr_absorption] = build_attr_info("Absorption");
  temp->morgue = true;
  temp->item_data = true;

  temp = attr_info[attr_fire_resistance] = build_attr_info("Fire Resistance");
  temp->morgue = true;
  temp->item_data = true;

  temp = attr_info[attr_cold_resistance] = build_attr_info("Cold Resistance");
  temp->morgue = true;
  temp->item_data = true;

  temp = attr_info[attr_acid_resistance] = build_attr_info("Acid Resistance");
  temp->morgue = true;
  temp->item_data = true;
  
  temp = attr_info[attr_electricity_resistance] = build_attr_info("Electricity Resistance");
  temp->morgue = true;
  temp->item_data = true;
  
  temp = attr_info[attr_wounded] = build_attr_info("Wounded");

  temp = attr_info[attr_wound_immunity] = build_attr_info("Immunity to wounding");
  temp->item_data = true;

  temp = attr_info[attr_poisoned] = build_attr_info("Poisoned");

  temp = attr_info[attr_poison_resistance] = build_attr_info("Poison Resistance");
  temp->morgue = true;
  temp->item_data = true;

  temp = attr_info[attr_s_trap_detection] = build_attr_info("Trap Detection");
  temp->morgue = true;
  temp->item_data = true;

  temp = attr_info[attr_dualwield] = build_attr_info("Dual-Wield");
  temp->item_data = true;

  temp = attr_info[attr_p_sleep] = build_attr_info("Protection from sleep");
  temp->item_data = true;

  temp = attr_info[attr_i_precision] = build_attr_info("Precision");
  temp->item_data = true;

  temp = attr_info[attr_i_blink] = build_attr_info("Blink");
  temp->max = 100;
  temp->percent = true;

  temp = attr_info[attr_i_drain] = build_attr_info("Drain");
  temp->max = 100;
  temp->percent = true;

  temp = attr_info[attr_i_stun] = build_attr_info("Stun");
  temp->max = 100;
  temp->percent = true;

  temp = attr_info[attr_i_knockback] = build_attr_info("Knockback");
  temp->max = 100;
  temp->percent = true;

  temp = attr_info[attr_i_instadeath] = build_attr_info("Insta-Death");
  temp->max = 100;
  temp->percent = true;

  temp = attr_info[attr_i_wound] = build_attr_info("Wound");
  temp->max = 100;
  temp->percent = true;

  temp = attr_info[attr_i_backstab] = build_attr_info("Backstab");
  temp->max = 100;
  temp->percent = true;

  temp = attr_info[attr_i_poison] = build_attr_info("Poisoned");
  temp->max = 1;

  temp = attr_info[attr_a_leap] = build_attr_info("Leap");
  temp->invoke = &leap;
  temp->cost = 3;

  temp = attr_info[attr_a_shape_native] = build_attr_info("Shapeshift Native");
  temp->invoke = &shapeshift_native;

  temp = attr_info[attr_a_aimed_shot] = build_attr_info("Aimed Shot");
  temp->invoke = &aimed_shot_prepare;
  temp->cost = 2;

  temp = attr_info[attr_a_dash] = build_attr_info("Dash");
  temp->invoke = &dash;

  temp = attr_info[attr_m_cure_poison] = build_attr_info("Cure Poison");
  temp->invoke = &cure_poison;
  temp->cost = 2;

  temp = attr_info[attr_m_destroy_trap] = build_attr_info("Destroy Trap");
  temp->invoke = &destroy_trap;
  temp->cost = 3;
    
  temp = attr_info[attr_m_magic_weapon] = build_attr_info("Magic Weapon");
  temp->invoke = &magic_weapon;
  temp->cost = 3;
    
  temp = attr_info[attr_m_recharge] = build_attr_info("Recharge");
  temp->invoke = &cast_recharge;
  temp->cost = 3;
    
  temp = attr_info[attr_m_deathspell] = build_attr_info("Deathspell");
  temp->invoke = &deathspell;
  temp->cost = 1;
    
  temp = attr_info[attr_m_first_aid] = build_attr_info("First Aid");
  temp->invoke = &first_aid;
  temp->cost = 1;
    
  temp = attr_info[attr_m_sticky_web] = build_attr_info("Sticky Web");
  temp->invoke = &sticky_web;
  temp->cost = 3;
    
  temp = attr_info[attr_m_fireball] = build_attr_info("Fireball");
  temp->invoke = &fireball;
  temp->cost = 3;

  temp = attr_info[attr_m_shock] = build_attr_info("Shock");
  temp->invoke = &shock;
  temp->cost = 3;

  temp = attr_info[attr_m_frost_ray] = build_attr_info("Frost Ray");
  temp->invoke = &frost_ray;
  temp->cost = 4;

  temp = attr_info[attr_m_teleport] = build_attr_info("Teleport");
  temp->invoke = &cast_teleport;
  temp->cost = 8;

  temp = attr_info[attr_m_phase] = build_attr_info("Phase");
  temp->invoke = &phase;
  temp->cost = 3;

  temp = attr_info[attr_a_interface] = build_attr_info("Interface");
  temp->invoke = &datajack;
  temp->cost = 4;

  temp = attr_info[attr_m_reveal_traps] = build_attr_info("Reveal Traps");
  temp->invoke = &reveal_traps;

  temp = attr_info[attr_m_blink] = build_attr_info("Blink");
  temp->invoke = &cast_blink;
  temp->cost = 3;

  temp = attr_info[attr_a_hide] = build_attr_info("Hide");
  temp->invoke = &hide;
  temp->cost = 3;

  temp = attr_info[attr_m_magic_mapping] = build_attr_info("Magic Mapping");
  temp->invoke = &magic_mapping;
  temp->cost = 4;

  temp = attr_info[attr_m_force_bolt] = build_attr_info("Force Bolt");
  temp->invoke = &force_bolt;
  temp->cost = 4;

  temp = attr_info[attr_m_mudball] = build_attr_info("Mudball");
  temp->invoke = &mudball;
  temp->cost = 3;

  temp = attr_info[attr_m_summon_familiar] = build_attr_info("Summon Familiar");
  temp->invoke = &summon_familiar;
  temp->cost = 6;

  temp = attr_info[attr_m_bone_crush] = build_attr_info("Bone Crush");
  temp->invoke = &bone_crush;

  temp = attr_info[attr_m_wish] = build_attr_info("Wish");
  temp->invoke = &wish;

  temp = attr_info[attr_m_enslave] = build_attr_info("Enslave");
  temp->invoke = &enslave;

  temp = attr_info[attr_m_mark] = build_attr_info("Mark");
  temp->invoke = &cast_mark;

  temp = attr_info[attr_m_recall] = build_attr_info("Recall");
  temp->invoke = &cast_recall;

  temp = attr_info[attr_m_identify] = build_attr_info("Identify");
  temp->invoke = &cast_identify;

  temp = attr_info[attr_m_noxious_breath] = build_attr_info("Noxious Breath");
  temp->invoke = &noxious_breath;
  temp->cost = 5;

  temp = attr_info[attr_m_breathe_fire] = build_attr_info("Breathe Fire");
  temp->invoke = &breathe_fire;
  temp->cost = 5;

  temp = attr_info[attr_m_flash] = build_attr_info("Flash");
  temp->invoke = &flash_spell;
  temp->cost = 2;

  temp = attr_info[attr_m_amnesia] = build_attr_info("Amnesia");
  temp->invoke = &amnesia;

  temp = attr_info[attr_m_push] = build_attr_info("Push");
  temp->invoke = &push;
  temp->cost = 3;

  temp = attr_info[attr_m_shapeshift_wolf] = build_attr_info("Shapeshift Wolf");
  temp->invoke = &shapeshift_wolf;
  temp->cost = 10;

  temp = attr_info[attr_dissolve] = build_attr_info("Dissolve");
  temp->invoke = &dissolve_items;

  /* I don't think anything should ever be bound to Q or S, to avoid accidental use of these commands. */
  attr_info[attr_m_first_aid         ]->key = 'a';
  attr_info[attr_m_breathe_fire      ]->key = 'b';
  attr_info[attr_m_cure_poison       ]->key = 'c';
  attr_info[attr_a_dash              ]->key = 'd';
  attr_info[attr_m_frost_ray         ]->key = 'f';
  attr_info[attr_a_hide              ]->key = 'h';
  attr_info[attr_a_interface         ]->key = 'i';
  attr_info[attr_m_magic_mapping     ]->key = 'm';
  attr_info[attr_m_noxious_breath    ]->key = 'n';
  attr_info[attr_m_force_bolt        ]->key = 'o';
  attr_info[attr_m_phase             ]->key = 'p';
  attr_info[attr_m_reveal_traps      ]->key = 'r';
  attr_info[attr_m_shock             ]->key = 's';
  attr_info[attr_m_teleport          ]->key = 't';
  attr_info[attr_m_push              ]->key = 'u';
  attr_info[attr_m_fireball          ]->key = 'x';

  attr_info[attr_a_aimed_shot        ]->key = 'A';
  attr_info[attr_m_blink             ]->key = 'B';
  attr_info[attr_m_recharge          ]->key = 'C';
  attr_info[attr_m_deathspell        ]->key = 'D';
  attr_info[attr_m_flash             ]->key = 'F';
  attr_info[attr_m_bone_crush        ]->key = 'H';
  attr_info[attr_m_identify          ]->key = 'I';
  attr_info[attr_a_leap              ]->key = 'L';
  attr_info[attr_m_magic_weapon      ]->key = 'M';
  attr_info[attr_a_shape_native      ]->key = 'N';
  attr_info[attr_m_destroy_trap      ]->key = 'R';
  attr_info[attr_m_summon_familiar   ]->key = 'S';
  attr_info[attr_m_mudball           ]->key = 'U';
  attr_info[attr_m_mark              ]->key = 'Y';
  attr_info[attr_m_recall            ]->key = 'Z';

  return;
} /* init_all_attr_info */






/*
  Returns the base attribute ATTR_INDEX of CREATURE. For the player
  this will include modifiers from facets since this is what we're
  usually after. See attr_facet() if you absolutely must know the
  facet modifier.
*/
attribute_t attr_base(const creature_t * creature,
		      const attr_index_t attr_index)
{
  signed int temp;
  
  if (creature == NULL ||
      attr_index < 0 ||
      attr_index >= ATTRIBUTES ||
      attr_info[attr_index] == NULL)
  {
    return 0;
  }
  
  temp = creature->attr_base[attr_index];

  if (is_player(creature))
  {
    if (attr_info[attr_index]->inherit)
      temp = game->native->attr_base[attr_index];

    temp += attr_facet(creature, attr_index);
  }
  
  temp = MAX(attr_info[attr_index]->min, temp);
  temp = MIN(attr_info[attr_index]->max, temp);
  
  return temp;
} /* attr_base */



/*
  Returns how much CREATUREs facet modify attribute ATTR_INDEX. This
  only has effect for the player creature, all others always return
  zero.
*/
signed int attr_facet(const creature_t * creature,
		      const attr_index_t attr_index)
{
  signed int temp;
  item_t * facet;

  if (creature == NULL ||
      is_player(creature) == false ||
      attr_index < 0 ||
      attr_index >= ATTRIBUTES ||
      attr_info[attr_index] == NULL)
  { 
    return 0;
  }

  temp = 0;

  /*
    We're dealing with the player creature. Loop through all facets
    the player has and sum their modifiers.
  */
  facet = game->first_facet;
  
  while (facet != NULL)
  {
    temp += facet->attr_mod[attr_index];
    facet = facet->next_item;
  }
  
  return temp;
} /* attr_facet */



/*
  Returns the attribute ATTR_INDEX modifier of CREATURE caused by
  effects (spells, etc). See also effect.h.
*/
signed int attr_fx_mods(const creature_t * creature,
			const attr_index_t attr_index)
{
  signed int ret;
  effect_t * effect;
  
  if (creature == NULL)
    return 0;
  
  ret = 0;
  
  /*
    Effects are stored in a single-linked list; just traverse it and
    sum whatever modifiers we encounter.
  */
  for (effect = creature->first_effect;
       effect != NULL;
       effect = effect->next_effect)
  {
    ret += effect->attr_mod[attr_index];
  }
  
  return ret;
} /* attr_fx_mods */



/*
  Returns the attribute ATTR_INDEX modifier of CREATUREs equipment. If
  DISCRIMINATE is true, only items that are known will be included.
*/
signed int attr_eq_mods(const creature_t * creature,
			const attr_index_t attr_index,
			const blean_t discriminate)
{
  signed int ret;
  item_t * item;

  ret = 0;

  if (creature == NULL)
    return ret;

  /*
    Items are stored in a single-linked list; just traverse it and sum
    whatever modifiers we encounter.
  */
  for (item = creature->first_item; item != NULL; item = item->next_item)
  {
    if (item->equipped == true)
    {
      /* Skip unidentified items */
      if (discriminate &&
	  (identified(item) & known_name) == 0)
      {
	continue;
      }

      ret += get_item_mod(item, attr_index);
    }
  }

  return ret;
} /* attr_eq_mods */



/*
  Returns the current (base + mods) attribute ATTR_INDEX of
  CREATURE. This is the "effective" value of the attribute that should
  be used for rolls, etc, but it should never be visible to the player
  since it includes *temporary* effects and we wish to keep the exact
  impact of these invisible for an unspoiled player.
*/
attribute_t attr_current(const creature_t * creature,
			 const attr_index_t attr_index)
{
  signed int temp;

  if (creature == NULL ||
      attr_index < 0 ||
      attr_index >= ATTRIBUTES ||
      attr_info[attr_index] == NULL)
  {
    return 0;
  }
  
  temp =
    attr_base(creature, attr_index) +
    attr_fx_mods(creature, attr_index) +
    attr_eq_mods(creature, attr_index, false);
  
  temp = MAX(attr_info[attr_index]->min, temp);
  temp = MIN(attr_info[attr_index]->max, temp);
  temp = MAX(0, temp);
  
  return temp;
} /* attr_current */



/*
  Returns the current ATTR_INDEX score for CREATURE, but include only
  modifiers that are known (e.g. identified equipment). This is not the
  effective score.
*/
attribute_t attr_known(const creature_t * creature,
		       const attr_index_t attr_index)
{
  signed int temp;

  if (creature == NULL ||
      attr_index < 0 ||
      attr_index >= ATTRIBUTES ||
      attr_info[attr_index] == NULL)
  {
    return 0;
  }
  
  temp =
    attr_base(creature, attr_index) +
    attr_fx_mods(creature, attr_index) +
    attr_eq_mods(creature, attr_index, true);
  
  temp = MAX(attr_info[attr_index]->min, temp);
  temp = MIN(attr_info[attr_index]->max, temp);
  temp = MAX(0, temp);
  
  return temp;
} /* attr_known */



/*
  Sets the base attribute ATTR_INDEX of CREATURE to VALUE. The base
  value is the only one that can be set, the others are derived values.
*/
void set_attr(creature_t * creature,
	      const attr_index_t attr_index,
	      const attribute_t value)
{
  if (creature == NULL ||
      attr_index < 0 ||
      attr_index >= ATTRIBUTES ||
      attr_info[attr_index] == NULL)
  {
    return;
  }
  
  if (is_player(creature) &&
      attr_info[attr_index]->inherit)
  {
    game->native->attr_base[attr_index] = value;
  }
  else
  {
    creature->attr_base[attr_index] = value;
  }
  
  return;
} /* set_attr_base */
