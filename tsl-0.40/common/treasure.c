#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "main.h"
#include "stuff.h"
#include "treasure.h"
#include "magic.h"
#include "level.h"
#include "rndnames.h"
#include "game.h"
#include "ability.h"



/*
  Builds an item whose singular (id or un-id) name (ignoring case)
  contains the substring NAME. If several matches exist one is chosen
  at random. This function won't create items that wouldn't appear
  naturally in the world otherwise (spells) or those marked as no_wish
  - use build_item(enum) directly to create these. If ARTIFACTS is
  true, items marked with item_t->artifact will be ignored.
*/
item_t * build_any_item(const char * name, const blean_t artifacts)
{
  unsigned int * match;
  unsigned int matches;
  unsigned int i;
  item_t * item;

  if (name == NULL)
    return NULL;

  /* We start out with an empty list of matches. */
  matches = 0;
  match = NULL;

  /* For each item in the item database... */
  for (i = 0; i < ITEMS; i++)
  {
    if (game->item_template[i] == NULL)
      continue;

    /*
      Some items we aren't allowed to create - call these by enum if
      they are needed.
    */
    if (game->item_template[i]->no_wish ||
	game->item_template[i]->item_type == item_type_spell ||
	(artifacts == false && game->item_template[i]->artifact))
      continue;

    /*
      Does any name match?
    */
    if (wishcmp(game->item_template[i]->single_unid_name, name) ||
	wishcmp(game->item_template[i]->single_id_name, name))
    {
      /* Expand the list of matches */
      matches++;
      match = realloc(match, sizeof(int) * matches);
      if (match == NULL) out_of_memory();
	
      /* Put the index of this item in the last slot. */
      match[matches - 1] = i;
    }
  }

  item = NULL;

  /*
    Try to build a random item - only legal ones should be posssible.
  */
  if (match != NULL)
  {
    item = build_item(match[tslrnd() % matches]);
    free(match);
  }

  return item;
} /* build_any_item */






/*
  Sets up the database of items for game G.
*/
void init_item_templates(game_t * g)
{
  unsigned int i;

  if (g == NULL)
    return;

  /* First, empty the array that will hold them. */
  for (i = 0; i < ITEMS; i++)
    g->item_template[i] = NULL;

  /*
    Fill the database with all available items in the game.
  */
  for (i = 0; i < ITEMS; i++)
  {
    g->item_template[i] = build_item_template(g, i);

    if (g->item_template[i] == NULL)
      continue;

    /* Disable random artifacts */
/*    if (g->item_template[i]->artifact &&
	(roll(1, 4) == 1))
    {
      del_item(g->item_template[i]);
      g->item_template[i] = NULL;
      }*/
  }

  return;
} /* init_item_templates */



/*
  Deletes all item in the item database for game G.
*/
void del_item_templates(game_t * g)
{
  unsigned int i;

  if (g == NULL)
    return;

  for (i = 0; i < ITEMS; i++)
  {
    if (g->item_template[i] != NULL)
      del_item(g->item_template[i]);
    
    g->item_template[i] = NULL;
  }

  return;
} /* del_item_templates */



/*
  Builds a "base item" of item# ITEM.
*/
item_t * build_item_template(game_t * g, const treasure_t item)
{
  item_t * local;
  char temp[800];
  
  local = alloc_item();

  local->item_number = item;
  
  switch (item)
  {
    case treasure_cranium:
      set_item_single_name(local, "cranium", "cranium");
      set_item_plural_name(local, "craniums", "craniums");
      local->gent = gent_cranium;
      local->id |= known_name;
      local->item_type = item_type_misc;
      local->weight = 9;
      break;
      
    case treasure_r_bone_dust:
      set_item_single_name(local, "pinch of bone dust", "pinch of bone dust");
      set_item_plural_name(local, "pinches of bone dust", "pinches of bone dust");
      local->gent = gent_bone_dust;
      local->id |= known_name;
      local->item_type = item_type_misc;
      local->weight = 1;
      break;
      
    case treasure_r_mandrake_root:
      set_item_single_name(local, "root", "mandrake root");
      set_item_plural_name(local, "roots", "mandrake roots");
      local->gent = gent_mandrake_root;
      local->item_type = item_type_misc;
      local->id |= known_name;
      local->weight = 1;
      break;
      
    case treasure_r_mushroom:
      set_item_single_name(local, "mushroom", "mushroom");
      set_item_plural_name(local, "mushrooms", "mushrooms");
      local->gent = gent_mushroom;
      local->item_type = item_type_food;
      local->edible = true;
      local->id |= known_name;
      local->weight = WEIGHT_FOOD;
      break;
      
    case treasure_severed_hand:
      set_item_single_name(local, "severed hand", "severed hand");
      set_item_plural_name(local, "severed hands", "severed hands");
      local->gent = gent_severed_hand;
      local->id |= known_name;
      local->item_type = item_type_food;
      local->edible = true;
      local->weight = WEIGHT_FOOD * 2;
      break;
      
    case treasure_decapitated_head:
      set_item_single_name(local, "decapitated head", "decapitated head");
      set_item_plural_name(local, "decapitated heads", "decapitated heads");
      local->gent = gent_corpse;
      local->id |= known_name;
      local->item_type = item_type_food;
      local->edible = true;
      local->weight = WEIGHT_FOOD * 2;
      break;
      
    case treasure_corpse:
      set_item_single_name(local, "corpse", "corpse");
      set_item_plural_name(local, "corpses", "corpses");
      set_item_description(local, "Whoever this was has seen better days.");
      local->gent = gent_corpse;
      local->id |= known_name;
      local->item_type = item_type_food;
      local->edible = true;
      local->weight = WEIGHT_CORPSE;
      break;
      
    case treasure_ogre_corpse:
      set_item_single_name(local, "ogre corpse", "ogre corpse");
      set_item_plural_name(local, "ogre corpses", "ogre corpses");
      local->single_id_article = local->single_unid_article = article_i_an;
      local->gent = gent_ogre_corpse;
      local->id |= known_name;
      local->item_type = item_type_food;
      local->edible = true;
      local->weight = WEIGHT_CORPSE * 3;
      break;
      
    case treasure_carcass:
      set_item_single_name(local, "carcass", "carcass");
      set_item_plural_name(local, "carcasses", "carcasses");
      /*set_item_description(local, "Whoever this was has seen better days.");*/
      local->gent = gent_carcass;
      local->id |= known_name;
      local->item_type = item_type_food;
      local->edible = true;
      local->weight = WEIGHT_CORPSE;
      break;
      
    case treasure_bread:
      set_item_single_name(local, "loaf of bread", "loaf of bread");
      set_item_plural_name(local, "loaves of bread", "loaves of bread");
      local->gent = gent_bread;
      local->id |= known_name;
      local->item_type = item_type_food;
      local->edible = true;
      local->weight = WEIGHT_FOOD;
      break;
      
    case treasure_meat:
      set_item_single_name(local, "slab of raw meat", "slab of raw meat");
      set_item_plural_name(local, "slabs of raw meat", "slabs of raw meat");
      local->gent = gent_meat;
      local->id |= known_name;
      local->item_type = item_type_food;
      local->edible = true;
      local->weight = WEIGHT_FOOD;
      break;
       
    case treasure_fish:
      set_item_single_name(local, "smelly fish", "smelly fish");
      set_item_plural_name(local, "smelly fishes", "smelly fishes");
      local->gent = gent_fish;
      local->id |= known_name;
      local->item_type = item_type_food;
      local->edible = true;
      local->weight = WEIGHT_FOOD;
      break;
      
    case treasure_cheese:
      set_item_single_name(local, "piece of cheese", "piece of cheese");
      set_item_plural_name(local, "pieces of cheese", "pieces of cheese");
      local->gent = gent_cheese;
      local->id |= known_name;
      local->item_type = item_type_food;
      local->edible = true;
      local->weight = WEIGHT_FOOD;
      break;

    case treasure_chickpeas:
      set_item_single_name(local, "handful of chickpeas", "handful of chickpeas");
      set_item_plural_name(local, "handfuls of chickpeas", "handfuls of chickpeas");
      local->gent = gent_chickpeas;
      local->id |= known_name;
      local->item_type = item_type_food;
      local->edible = true;
      local->weight = WEIGHT_FOOD;
      break;

    case treasure_falafel:
      set_item_single_name(local, "falafel", "falafel");
      set_item_plural_name(local, "falafel", "falafel");
      local->gent = gent_falafel;
      local->id |= known_name;
      local->item_type = item_type_food;
      local->edible = true;
      local->weight = WEIGHT_FOOD;
      break;
      
    case treasure_sausage:
      set_item_single_name(local, "sausage", "sausage");
      set_item_plural_name(local, "sausages", "sausages");
      local->gent = gent_sausage;
      local->id |= known_name;
      local->item_type = item_type_food;
      local->edible = true;
      local->weight = WEIGHT_FOOD;
      break;
      
    case treasure_r_eyeball:
      set_item_single_name(local, "eyeball", "eyeball");
      set_item_plural_name(local, "eyeballs", "eyeballs");
      local->gent = gent_eyeball;
      local->single_id_article = local->single_unid_article = article_i_an;
      local->id |= known_name;
      local->item_type = item_type_misc;
      local->weight = WEIGHT_FOOD;
      break;
      
    case treasure_dataprobe:
      set_item_single_name(local, "dataprobe", "dataprobe");
      set_item_plural_name(local, "dataprobes", "dataprobes");
      local->gent = gent_dataprobe;
      local->id |= known_name;
      local->item_type = item_type_tool;
      /*local->prohibit_stacking = true;*/
      local->weight = WEIGHT_DATAPROBE;
      break;
      
    case treasure_key:
      set_item_single_name(local, "key", "key");
      set_item_plural_name(local, "keys", "keys");
      local->gent = gent_key;
      local->id |= known_name;
      local->item_type = item_type_tool;
      /*local->prohibit_stacking = true;*/
      local->weight = WEIGHT_KEY;
      break;
      
    case treasure_torch:
      set_item_single_name(local, "torch", "torch");
      local->gent = gent_torch;
      local->id |= known_name;
      local->item_type = item_type_light;
      local->attr_mod[attr_vision] = +3;
      local->custom[CUSTOM_LIGHT_TICKS] = TORCH_LIFETIME;
      local->weight = WEIGHT_TOOL;
      break;
      
    case treasure_lantern:
      set_item_single_name(local, "lantern", "lantern");
      local->gent = gent_lantern;
      local->id |= known_name;
      local->item_type = item_type_light;
      local->attr_mod[attr_vision] = +4;
      local->custom[WEAPON_MIN_DAMAGE] = 5;
      local->custom[WEAPON_MAX_DAMAGE] = 10;
      local->custom[CUSTOM_LIGHT_TICKS] = LANTERN_LIFETIME;
      local->weight = WEIGHT_TOOL;
      break;
      
    case treasure_dark_cloak:
      set_item_single_name(local, "cloak", "dark cloak");
      set_item_description(local, "It seems to absorb light and sound.");
      local->gent = gent_cloak;
      local->item_type = item_type_cloak;
      local->attr_mod[attr_stealth] = +2;
      local->weight = WEIGHT_CLOAK;
      break;

    case treasure_wool_cloak:
      set_item_single_name(local, "cloak", "wool cloak");
      set_item_description(local, "It keeps you warm.");
      local->gent = gent_cloak;
      local->item_type = item_type_cloak;
      local->attr_mod[attr_cold_resistance] = +2;
      local->weight = WEIGHT_CLOAK;
      break;

    case treasure_seaweed_cloak:
      set_item_single_name(local, "cloak", "seaweed cloak");
      local->gent = gent_cloak;
      local->item_type = item_type_cloak;
      local->attr_mod[attr_speed] = +20;
      local->weight = WEIGHT_CLOAK;
      break;

    case treasure_lab_coat:
      set_item_single_name(local, "cloak", "lab coat");
      set_item_description(local, "It is intended to protect against corrosive substances.");
      local->auto_id = true;
      local->gent = gent_robe;
      local->item_type = item_type_cloak;
      local->attr_mod[attr_acid_resistance] = +2;
      local->weight = WEIGHT_CLOAK;
      break;

    case treasure_fur_pelt:
      set_item_single_name(local, "cloak", "fur pelt");
      set_item_description(local, "The unprocessed hide of an animal. It provides minimal protection against weather.");
      local->gent = gent_cloak;
      local->item_type = item_type_cloak;
      local->attr_mod[attr_cold_resistance] = +2;
      local->attr_mod[attr_swimming] = -1;
      local->weight = WEIGHT_CLOAK;
      break;

    case treasure_filthy_rags:
      set_item_single_name(local, "filthy rags", "filthy rags");
      set_item_description(local, "Why would anyone want to wear these?");
      local->id |= known_name;
      local->gent = gent_light_armor;
      local->single_id_article = local->single_unid_article = article_i_some_are;
      local->custom[ARMOR_PROTECTION] = 1;
      local->custom[ARMOR_MAX_DURABILITY] = 30;
      local->item_type = item_type_body;
      local->weight = 7;
      break;

    case treasure_leather:
      set_item_single_name(local, "armor", "leather jacket");
      set_item_description(local, "A garment made of thick, riveted leather.");
      local->id |= known_name;
      local->single_id_article = article_i_a;
      local->single_unid_article = article_i_a_set_of;
      local->gent = gent_light_armor;
      local->item_type = item_type_body;
      local->custom[ARMOR_PROTECTION] = 1;
      local->custom[ARMOR_MAX_DURABILITY] = 70;
      local->weight = WEIGHT_LIGHT_ARMOR;
      break;

    case treasure_chainmail:
      set_item_single_name(local, "armor", "chainmail hauberk");
      set_item_description(local, "It is heavy and noisy, but provides good protection. 100% metal, wash separately.");
      local->id |= known_name;
      local->single_id_article = article_i_a;
      local->single_unid_article = article_i_a_set_of;
      local->gent = gent_heavy_armor;
      local->item_type = item_type_body;
      local->custom[ARMOR_PROTECTION] = 2;
      local->custom[ARMOR_MAX_DURABILITY] = 90;
      local->attr_mod[attr_stealth] = -1;
      local->attr_mod[attr_swimming] = -1;
      local->attr_mod[attr_speed] = -10;
      local->weight = WEIGHT_HEAVY_ARMOR;
      break;

    case treasure_rune_armor:
      set_item_single_name(local, "armor", "rune armor");
      local->gent = gent_heavy_armor;
      local->single_id_article = local->single_unid_article = article_i_a_set_of;
      local->item_type = item_type_body;
      local->custom[ARMOR_PROTECTION] = 5;
      local->custom[ARMOR_MAX_DURABILITY] = 400;
      local->attr_mod[attr_speed] = +20;
      local->weight = WEIGHT_HEAVY_ARMOR;
      break;

    case treasure_flippers:
      set_item_single_name(local, "boots", "lizardskin flippers");
      local->single_id_article = local->single_unid_article = article_i_a_pair_of;
      local->gent = gent_boots;
      local->item_type = item_type_feet;
      local->attr_mod[attr_feet_protected] = 1;
      local->attr_mod[attr_swimming] = +3;
      local->weight = WEIGHT_BOOTS;
      break;

    case treasure_padded_boots:
      set_item_single_name(local, "boots", "padded boots");
      set_item_description(local, "They let you move silently.");
      local->single_id_article = local->single_unid_article = article_i_a_pair_of;
      local->gent = gent_boots;
      local->item_type = item_type_feet;
      local->attr_mod[attr_stealth] = +2;
      local->attr_mod[attr_feet_protected] = 1;
      local->weight = WEIGHT_BOOTS;
      break;

    case treasure_lead_boots:
      set_item_single_name(local, "boots", "lead boots");
      local->single_id_article = local->single_unid_article = article_i_a_pair_of;
      local->gent = gent_boots;
      local->item_type = item_type_feet;
/*      local->cursed = true;*/
      local->attr_mod[attr_speed] = -50;
      local->attr_mod[attr_swimming] = -2;
      local->attr_mod[attr_stealth] = -4;
      local->attr_mod[attr_feet_protected] = 1;
      local->weight = WEIGHT_BOOTS * 5;
      break;

    case treasure_fur_boots:
      set_item_single_name(local, "boots", "fur boots");
      local->gent = gent_boots;
      local->single_id_article = local->single_unid_article = article_i_a_pair_of;
      local->item_type = item_type_feet;
      local->attr_mod[attr_cold_resistance] = +2;
      local->attr_mod[attr_feet_protected] = 1;
      local->weight = WEIGHT_BOOTS;
      break;

    case treasure_boots_of_speed:
      set_item_single_name(local, "boots", "boots of speed");
      set_item_description(local, "A pair of fine-quality leather boots. Maybe they were discarded after someone did accidentaly pee in them. Who knows?");
      local->gent = gent_boots;
      local->single_unid_article = article_i_a_pair_of;
      local->single_id_article = article_i_a_pair_of;
      local->item_type = item_type_feet;
      local->attr_mod[attr_speed] = +20;
      local->attr_mod[attr_feet_protected] = 1;
      local->weight = WEIGHT_BOOTS;
      break;

    case treasure_blindfold:
      set_item_single_name(local, "blindfold", "blindfold");
      set_item_description(local, "It obscures your vision, to the point where you cannot see anything at all.");
      local->gent = gent_goggles;
      local->id |= known_name;
      local->item_type = item_type_head;
      local->attr_mod[attr_blindness] = 1;
      local->weight = WEIGHT_TOOL;
      break;

    case treasure_gas_mask:
      set_item_single_name(local, "mask", "gas mask");
      local->gent = gent_gas_mask;
      local->id |= known_name;
      local->item_type = item_type_head;
      local->attr_mod[attr_gas_immunity] = 1;
      local->attr_mod[attr_p_eat] = 1;
      local->attr_mod[attr_p_drink] = 1;
      local->weight = WEIGHT_HAT;
      break;

/*    case treasure_w_enslavement:
      set_item_single_name(local, get_wand_name(local->item_number), "wand of enslavement");
      local->gent = gent_wand;
      local->item_type = item_type_wand;
      local->invoke_power = attr_m_enslave;
      local->charges = 5;
      local->rechargeable = true;
      local->weight = WEIGHT_WAND;
      break;*/

    case treasure_electric_prod:
      set_item_single_name(local, "electric prod", "electric prod");
      local->single_id_article = article_i_an;
      local->single_unid_article = article_i_an;
      local->id |= known_name;
      local->gent = gent_prod;
      local->item_type = item_type_tool;
      local->invoke_power = attr_m_shock;
      local->charges = 12;
      local->rechargeable = true;
      local->weight = WEIGHT_WAND;
      break;

    case treasure_fire_extinguisher:
      set_item_single_name(local, "fire extinguisher", "fire extinguisher");
      set_item_description(local, "Contains a pressurized mixture of foam and gas that quickly cools and extinguishes fire.");
      local->id |= known_name;
      local->gent = gent_fire_extinguisher;
      local->item_type = item_type_tool;
      local->invoke_power = attr_m_frost_ray;
      local->charges = 12;
      local->rechargeable = false;
      local->weight = WEIGHT_WAND;
      break;

    case treasure_w_phasing:
      set_item_single_name(local, get_wand_name(local->item_number), "wand of phasing");
      explain_ability(temp, attr_m_phase);
      set_item_description(local, temp);
      local->gent = gent_wand;
      local->item_type = item_type_wand;
      local->invoke_power = attr_m_phase;
      local->charges = 50;
      local->rechargeable = true;
      local->weight = WEIGHT_WAND;
      break;

    case treasure_w_pushing:
      set_item_single_name(local, get_wand_name(local->item_number), "wand of pushing");
      explain_ability(temp, attr_m_push);
      set_item_description(local, temp);
      local->gent = gent_wand;
      local->item_type = item_type_wand;
      local->invoke_power = attr_m_push;
      local->charges = 25;
      local->rechargeable = true;
      local->weight = WEIGHT_WAND;
      break;

    case treasure_w_destroy_trap:
      set_item_single_name(local, get_wand_name(local->item_number), "wand of Destroy Trap");
      explain_ability(temp, attr_m_destroy_trap);
      set_item_description(local, temp);
      local->gent = gent_wand;
      local->item_type = item_type_wand;
      local->invoke_power = attr_m_destroy_trap;
      local->charges = 20;
      local->rechargeable = true;
      local->weight = WEIGHT_WAND;
      break;

    case treasure_w_fireball:
      set_item_single_name(local, get_wand_name(local->item_number), "wand of Fireball");
      explain_ability(temp, attr_m_fireball);
      set_item_description(local, temp);
      local->gent = gent_wand;
      local->item_type = item_type_wand;
      local->invoke_power = attr_m_fireball;
      local->charges = 30;
      local->rechargeable = true;
      local->weight = WEIGHT_WAND;
      break;

    case treasure_w_force_bolt:
      set_item_single_name(local, get_wand_name(local->item_number), "wand of Force Bolt");
      explain_ability(temp, attr_m_force_bolt);
      set_item_description(local, temp);
      local->gent = gent_wand;
      local->item_type = item_type_wand;
      local->invoke_power = attr_m_force_bolt;
      local->charges = 40;
      local->rechargeable = true;
      local->weight = WEIGHT_WAND;
      break;

    case treasure_w_frost_ray:
      set_item_single_name(local, get_wand_name(local->item_number), "wand of Frost Ray");
      explain_ability(temp, attr_m_frost_ray);
      set_item_description(local, temp);
      local->gent = gent_wand;
      local->item_type = item_type_wand;
      local->invoke_power = attr_m_frost_ray;
      local->charges = 30;
      local->rechargeable = true;
      local->weight = WEIGHT_WAND;
      break;

    case treasure_doomblade:
      set_item_single_name(local, "Doomblade", "Doomblade");
      local->gent = gent_sword;
      local->no_wish = true;
      local->indestructible = true;
      local->single_id_article = article_i_none;
      local->single_unid_article = article_i_none;
      local->item_type = item_type_m_weapon;
      local->attr_mod[attr_i_drain] = 50;
      local->custom[WEAPON_DAMAGE_TYPE] = damage_general;
      local->custom[WEAPON_FINISH] = gore_cut;
      local->custom[WEAPON_FINISH_ALT] = gore_impale;

      MKATKSEQ(local, -2, -2, -1, 5, 6, 7, 8, 9, 0, 0)

      break;

/*    case treasure_hungry_book:
      set_item_single_name(local, "hungry book", "hungry book");
      local->id |= known_name;
      local->gent = gent_book;
      local->no_wish = true;
      local->indestructible = true;
      local->single_id_article = article_i_none;
      local->single_unid_article = article_i_none;
      local->item_type = item_type_m_weapon;
      local->custom[WEAPON_DAMAGE_TYPE] = damage_general;

      MKATKSEQ(temp, -2, -2, -1, 5, 6, 7, 8, 9, 0, 0)
      break;*/

    case treasure_plain_boring_sword:
      set_item_single_name(local, "sword", "plain boring sword");
      set_item_description(local, "It's a sword.");
      local->id |= known_name;
      local->gent = gent_sword;
      local->item_type = item_type_m_weapon;
      local->custom[WEAPON_DAMAGE_TYPE] = damage_general;
      local->custom[WEAPON_FINISH] = gore_cut;
      local->custom[WEAPON_FINISH_ALT] = gore_impale;
      local->weight = WEIGHT_SWORD_SMALL;

      MKATKSEQ(local, -2, -2, -1, 2, 5, 0, 0, 0, 0, 0)

      break;

    case treasure_crystal_sword:
      set_item_single_name(local, "sword", "crystal sword");
      local->id |= known_name;
      local->gent = gent_sword;
      local->item_type = item_type_m_weapon;
      local->custom[WEAPON_DAMAGE_TYPE] = damage_general;
      local->custom[WEAPON_BREAK] = 600;
      local->custom[WEAPON_FINISH] = gore_cut;
      local->custom[WEAPON_FINISH_ALT] = gore_impale;
      local->weight = WEIGHT_SWORD_SMALL;

      MKATKSEQ(local, -2, -2, -1, 2, 5, 0, 0, 0, 0, 0)

      break;

    case treasure_cleaver:
      set_item_single_name(local, "blood-stained axe", "cleaver");
      set_item_description(local, "It appears to have been brought straight from the kitchen.");
      local->id |= known_name;
      local->gent = gent_axe;
      local->item_type = item_type_m_weapon;
      local->custom[WEAPON_DAMAGE_TYPE] = damage_general;
      local->custom[WEAPON_FINISH] = gore_crush;
      local->weight = WEIGHT_AXE;

      MKATKSEQ(local, -2, -2, -1, 3, 2, 0, 0, 0, 0, 0)

      break;

    case treasure_crowbar:
      set_item_single_name(local, "crowbar", "crowbar");
      local->id |= known_name;
      local->gent = gent_club;
      local->item_type = item_type_m_weapon;
      local->custom[WEAPON_DAMAGE_TYPE] = damage_general;
      local->custom[WEAPON_FINISH] = gore_crush;
      local->weight = WEIGHT_MACE;

      MKATKSEQ(local, -2, -1, -1, 4, 0, 0, 0, 0, 0, 0)

      break;

    case treasure_broken_bottle:
      set_item_single_name(local, "broken bottle", "broken bottle");
      local->id |= known_name;
      local->gent = gent_club;
      local->item_type = item_type_m_weapon;
      local->attr_mod[attr_i_wound] = 50;
      local->custom[WEAPON_DAMAGE_TYPE] = damage_general;
      local->custom[WEAPON_FINISH] = gore_cut;
      local->weight = WEIGHT_MACE;

      MKATKSEQ(local, -2, -2, -1, 2, 1, 1, 0, 0, 0, 0)

      break;

    case treasure_stiletto:
      set_item_single_name(local, "stiletto", "stiletto");
      local->id |= known_name;
      local->gent = gent_sword;
      local->item_type = item_type_m_weapon;
      local->attr_mod[attr_i_backstab] = 30;
      local->custom[WEAPON_DAMAGE_TYPE] = damage_general;
      local->custom[WEAPON_FINISH] = gore_stab;
      local->custom[WEAPON_FINISH_ALT] = gore_cut;
      local->weight = WEIGHT_DAGGER;

      MKATKSEQ(local, -2, -2, -1, 2, 4, 6, 0, 0, 0, 0)

      break;

    case treasure_steel_pipe:
      set_item_single_name(local, "steel pipe", "steel pipe");
      local->id |= known_name;
      local->gent = gent_club;
      local->item_type = item_type_m_weapon;
      local->attr_mod[attr_i_stun] = 20;
      local->custom[WEAPON_DAMAGE_TYPE] = damage_general;
      local->custom[WEAPON_FINISH] = gore_crush;
      local->weight = WEIGHT_MACE;

      MKATKSEQ(local, -2, -1, -1, 3, 3, 3, 3, 0, 0, 0)

      break;

    case treasure_bone:
      set_item_single_name(local, "bone", "bone");
      local->id |= known_name;
      local->gent = gent_club;
      local->item_type = item_type_m_weapon;
      local->attr_mod[attr_i_stun] = 20;
      local->custom[WEAPON_DAMAGE_TYPE] = damage_general;
      local->weight = WEIGHT_CLUB;

      MKATKSEQ(local, -2, -1, -1, 2, 2, 2, 6, 0, 0, 0)

      break;

    case treasure_quarterstaff:
      set_item_single_name(local, "staff", "quarterstaff");
      local->id |= known_name;
      local->gent = gent_staff;
      local->item_type = item_type_m_weapon;
      local->custom[WEAPON_DAMAGE_TYPE] = damage_general;
      local->custom[WEAPON_TWOHANDED] = true;
      local->custom[WEAPON_FINISH] = gore_crush;
      local->attr_mod[attr_i_stun] = 10;
      local->weight = WEIGHT_STAFF;

      MKATKSEQ(local, -2, -1, -1, 2, 2, 2, 6, 0, 0, 0)

      break;

    case treasure_iron_shod_staff:
      set_item_single_name(local, "staff", "iron-shod staff");
      local->id |= known_name;
      local->gent = gent_staff;
      local->single_id_article = article_i_an;
      local->item_type = item_type_m_weapon;
      local->custom[WEAPON_DAMAGE_TYPE] = damage_general;
      local->custom[WEAPON_TWOHANDED] = true;
      local->custom[WEAPON_FINISH] = gore_crush;
      local->attr_mod[attr_i_stun] = 20;
      local->weight = WEIGHT_STAFF;

      MKATKSEQ(local, -2, -1, -1, 4, 5, 0, 0, 0, 0, 0)

      break;

    case treasure_spear:
      set_item_single_name(local, "pole", "wooden spear");
      set_item_description(local, "A pole with a sharp tip. Usage: Point at foe, apply pressure.");
      local->id |= known_name;
      local->gent = gent_spear;
      local->item_type = item_type_m_weapon;
      local->custom[WEAPON_DAMAGE_TYPE] = damage_general;
      local->custom[WEAPON_FINISH] = gore_impale;
      local->custom[WEAPON_FINISH_ALT] = gore_stab;
      local->weight = WEIGHT_STAFF;

      MKATKSEQ(local, -2, -1, -1, 4, 0, 0, 0, 0, 0, 0)

      break;

    case treasure_wooden_stick:
      set_item_single_name(local, "pole", "wooden stick");
      set_item_description(local, "");
      local->id |= known_name;
      local->gent = gent_spear;
      local->item_type = item_type_m_weapon;
      local->custom[WEAPON_DAMAGE_TYPE] = damage_general;
      local->custom[WEAPON_FINISH] = gore_impale;
      local->custom[WEAPON_FINISH_ALT] = gore_stab;
      local->weight = WEIGHT_STAFF;

      MKATKSEQ(local, -2, -2, -1, 2, 3, 0, 0, 0, 0, 0)

      break;

    case treasure_trident:
      set_item_single_name(local, "pole", "trident");
      local->gent = gent_spear;
      local->id |= known_name;
      local->item_type = item_type_m_weapon;
      local->custom[WEAPON_DAMAGE_TYPE] = damage_general;
      local->custom[WEAPON_FINISH] = gore_impale;
      local->custom[WEAPON_FINISH_ALT] = gore_stab;
      local->weight = WEIGHT_STAFF;

      MKATKSEQ(local, -2, -2, -1, 3, 3, 3, 0, 0, 0, 0)

      break;

    case treasure_fish_scale_armor:
      set_item_single_name(local, "armor", "fish scale armor");
      local->id |= known_name;
      local->gent = gent_light_armor;
      local->single_id_article = local->single_unid_article = article_i_a_set_of;
      local->item_type = item_type_body;
      local->custom[ARMOR_PROTECTION] = 1;
      local->custom[ARMOR_MAX_DURABILITY] = 3;
      local->attr_mod[attr_swimming] = +1;
      local->weight = WEIGHT_LIGHT_ARMOR;
      break;

    case treasure_big_ugly_knife:
      set_item_single_name(local, "big ugly knife", "big ugly knife");
      set_item_description(local, "A long knife, crooked and mean-looking. In the right hands, it can do wonders. In the wrong hands, it will gut you instead.");
      local->id |= known_name;
      local->gent = gent_sword;
      local->item_type = item_type_m_weapon;
      local->attr_mod[attr_i_wound] = 20;

      local->weight = WEIGHT_DAGGER;

      local->custom[WEAPON_DAMAGE_TYPE] = damage_general;
      local->custom[WEAPON_FINISH] = gore_cut;
      local->custom[WEAPON_FINISH_ALT] = gore_stab;

      MKATKSEQ(local, -2, -2, -1, 2, 3, 4, 5, 6, 7, 8)

      break;

    case treasure_machete:
      set_item_single_name(local, "machete", "machete");
      set_item_description(local, "A handle with a long blade, used for chopping and commonly employed to make way through thick vegetation. It has seen some heavy use.");
      local->gent = gent_sword;
      local->id |= known_name;
      local->item_type = item_type_m_weapon;
      local->attr_mod[attr_i_wound] = 50;
      local->custom[WEAPON_DAMAGE_TYPE] = damage_general;
      local->custom[WEAPON_FINISH] = gore_cut;
      local->custom[WEAPON_FINISH_ALT] = gore_stab;
      local->weight = WEIGHT_DAGGER;
      
      MKATKSEQ(local, -2, -2, -1, 2, 4, 6, 0, 0, 0, 0)

      break;

    case treasure_crappy_weapon:
      set_item_single_name(local, "crappy weapon", "crappy weapon");
      set_item_description(local, "This is purely a debug item. How did you get your hands on it?");
      local->no_wish = true;
      local->gent = gent_sword;
      local->id |= known_name;
      local->item_type = item_type_m_weapon;
      local->attr_mod[attr_i_wound] = 90;
      local->custom[WEAPON_DAMAGE_TYPE] = damage_general;
      local->custom[WEAPON_FINISH] = gore_cut;
      local->custom[WEAPON_FINISH_ALT] = gore_stab;
      local->custom[WEAPON_BREAK] = 9900;
      local->weight = WEIGHT_DAGGER;
      
      MKATKSEQ(local, -2, -2, -1, 1, 1, 0, 0, 0, 0, 0)

      break;

    case treasure_p_yuck:
      sprintf(temp, "%ss", get_potion_name(local->item_number));
      local->gent = get_potion_gent(local->item_number);
      set_item_plural_name(local, temp, "potions of yuck");
      set_item_single_name(local, get_potion_name(local->item_number),
			   "potion of yuck");
      local->single_id_article = article_i_a;
      local->item_type = item_type_potion;
      local->weight = WEIGHT_POTION;
      break;

    case treasure_p_blindness:
      sprintf(temp, "%ss", get_potion_name(local->item_number));
      local->gent = get_potion_gent(local->item_number);
      set_item_plural_name(local, temp, "potions of blindness");
      set_item_single_name(local, get_potion_name(local->item_number),
			   "potion of blindness");
      local->single_id_article = article_i_a;
      local->item_type = item_type_potion;
      local->weight = WEIGHT_POTION;
      break;

    case treasure_elixir:
      sprintf(temp, "%ss", get_potion_name(local->item_number));
      set_item_plural_name(local, temp, "elixirs");
      set_item_single_name(local, get_potion_name(local->item_number), "elixir");
      set_item_description(local, "Removes all temporary effects (even beneficial ones) from a creature.");
      local->gent = get_potion_gent(local->item_number);
      local->single_id_article = article_i_an;
      local->item_type = item_type_potion;
      local->weight = WEIGHT_POTION;
      break;

    case treasure_p_poison:
      sprintf(temp, "%ss", get_potion_name(local->item_number));
      set_item_plural_name(local, temp, "potions of poison");
      set_item_single_name(local, get_potion_name(local->item_number), "potion of poison");
      local->gent = get_potion_gent(local->item_number);
      local->item_type = item_type_potion;
      local->weight = WEIGHT_POTION;
      break;

    case treasure_p_sleep:
      sprintf(temp, "%ss", get_potion_name(local->item_number));
      set_item_plural_name(local, temp, "potions of sleep");
      set_item_single_name(local, get_potion_name(local->item_number), "potion of sleep");
      set_item_description(local, "Instantly puts a creature into deep sleep.");
      local->gent = get_potion_gent(local->item_number);
      local->item_type = item_type_potion;
      local->weight = WEIGHT_POTION;
      break;

    case treasure_p_slowing:
      sprintf(temp, "%ss", get_potion_name(local->item_number));
      set_item_plural_name(local, temp, "potions of slowing");
      set_item_single_name(local, get_potion_name(local->item_number), "potion of slowing");
      set_item_description(local, "Slows a creature down.");
      local->gent = get_potion_gent(local->item_number);
      local->item_type = item_type_potion;
      local->weight = WEIGHT_POTION;
      break;

    case treasure_p_polymorph:
      sprintf(temp, "%ss", get_potion_name(local->item_number));
      set_item_plural_name(local, temp, "potions of polymorph");
      set_item_single_name(local, get_potion_name(local->item_number), "potion of polymorph");
      set_item_description(local, "Changes a creature into something different.");
      local->gent = get_potion_gent(local->item_number);
      local->item_type = item_type_potion;
      local->weight = WEIGHT_POTION;
      break;

    case treasure_p_speed:
      sprintf(temp, "%ss", get_potion_name(local->item_number));
      set_item_plural_name(local, temp, "potions of speed");
      set_item_single_name(local, get_potion_name(local->item_number), "potion of speed");
      set_item_description(local, "It speeds you up, duh.");
      local->gent = get_potion_gent(local->item_number);
      local->item_type = item_type_potion;
      local->weight = WEIGHT_POTION;
      break;

    case treasure_p_energy:
      sprintf(temp, "%ss", get_potion_name(local->item_number));
      set_item_plural_name(local, temp, "potions of energy");
      set_item_single_name(local, get_potion_name(local->item_number), "potion of energy");
      sprintf(temp, "Restores %d points of energy.", POTION_OF_ENERGY);
      set_item_description(local, temp);
      local->gent = get_potion_gent(local->item_number);
      local->item_type = item_type_potion;
      local->weight = WEIGHT_POTION;
      break;

    case treasure_p_healing:
      sprintf(temp, "%ss", get_potion_name(local->item_number));
      set_item_plural_name(local, temp, "potions of healing");
      set_item_single_name(local, get_potion_name(local->item_number), "potion of healing");
      set_item_description(local, "Gradually restores a small amount of health.");
      local->gent = get_potion_gent(local->item_number);
      local->item_type = item_type_potion;
      local->weight = WEIGHT_POTION;
      break;

    case treasure_p_instant_healing:
      sprintf(temp, "%ss", get_potion_name(local->item_number));
      set_item_plural_name(local, temp, "potions of instant healing");
      set_item_single_name(local, get_potion_name(local->item_number), "potion of instant healing");
      set_item_description(local, "Instantly restores an amount of Health.");
      local->gent = get_potion_gent(local->item_number);
      local->item_type = item_type_potion;
      local->weight = WEIGHT_POTION;
      break;

    case treasure_p_pain:
      sprintf(temp, "%ss", get_potion_name(local->item_number));
      set_item_plural_name(local, temp, "potions of pain");
      set_item_single_name(local, get_potion_name(local->item_number), "potion of pain");
      local->gent = get_potion_gent(local->item_number);
      local->item_type = item_type_potion;
      local->weight = WEIGHT_POTION;
      break;

    case treasure_p_levitation:
      sprintf(temp, "%ss", get_potion_name(local->item_number));
      set_item_plural_name(local, temp, "potions of levitation");
      set_item_single_name(local, get_potion_name(local->item_number), "potion of levitation");
      set_item_description(local, "Makes a creature levitate (fly) for a short while.");
      local->gent = get_potion_gent(local->item_number);
      local->item_type = item_type_potion;
      local->weight = WEIGHT_POTION;
      break;

    case treasure_s_identify:
      set_item_single_name(local, get_scroll_name(local->item_number), "scroll of identify");
      set_item_plural_name(local, get_scroll_name_plural(local->item_number), "scrolls of identify");
      local->item_type = item_type_scroll;
      local->invoke_power = attr_m_identify;
      break;

    case treasure_b_noxious_breath:
      set_item_single_name(local, get_book_name(local->item_number), "book of Noxious Breath");
      local->item_type = item_type_book;
      local->custom[BOOK_ABILITY] = attr_m_noxious_breath;
      break;

    case treasure_b_breathe_fire:
      set_item_single_name(local, get_book_name(local->item_number), "book of Breathe Fire");
      local->item_type = item_type_book;
      local->custom[BOOK_ABILITY] = attr_m_breathe_fire;
      break;

    case treasure_b_force_bolt:
      set_item_single_name(local, get_book_name(local->item_number), "book of Force Bolt");
      local->item_type = item_type_book;
      local->custom[BOOK_ABILITY] = attr_m_force_bolt;
      break;

    case treasure_b_deathspell:
      set_item_single_name(local, get_book_name(local->item_number), "book of Deathspell");
      local->item_type = item_type_book;
      local->custom[BOOK_ABILITY] = attr_m_deathspell;
      break;

    case treasure_b_flash:
      set_item_single_name(local, get_book_name(local->item_number), "book of Flash");
      local->item_type = item_type_book;
      local->custom[BOOK_ABILITY] = attr_m_flash;
      break;

    case treasure_b_frost_ray:
      set_item_single_name(local, get_book_name(local->item_number), "book of Frost Ray");
      local->item_type = item_type_book;
      local->custom[BOOK_ABILITY] = attr_m_frost_ray;
      break;

/*    case treasure_b_camouflage:
      set_item_single_name(local, get_book_name(local->item_number), "manual of camouflage");
      set_item_description(local, "This manual contains instructions on remaining unseen and avoiding detection.");
      local->item_type = item_type_book;
      break;*/

    case treasure_b_first_aid:
      set_item_single_name(local, get_book_name(local->item_number), "manual of first aid");
      set_item_description(local, "This manual contains pratical instructions for dealing with physical trauma, bandaging wounds and stopping bloodflow.");
      local->item_type = item_type_book;
      local->custom[BOOK_ABILITY] = attr_m_first_aid;
      break;

    case treasure_b_pharmacy:
      set_item_single_name(local, get_book_name(local->item_number), "manual of pharmacy");
      set_item_description(local, "This manual contains detailed description of potions and their effects.");
      local->item_type = item_type_book;
      break;

    case treasure_s_magic_weapon:
      set_item_single_name(local, get_scroll_name(local->item_number), "scroll of magic weapon");
      set_item_plural_name(local, get_scroll_name_plural(local->item_number), "scrolls of magic weapon");
      local->item_type = item_type_scroll;
      local->invoke_power = attr_m_magic_weapon;
      break;

    case treasure_s_amnesia:
      set_item_single_name(local, get_scroll_name(local->item_number), "scroll of amnesia");
      set_item_plural_name(local, get_scroll_name_plural(local->item_number), "scroll of amnesia");
      local->item_type = item_type_scroll;
      local->invoke_power = attr_m_amnesia;
      break;

    case treasure_s_magic_mapping:
      set_item_single_name(local, get_scroll_name(local->item_number), "scroll of magic mapping");
      set_item_plural_name(local, get_scroll_name_plural(local->item_number), "scrolls of magic mapping");
      local->item_type = item_type_scroll;
      local->invoke_power = attr_m_magic_mapping;
      break;

    case treasure_s_recharge:
      set_item_single_name(local, get_scroll_name(local->item_number), "recharging scroll");
      set_item_plural_name(local, get_scroll_name_plural(local->item_number), "recharging scrolls");
      set_item_description(local, "This scroll will let you recharge a magical wand.");
      local->item_type = item_type_scroll;
      local->invoke_power = attr_m_recharge;
      break;

    case treasure_s_trap_detection:
      set_item_single_name(local, get_scroll_name(local->item_number), "trap detection scroll");
      set_item_plural_name(local, get_scroll_name_plural(local->item_number), "trap detection scroll");
      local->item_type = item_type_scroll;
      local->invoke_power = attr_m_reveal_traps;
      break;

    case treasure_s_familiar:
      set_item_single_name(local, get_scroll_name(local->item_number), "scroll of Summon Familiar");
      set_item_plural_name(local, get_scroll_name_plural(local->item_number), "scrolls of Summon Familiar");
      local->item_type = item_type_scroll;
      local->invoke_power = attr_m_summon_familiar;
      break;

    case treasure_s_blink:
      set_item_single_name(local, get_scroll_name(local->item_number), "blink scroll");
      set_item_plural_name(local, get_scroll_name_plural(local->item_number), "blink scrolls");
      local->item_type = item_type_scroll;
      local->invoke_power = attr_m_blink;
      break;

/*    case treasure_s_wish:
      set_item_single_name(local, get_scroll_name(local->item_number), "wish scroll");
      set_item_plural_name(local, get_scroll_name_plural(local->item_number), "wish scrolls");
      local->item_type = item_type_scroll;
      local->invoke_power = attr_m_wish;
      break;*/

    case treasure_s_mark:
      set_item_single_name(local, get_scroll_name(local->item_number), "mark scroll");
      set_item_plural_name(local, get_scroll_name_plural(local->item_number), "mark scrolls");
      local->item_type = item_type_scroll;
      local->invoke_power = attr_m_mark;
      break;

    case treasure_s_recall:
      set_item_single_name(local, get_scroll_name(local->item_number), "recall scroll");
      set_item_plural_name(local, get_scroll_name_plural(local->item_number), "recall scrolls");
      local->item_type = item_type_scroll;
      local->invoke_power = attr_m_recall;
      break;

    case treasure_d_poison:
    case treasure_d_poison_pile:
      set_item_single_name(local, "dart", "poisoned dart");
      set_item_plural_name(local, "darts", "poisoned darts");
      set_item_description(local, "These darts have had their tips dipped in poison, which will transfer to a victims bloodstream if delivered with sufficient force to break the skin.");
      local->gent = gent_dart;
      local->item_type = item_type_ammo;
      local->sort_type = sort_type_dart;
      local->custom[WEAPON_MIN_DAMAGE] = 1;
      local->attr_mod[attr_i_poison] = 2;
      local->custom[CUSTOM_AMMO_DTYPE] = damage_poison;
      local->custom[CUSTOM_AMMO_ATYPE] = ammo_type_dart;
      local->weight = WEIGHT_MISSILE;
      break;

    case treasure_d_tranq:
    case treasure_d_tranq_pile:
      set_item_single_name(local, "dart", "tranquilizer dart");
      set_item_plural_name(local, "darts", "tranquilizer darts");
      set_item_description(local, "These darts immediately put a creature into deep sleep.");
      local->gent = gent_dart;
      local->item_type = item_type_ammo;
      local->sort_type = sort_type_dart;
      local->custom[WEAPON_MIN_DAMAGE] = 0;
      local->custom[CUSTOM_AMMO_ATYPE] = ammo_type_dart;
      local->weight = WEIGHT_MISSILE;
      break;

    case treasure_d_small:
    case treasure_d_small_pile:
      set_item_single_name(local, "dart", "small dart");
      set_item_plural_name(local, "darts", "small darts");
      local->auto_id = true;
      local->gent = gent_dart;
      local->item_type = item_type_ammo;
      local->sort_type = sort_type_dart;
      local->custom[WEAPON_MIN_DAMAGE] = 3;
      local->custom[CUSTOM_AMMO_DTYPE] = damage_general;
      local->custom[CUSTOM_AMMO_ATYPE] = ammo_type_dart;
      local->weight = WEIGHT_MISSILE;
      break;

    case treasure_force_bolt:
      set_item_single_name(local, "force bolt", "force bolt");
      local->no_wish = true;
      local->ricochets = true;
      local->item_type = item_type_spell;
      local->custom[WEAPON_MIN_DAMAGE] = 5;
      break;

    case treasure_mudball:
      set_item_single_name(local, "mudball", "mudball");
      local->no_wish = true;
      local->item_type = item_type_spell;
      local->custom[WEAPON_MIN_DAMAGE] = 5;
      /* TODO: Blinds target */
      break;

    case treasure_fireball:
      set_item_single_name(local, "fireball", "fireball");
      local->no_wish = true;
      local->item_type = item_type_spell;
/*      local->custom[CUSTOM_AMMO_EXPLOSIVE] = true;*/
      local->custom[WEAPON_MIN_DAMAGE] = 5;
      local->custom[WEAPON_MIN_DAMAGE] = 15;
      break;

    case treasure_b_bullet:
    case treasure_b_bullet_pile:
      set_item_single_name(local, "bullet", "bullet");
      set_item_plural_name(local, "bullets", "bullets");
      local->id |= known_name;
      local->gent = gent_bullet;
      local->item_type = item_type_ammo;
      local->sort_type = sort_type_bullet;
      local->custom[WEAPON_MIN_DAMAGE] = 0;
      local->custom[CUSTOM_AMMO_DTYPE] = damage_general;
      local->custom[CUSTOM_AMMO_ATYPE] = ammo_type_bullet;
      local->weight = WEIGHT_MISSILE;
      break;

    case treasure_b_hollow:
    case treasure_b_hollow_pile:
      set_item_single_name(local, "bullet", "hollow-point bullet");
      set_item_plural_name(local, "bullets", "hollow-point bullets");
      local->gent = gent_bullet;
      local->item_type = item_type_ammo;
      local->sort_type = sort_type_bullet;
      local->custom[WEAPON_MIN_DAMAGE] = 0;
      local->attr_mod[attr_i_wound] = 1;
      local->custom[CUSTOM_AMMO_DTYPE] = damage_general;
      local->custom[CUSTOM_AMMO_ATYPE] = ammo_type_bullet;
      local->weight = WEIGHT_MISSILE;
      break;

    case treasure_s_buckshot:
    case treasure_s_buckshot_pile:
      set_item_single_name(local, "shotgun shell", "buckshot shell");
      set_item_plural_name(local, "shotgun shells", "buckshot shells");
      local->gent = gent_shell;
      local->plural_unid_article = article_i_some_are;
      local->plural_id_article = article_i_some_are;
      local->item_type = item_type_ammo;
      local->sort_type = sort_type_shell;
      local->custom[WEAPON_MIN_DAMAGE] = 0;
      local->custom[CUSTOM_AMMO_DTYPE] = damage_general;
      local->custom[CUSTOM_AMMO_ATYPE] = ammo_type_shell;
      local->weight = WEIGHT_MISSILE;
      break;

    case treasure_grenade:
      set_item_single_name(local, "grenade", "grenade");
      set_item_plural_name(local, "grenades", "grenades");
      local->id |= known_name;
      local->gent = gent_grenade;
      local->plural_unid_article = article_i_none;
      local->plural_unid_article = article_i_none;
      local->item_type = item_type_ammo;
      local->custom[WEAPON_MIN_DAMAGE] = 10;
      local->custom[WEAPON_MAX_DAMAGE] = 20;
      local->ricochets = true;
      local->custom[CUSTOM_AMMO_DTYPE] = damage_general;
      local->custom[CUSTOM_AMMO_ATYPE] = ammo_type_grenade;
      local->custom[CUSTOM_AMMO_EXPLOSIVE] = true;
      local->weight = WEIGHT_MISSILE;
      break;

    case treasure_a_crude:
    case treasure_a_crude_pile:
      set_item_single_name(local, "arrow", "crude arrow");
      set_item_plural_name(local, "arrows", "crude arrows");
      local->auto_id = true;
      local->gent = gent_arrow;
      local->single_unid_article = article_i_an;
      local->item_type = item_type_ammo;
      local->sort_type = sort_type_arrow;
      local->custom[WEAPON_MIN_DAMAGE] = 0;
      local->custom[CUSTOM_AMMO_DTYPE] = damage_general;
      local->custom[CUSTOM_AMMO_ATYPE] = ammo_type_arrow;
      local->weight = WEIGHT_MISSILE;
      break;

    case treasure_a_sharp:
    case treasure_a_sharp_pile:
      set_item_single_name(local, "arrow", "sharp arrow");
      set_item_plural_name(local, "arrows", "sharp arrows");
      local->auto_id = true;
      local->gent = gent_arrow;
      local->single_unid_article = article_i_an;
      local->item_type = item_type_ammo;
      local->sort_type = sort_type_arrow;
      local->custom[WEAPON_MIN_DAMAGE] = +1;
      local->custom[CUSTOM_AMMO_DTYPE] = damage_general;
      local->custom[CUSTOM_AMMO_ATYPE] = ammo_type_arrow;
      local->weight = WEIGHT_MISSILE;
      break;

    case treasure_a_masterwork:
    case treasure_a_masterwork_pile:
      set_item_single_name(local, "arrow", "masterwork arrow");
      set_item_plural_name(local, "arrows", "masterwork arrows");
      local->gent = gent_arrow;
      local->single_unid_article = article_i_an;
      local->item_type = item_type_ammo;
      local->sort_type = sort_type_arrow;
      local->custom[WEAPON_MIN_DAMAGE] = +3;
      local->custom[CUSTOM_AMMO_DTYPE] = damage_general;
      local->custom[CUSTOM_AMMO_ATYPE] = ammo_type_arrow;
      local->weight = WEIGHT_MISSILE;
      break;

    case treasure_a_black:
      set_item_single_name(local, "arrow", "black arrow");
      set_item_plural_name(local, "arrows", "black arrows");
      local->gent = gent_arrow;
      local->single_unid_article = article_i_an;
      local->item_type = item_type_ammo;
      local->sort_type = sort_type_arrow;
      local->custom[WEAPON_MIN_DAMAGE] = +5;
      local->custom[CUSTOM_AMMO_DTYPE] = damage_general;
      local->custom[CUSTOM_AMMO_ATYPE] = ammo_type_arrow;
      local->weight = WEIGHT_MISSILE;
      break;

    case treasure_a_flaming:
      set_item_single_name(local, "arrow", "flaming arrow");
      set_item_plural_name(local, "arrows", "flaming arrows");
      local->gent = gent_arrow;
      local->single_unid_article = article_i_an;
      local->item_type = item_type_ammo;
      local->sort_type = sort_type_arrow;
      local->custom[WEAPON_MIN_DAMAGE] = +10;
      local->custom[CUSTOM_AMMO_DTYPE] = damage_fire;
      local->custom[CUSTOM_AMMO_ATYPE] = ammo_type_arrow;
      local->weight = WEIGHT_MISSILE;
      break;

    case treasure_blowgun:
      set_item_single_name(local, "blowgun", "blowgun");
      local->gent = gent_blowgun;
      local->item_type = item_type_r_weapon;
      local->sort_type = sort_type_crossbow;
      local->custom[WEAPON_MIN_DAMAGE] = 2;
      local->custom[WEAPON_MAX_DAMAGE] = 4;
      local->custom[WEAPON_RANGE] = 7;
      local->custom[WEAPON_AMMO_TYPE] = ammo_type_dart;
      local->weight = WEIGHT_BOW;
      break;

    case treasure_crossbow:
      set_item_single_name(local, "crossbow", "crossbow");
      local->id |= known_name;
      local->gent = gent_crossbow;
      local->item_type = item_type_r_weapon;
      local->sort_type = sort_type_crossbow;
      local->custom[WEAPON_MIN_DAMAGE] = 3;
      local->custom[WEAPON_MAX_DAMAGE] = 5;
      local->custom[WEAPON_RANGE] = 7;
      local->custom[WEAPON_AMMO_TYPE] = ammo_type_dart;
      local->weight = WEIGHT_BOW;
      break;

    case treasure_longbow:
      set_item_single_name(local, "bow", "longbow");
      local->id |= known_name;
      local->gent = gent_bow;
      local->item_type = item_type_r_weapon;
      local->sort_type = sort_type_bow;
      local->custom[WEAPON_MIN_DAMAGE] = 2;
      local->custom[WEAPON_MAX_DAMAGE] = 6;
      local->custom[WEAPON_RANGE] = 8;
      local->custom[WEAPON_AMMO_TYPE] = ammo_type_arrow;
      local->weight = WEIGHT_BOW;
      break;

    case treasure_hunting_rifle:
      set_item_single_name(local, "rifle", "hunting rifle");
      local->gent = gent_shotgun;
      local->item_type = item_type_r_weapon;
      local->sort_type = sort_type_pistol;
      local->custom[WEAPON_MIN_DAMAGE] = 6;
      local->custom[WEAPON_MAX_DAMAGE] = 10;
      local->custom[WEAPON_RANGE] = 8;
      local->custom[WEAPON_AMMO_TYPE] = ammo_type_bullet;
      local->weight = WEIGHT_FIREARM;
      break;

    case treasure_shotgun:
      set_item_single_name(local, "shotgun", "sawn-off shotgun");
      set_item_description(local, "Pump-action. It has good precision but limited range.");
      local->gent = gent_shotgun;
      local->item_type = item_type_r_weapon;
      local->sort_type = sort_type_shotgun;
      local->custom[WEAPON_MIN_DAMAGE] = 6;
      local->custom[WEAPON_MAX_DAMAGE] = 10;
      local->attr_mod[attr_i_precision] = +10;
      local->custom[WEAPON_RANGE] = 3;
      local->custom[WEAPON_AMMO_TYPE] = ammo_type_shell;
      local->weight = WEIGHT_FIREARM;
      break;

    case treasure_pistol:
      set_item_single_name(local, "pistol", "pistol");
      local->gent = gent_pistol;
      local->item_type = item_type_r_weapon;
      local->sort_type = sort_type_pistol;
      local->custom[WEAPON_MIN_DAMAGE] = 5;
      local->custom[WEAPON_MAX_DAMAGE] = 10;
      local->custom[WEAPON_RANGE] = 3;
      local->custom[WEAPON_AMMO_TYPE] = ammo_type_bullet;
      local->weight = WEIGHT_FIREARM;
      break;

    case artifact_yyterr_durr:
      set_item_single_name(local, "staff", "staff \"Y\'yter Durr\"");
      local->gent = gent_staff;
      local->artifact = true;
      local->single_id_article = article_i_the;
      local->item_type = item_type_m_weapon;
/*      local->invoke_power = attr_m_teleport;
	local->charges = roll(1, 4);*/
      local->custom[WEAPON_DAMAGE_TYPE] = damage_general;
      local->custom[WEAPON_TWOHANDED] = true;
      local->custom[WEAPON_FINISH] = gore_crush;
      local->weight = WEIGHT_STAFF;

      MKATKSEQ(local, -2, -1, -1, 4, 10, 0, 0, 0, 0, 0)

      break;

    case artifact_runestaff:
      set_item_single_name(local, "staff", "Runestaff");
      local->gent = gent_staff;
      local->artifact = true;
      local->single_id_article = article_i_the;
      local->item_type = item_type_m_weapon;
      local->invoke_power = attr_m_teleport;
      local->charges = roll(1, 4);
      local->attr_mod[attr_i_blink] = 5;
      local->custom[WEAPON_DAMAGE_TYPE] = damage_general;
      local->custom[WEAPON_TWOHANDED] = true;
      local->weight = WEIGHT_STAFF;

      MKATKSEQ(local, -2, -1, -1, 6, 6, 0, 0, 0, 0, 0)

      break;

    case artifact_whip_of_thorns:
      set_item_single_name(local, "whip", "\"Whip of Thorns\"");
      local->single_id_article = article_i_the;
      local->gent = gent_whip;
      local->no_wish = true;
      local->item_type = item_type_m_weapon;
      local->attr_mod[attr_i_drain] = 20;
      local->custom[WEAPON_ATTACK_STRING]  = attack_string_whips;
      local->custom[WEAPON_DAMAGE_TYPE] = damage_general;
      local->weight = WEIGHT_SWORD_SMALL;

      MKATKSEQ(local, -2, -1, -1, 2, 4, 8, 8, 0, 0, 0)
      break;

    case artifact_fire_drake_boots:
      set_item_single_name(local, "boots", "\"Fire Drake Boots\"");
      set_item_description(local, "They can sit forever in lava without burning up.");
      local->gent = gent_boots;
      local->artifact = true;
      local->single_unid_article = article_i_a_pair_of;
      local->single_id_article = article_i_the;
      local->item_type = item_type_feet;
      local->attr_mod[attr_fire_resistance] = +3;
      local->attr_mod[attr_feet_protected] = 1;
      local->weight = WEIGHT_BOOTS;
      break;

    case artifact_heavy_plate_mail_of_doom:
      set_item_single_name(local, "armor", "\"Heavy Plate Mail of Doom\"");
      local->gent = gent_heavy_armor;
      local->artifact = true;
      local->single_unid_article = article_i_a_set_of;
      local->single_id_article = article_i_the;
      local->item_type = item_type_body;
      local->custom[ARMOR_PROTECTION] = 5;
      local->custom[ARMOR_MAX_DURABILITY] = 70;
/*      local->attr_mod[attr_fatigue_limit] = +5;*/
      local->attr_mod[attr_speed] = -1;
      local->attr_mod[attr_swimming] = -2;
      local->attr_mod[attr_stealth] = -1;
      local->weight = WEIGHT_HEAVY_ARMOR;

      add_uniqitem(g, UNIQITEM_ARMOR, local->item_number);
      break;

    case artifact_battalion_mail:
      set_item_single_name(local, "armor", "\"Battalion Mail\"");
      local->gent = gent_heavy_armor;
      local->artifact = true;
      local->single_unid_article = article_i_a_set_of;
      local->single_id_article = article_i_the;
      local->item_type = item_type_body;
      local->custom[ARMOR_PROTECTION] = 7;
      local->custom[ARMOR_MAX_DURABILITY] = 500;
      local->attr_mod[attr_speed] = -10;
      local->attr_mod[attr_swimming] = -1;
      local->attr_mod[attr_stealth] = -1;
      local->weight = WEIGHT_HEAVY_ARMOR;

      add_uniqitem(g, UNIQITEM_ARMOR, local->item_number);
      break;

    case artifact_dragon_scales:
      set_item_single_name(local, "armor", "dragon scales");
      local->gent = gent_heavy_armor;
      local->artifact = true;
      local->single_unid_article = article_i_a_set_of;
      local->single_id_article = article_i_some_are;
      local->item_type = item_type_body;
      local->custom[ARMOR_PROTECTION] = 5;
      local->custom[ARMOR_MAX_DURABILITY] = 800;
      local->attr_mod[attr_fire_resistance] = DRAGON_FIRE_RESISTANCE;
      local->attr_mod[attr_speed] = +10;
      local->weight = WEIGHT_LIGHT_ARMOR;
      break;

/*    case artifact_ring_of_the_archmage:
      set_item_single_name(local, "ring", "ring of the Arch-Mage");
      local->gent = gent_ring;
      local->artifact = true;
      local->single_id_article = article_i_the;
      local->item_type = item_type_ring_fixed;
      local->attr_mod[attr_ep_max] = +15;
      local->weight = WEIGHT_RING;
      break;*/

    case artifact_crown_of_thorns:
      set_item_single_name(local, "crown", "\"Crown of Thorns\"");
      local->gent = gent_crown;
      local->artifact = true;
      local->single_id_article = article_i_the;
      local->item_type = item_type_head;
      local->attr_mod[attr_acid_resistance] = +2;
      local->weight = WEIGHT_HAT;

      add_uniqitem(g, UNIQITEM_ARMOR, local->item_number);
      break;

    case artifact_coral_crown:
      set_item_single_name(local, "crown", "\"Coral Crown\"");
      local->gent = gent_crown;
      local->artifact = true;
      local->single_id_article = article_i_the;
      local->attr_mod[attr_speed] = +20;
      local->item_type = item_type_head;
      local->weight = WEIGHT_HAT;

      add_uniqitem(g, UNIQITEM_ARMOR, local->item_number);
      break;

    case artifact_mail_of_life:
      set_item_single_name(local, "armor", "\"Mail of Life\"");
      local->gent = gent_heavy_armor;
      local->artifact = true;
      local->single_id_article = article_i_the;
      local->single_unid_article = article_i_a_set_of;
      local->item_type = item_type_body;
      local->custom[ARMOR_PROTECTION] = 2;
      local->custom[ARMOR_MAX_DURABILITY] = 70;
/*      local->attr_mod[attr_fatigue_limit] = 5;*/
      local->invoke_power = attr_m_cure_poison;
      local->charges = roll(1, 6);
      local->weight = 120;
      local->weight = WEIGHT_LIGHT_ARMOR;

      add_uniqitem(g, UNIQITEM_ARMOR, local->item_number);
      break;

/*    case artifact_rogues_ring_escape:
      set_item_single_name(local, "ring", "Rogues ring of Hasty Escape");
      local->gent = gent_ring;
      local->artifact = true;
      local->single_id_article = article_i_the;
      local->item_type = item_type_ring_fixed;
      local->attr_mod[attr_speed] = +1;
      local->invoke_power = attr_m_blink;
      local->charges = 1;
      local->rechargeable = false;
      local->weight = WEIGHT_RING;
      break;*/

    case artifact_blood_fang:
      set_item_single_name(local, "sword", "sabre \"Blood Fang\"");
      local->gent = gent_sword;
      local->no_wish = true;
      local->single_id_article = article_i_the;
      local->item_type = item_type_m_weapon;
      local->custom[WEAPON_DAMAGE_TYPE] = damage_general;
      local->custom[WEAPON_FINISH] = gore_cut;
      local->custom[WEAPON_FINISH_ALT] = gore_impale;
      local->weight = WEIGHT_SWORD_SMALL;

      MKATKSEQ(local, -2, -2, -1, 1, 1, 1, 1, 30, 0, 0)

      add_uniqitem(g, UNIQITEM_WEAPON, local->item_number);
      break;

    case artifact_black_plate:
      set_item_single_name(local, "armor", "\"Black Plate\"");
      local->gent = gent_heavy_armor;
      local->artifact = true;
      local->single_id_article = article_i_the;
      local->single_unid_article = article_i_a_set_of;
      local->item_type = item_type_body;
      local->custom[ARMOR_PROTECTION] = 3;
      local->custom[ARMOR_MAX_DURABILITY] = 500;
      local->attr_mod[attr_speed] = +20;
      local->attr_mod[attr_s_trap_detection] = +10;
      local->weight = WEIGHT_HEAVY_ARMOR;

      add_uniqitem(g, UNIQITEM_ARMOR, local->item_number);
      break;

    case artifact_macabre_sabre:
      set_item_single_name(local, "sword", "\"Macabre Sabre\"");
      local->gent = gent_sword;
      local->artifact = true;
      local->single_id_article = article_i_the;
      local->item_type = item_type_m_weapon;
      local->attr_mod[attr_i_backstab] = 5;
      local->custom[WEAPON_DAMAGE_TYPE] = damage_general;
      local->custom[WEAPON_FINISH] = gore_cut;
      local->custom[WEAPON_FINISH_ALT] = gore_impale;
      local->weight = WEIGHT_SWORD_SMALL;

      MKATKSEQ(local, -2, -2, -1, 4, 2, 2, 7, 0, 0, 0)

      add_uniqitem(g, UNIQITEM_WEAPON, local->item_number);
      break;

    case artifact_glory:
      set_item_single_name(local, "sword", "sword \"Glory\"");
      local->gent = gent_sword;
      local->artifact = true;
      local->single_id_article = article_i_the;
      local->item_type = item_type_m_weapon;
      local->custom[WEAPON_DAMAGE_TYPE] = damage_general;
      local->custom[WEAPON_FINISH] = gore_cut;
      local->weight = WEIGHT_SWORD_SMALL;

      MKATKSEQ(local, -2, -2, -1, 6, 8, 4, 0, 0, 0, 0)

      add_uniqitem(g, UNIQITEM_WEAPON, local->item_number);
      break;

    case artifact_lifetaker:
      set_item_single_name(local, "axe", "axe \"Lifetaker\"");
      local->gent = gent_axe;
      local->artifact = true;
      local->single_unid_article = article_i_an;
      local->single_id_article = article_i_the;
      local->item_type = item_type_m_weapon;
      local->custom[WEAPON_DAMAGE_TYPE] = damage_general;
      local->attr_mod[attr_i_drain] = 50;
      local->weight = WEIGHT_AXE;

      MKATKSEQ(local, -2, -1, -1, 4, 4, 0, 0, 0, 0, 0)

      add_uniqitem(g, UNIQITEM_WEAPON, local->item_number);
      break;

    case artifact_axe_trollish:
      set_item_single_name(local, "axe", "\"Axe of Trollish Bloodlust\"");
      local->gent = gent_axe;
      local->artifact = true;
      local->single_unid_article = article_i_an;
      local->single_id_article = article_i_the;
      local->item_type = item_type_m_weapon;
      local->custom[WEAPON_DAMAGE_TYPE] = damage_general;
      local->weight = WEIGHT_AXE;

      MKATKSEQ(local, -2, -1, -1, 8, 6, 8, 0, 0, 0, 0)

      add_uniqitem(g, UNIQITEM_WEAPON, local->item_number);
      break;

    case artifact_axe_of_the_north:
      set_item_single_name(local, "axe", "\"Axe of the North\"");
      set_item_description(local, "");
      local->gent = gent_axe;
      local->artifact = true;
      local->single_unid_article = article_i_an;
      local->single_id_article = article_i_the;
      local->item_type = item_type_m_weapon;
      local->custom[WEAPON_DAMAGE_TYPE] = damage_cold;
      local->attr_mod[attr_cold_resistance] = +3;
      local->weight = WEIGHT_AXE;

      MKATKSEQ(local, -2, -1, -1, 3, 8, 0, 0, 0, 0, 0)

      add_uniqitem(g, UNIQITEM_WEAPON, local->item_number);
      break;

    case artifact_justice:
      set_item_single_name(local, "axe", "axe \"Justice\"");
      local->gent = gent_axe;
      local->artifact = true;
      local->single_unid_article = article_i_an;
      local->single_id_article = article_i_the;
      local->item_type = item_type_m_weapon;
      local->custom[WEAPON_DAMAGE_TYPE] = damage_general;
      local->attr_mod[attr_i_blink] = 10;
      local->custom[WEAPON_TWOHANDED] = true;
      local->weight = WEIGHT_AXE;

      MKATKSEQ(local, -1, -1, -1, 8, 10, 0, 0, 0, 0, 0)

      add_uniqitem(g, UNIQITEM_WEAPON, local->item_number);
      break;

    case artifact_headsplitter:
      set_item_single_name(local, "axe", "axe \"Headsplitter\"");
      local->gent = gent_axe;
      local->artifact = true;
      local->single_unid_article = article_i_an;
      local->single_id_article = article_i_the;
      local->item_type = item_type_m_weapon;
      local->custom[WEAPON_MIN_DAMAGE] = 3;
      local->custom[WEAPON_MAX_DAMAGE] = 8;
      local->custom[WEAPON_DAMAGE_TYPE] = damage_general;
      local->weight = WEIGHT_AXE;

      MKATKSEQ(local, -2, -2, -1, 4, 6, 0, 0, 0, 0, 0)

      add_uniqitem(g, UNIQITEM_WEAPON, local->item_number);
      break;

    case artifact_grim_cutter:
      set_item_single_name(local, "sword", "sword \"Grim Cutter\"");
      local->gent = gent_sword;
      local->artifact = true;
      local->single_id_article = article_i_the;
      local->item_type = item_type_m_weapon;
      local->attr_mod[attr_speed] = +20;
      local->custom[WEAPON_DAMAGE_TYPE] = damage_general;
      local->custom[WEAPON_FINISH] = gore_cut;
      local->custom[WEAPON_FINISH_ALT] = gore_impale;
      local->weight = WEIGHT_SWORD_SMALL;

      MKATKSEQ(local, -2, -2, -1, 6, 6, 6, 0, 0, 0, 0)

      add_uniqitem(g, UNIQITEM_WEAPON, local->item_number);
      break;

    case artifact_pike_of_thon:
      set_item_single_name(local, "spear", "\"Pike of Thon\"");
      local->gent = gent_spear;
      local->artifact = true;
      local->single_id_article = article_i_the;
      local->item_type = item_type_m_weapon;
      local->custom[WEAPON_DAMAGE_TYPE] = damage_general;
      local->custom[WEAPON_FINISH] = gore_impale;
      local->weight = WEIGHT_STAFF;

      MKATKSEQ(local, -2, -1, -1, 4, 8, 0, 0, 0, 0, 0)

      add_uniqitem(g, UNIQITEM_WEAPON, local->item_number);
      break;

    case artifact_sahiras_cudgel:
      set_item_single_name(local, "club", "\"Sahiras Cudgel\"");
      local->gent = gent_club;
      local->artifact = true;
      local->single_id_article = article_i_none;
      local->item_type = item_type_m_weapon;
      local->custom[WEAPON_DAMAGE_TYPE] = damage_cold;
      local->weight = WEIGHT_CLUB;

      MKATKSEQ(local, -2, -1, -1, 5, 7, 0, 0, 0, 0, 0)

      add_uniqitem(g, UNIQITEM_WEAPON, local->item_number);
      break;

    case artifact_avengers_steel:
      set_item_single_name(local, "sword", "sword \"Avengers Steel\"");
      local->gent = gent_sword;
      local->artifact = true;
      local->single_id_article = article_i_the;
      local->item_type = item_type_m_weapon;
      local->custom[WEAPON_DAMAGE_TYPE] = damage_fire;
      local->custom[WEAPON_FINISH] = gore_cut;
      local->attr_mod[attr_i_stun] = 10;
      local->attr_mod[attr_i_drain] = 10;
      local->weight = WEIGHT_SWORD_SMALL;

      MKATKSEQ(local, -2, -2, -1, 2, 2, 2, 2, 2, 20, 0)

      add_uniqitem(g, UNIQITEM_WEAPON, local->item_number);
      break;

    case artifact_starflame_mace:
      set_item_single_name(local, "mace", "\"Starflame Mace\"");
      local->gent = gent_club;
      local->artifact = true;
      local->single_id_article = article_i_the;
      local->item_type = item_type_m_weapon;
      local->attr_mod[attr_vision] = +2;
      local->custom[WEAPON_DAMAGE_TYPE] = damage_fire;
      local->custom[WEAPON_FINISH] = gore_crush;
      local->weight = WEIGHT_MACE;

      MKATKSEQ(local, -2, -2, -1, 4, 4, 4, 0, 0, 0, 0)

      add_uniqitem(g, UNIQITEM_WEAPON, local->item_number);
      break;

    case artifact_everlasting_lantern:
      set_item_single_name(local, "lantern", "\"Everlasting Lantern\"");
      local->gent = gent_lantern;
      local->artifact = true;
      local->single_id_article = article_i_the;
      local->item_type = item_type_light;
      local->attr_mod[attr_vision] = +3;
      local->custom[WEAPON_MIN_DAMAGE] = 5;
      local->custom[WEAPON_MAX_DAMAGE] = 10;
      local->custom[CUSTOM_LIGHT_TICKS] = -1;
      local->weight = WEIGHT_TOOL;

      add_uniqitem(g, UNIQITEM_WEAPON, local->item_number);
      break;
      
    case artifact_stormhammer:
      set_item_single_name(local, "hammer", "\"Stormhammer\"");
      local->gent = gent_club;
      local->artifact = true;
      local->single_id_article = article_i_the;
      local->item_type = item_type_m_weapon;
      local->custom[WEAPON_DAMAGE_TYPE] = damage_general;
      local->custom[WEAPON_FINISH] = gore_crush;
      local->attr_mod[attr_i_knockback] = 50;
      local->weight = WEIGHT_MACE;

      MKATKSEQ(local, -1, -1, -1, 15, 2, 2, 0, 0, 0, 0)

      add_uniqitem(g, UNIQITEM_WEAPON, local->item_number);
      break;

    case artifact_heart_piercer:
      set_item_single_name(local, "bow", "bow \"Heart Piercer\"");
      local->gent = gent_bow;
      local->artifact = true;
      set_item_description(local, "It seeks the heart of the enemy.");
      local->single_id_article = article_i_the;
      local->item_type = item_type_r_weapon;
      local->sort_type = sort_type_bow;
      local->custom[WEAPON_MIN_DAMAGE] = 5;
      local->custom[WEAPON_MAX_DAMAGE] = 12;
      local->custom[WEAPON_AMMO_TYPE] = ammo_type_arrow;
      local->custom[WEAPON_RANGE] = 5;
      local->weight = WEIGHT_BOW;

      add_uniqitem(g, UNIQITEM_WEAPON, local->item_number);
      break;

    case treasure_undefined:
    default:
      del_item(local);
      return NULL;
  }

  /* Items with random names must have a proper article set */
  if (local->item_type == item_type_wand)
  {
    local->single_unid_article = get_wand_article(local->item_number);
  }
  else if (local->item_type == item_type_potion)
  {
    local->single_unid_article = get_potion_article(local->item_number);
  }
  else if (local->item_type == item_type_book)
  {
    local->gent = gent_book;
    add_uniqitem(g, UNIQITEM_BOOKS, local->item_number);
  }
  else if (local->item_type == item_type_scroll)
  {
    local->single_unid_article = get_scroll_article(local->item_number);

    /* This is the same for _all_ scrolls. */
    local->gent = gent_scroll;
    local->charges = 1;
    local->rechargeable = false;
    local->weight = WEIGHT_SCROLL;
  }

/*  if (local->artifact && local->item_type == item_type_m_weapon)
    local->auto_id = true;*/

  if (local->item_type == item_type_body)
  {
    local->custom[ARMOR_DURABILITY] = local->custom[ARMOR_MAX_DURABILITY];
  }

  if (local->sort_type == item_type_none)
    local->sort_type = local->item_type;

  return local;
} /* build_item_template */



/*
  Clones an item of item number ITEM.

  If ITEM is an artifact, only one
  copy of it will be created, and subsequent calls to this function
  won't build any more of it. WELL MAYBE NOT.

  Items with certain properties (charges, light lifetime, etc) will
  have these randomized.
*/
item_t * build_item(const treasure_t item)
{
  item_t * temp;
  unsigned int t;

  if (item < 0 || item >= ITEMS || game->item_template[item] == NULL)
    return NULL;
  
  /*
    Most items we will just clone. Artifacts, OTOH, we will remove
    from the template list. This has two advantages - later attempts
    to create this item will automatically be foiled, and we won't
    need to build the item again.
  */
/*  if (game->item_template[item]->artifact == true)
  {
    temp = game->item_template[item];
    game->item_template[item] = NULL;
  }
  else
  {*/
/*  }*/

  temp = clone_item(game->item_template[item]);
  
  if (temp == NULL)
    return NULL;

  /* Item *type*-specific init code. */
  switch (temp->item_type)
  {
    case item_type_light:
      /* Make lightsources partly spent */
      temp->custom[CUSTOM_LIGHT_MAX_TICKS] = temp->custom[CUSTOM_LIGHT_TICKS];
      
      if ((temp->custom[CUSTOM_LIGHT_MAX_TICKS] > 0) &&
	  (roll(1, 4) != 1))
      {
	temp->custom[CUSTOM_LIGHT_TICKS] =
	  MAX(50, tslrnd() % temp->custom[CUSTOM_LIGHT_TICKS]);
      }
      
      break;

    default:
      /* Make some items cursed. */
/*      if (is_cursable(temp) && (roll(1, 4) == 1))
	temp->cursed = true;*/

      break;
  }

  /* 
     Randomize the number of charges (for items that have them). The
     value will be between 1 and whatever number was specified in the
     item template.
  */
  if (temp->charges > 0)
  {
    temp->charges = MAX(1, 1 + tslrnd() % temp->charges);
  }

  /* Item *number*-specific init code. */

  /*
    This is where we build item "piles". For those items that are
    marked as "pile", generate a random number of "base" items, then
    stack them below the "pile" item. The item number of the pile
    parent will also be set to that of the base item, so the items
    will stack properly when picked up.
  */
  t = 0;

  switch (temp->item_number)
  {
    case treasure_a_crude_pile:
      t = roll(3, 4);
      temp->item_number = treasure_a_crude;
      break;

    case treasure_a_sharp_pile:
      t = roll(2, 4);
      temp->item_number = treasure_a_sharp;
      break;

    case treasure_a_masterwork_pile:
      t = roll(1, 4);
      temp->item_number = treasure_a_masterwork;
      break;

    case treasure_d_small_pile:
      t = roll(2, 4);
      temp->item_number = treasure_d_small;
      break;

    case treasure_d_poison_pile:
      t = roll(2, 3);
      temp->item_number = treasure_d_poison;
      break;

    case treasure_d_tranq_pile:
      t = roll(2, 3);
      temp->item_number = treasure_d_tranq;
      break;

    case treasure_s_buckshot_pile:
      t = roll(2, 4);
      temp->item_number = treasure_s_buckshot;
      break;

    case treasure_b_bullet_pile:
      t = roll(2, 4);
      temp->item_number = treasure_b_bullet;
      break;

    case treasure_b_hollow_pile:
      t = roll(2, 4);
      temp->item_number = treasure_b_hollow;
      break;

    default:
      break;
  }

  while (t-- > 0)
    attach_item_to_stack(temp, build_item(temp->item_number));

  return temp;
} /* treasure */



/*
  Returns a random item from ITEM_TABLE.
*/
item_t * random_treasure(const item_table_t item_table)
{
  unsigned int gen = 0;

  switch(item_table)
  {
    case item_table_supply:
      switch (tslrnd() % 20)
      {
	case 0:
	case 1:
	case 2:
	case 3:
	case 4:  gen = treasure_s_identify;     break;  
	case 5:
	case 6:  gen = treasure_lantern;        break;
	case 7:
	case 8:
	case 9:
	case 10: gen = treasure_torch;          break;
	case 11:
	case 12:
	case 13: gen = treasure_dataprobe;      break;
	case 14:
	case 15:
	case 16:
	case 17:
	case 18:
	case 19: gen = treasure_key;            break;
      }
      break;

    case item_table_ammo:
      switch (tslrnd() % 29)
      {
	case 0: 
	case 1:  gen = treasure_a_crude_pile;        break;
	case 2: 
	case 3: 
	case 4:  gen = treasure_a_sharp_pile;        break;
	case 5:
	case 6:  gen = treasure_a_masterwork_pile;   break;
	case 7:  gen = treasure_a_black;             break;
	case 8:
	case 9:
	case 10: gen = treasure_d_small_pile;        break;
	case 11: gen = treasure_d_poison;            break;
	case 12: gen = treasure_d_poison_pile;       break;
	case 13: gen = treasure_b_hollow;            break;
	case 14: gen = treasure_b_hollow_pile;       break;
	case 15: 
	case 16:
	case 17: gen = treasure_b_bullet_pile;       break;
	case 18:
	case 19:
	case 20:
	case 21: gen = treasure_s_buckshot_pile;     break;
	case 22:
	case 23:
	case 24:
	case 25: gen = treasure_grenade;             break;
	case 26: gen = treasure_d_tranq;             break;
	case 27: gen = treasure_d_tranq_pile;        break;
	case 28: gen = treasure_a_flaming;           break;
      }
      break;

    case item_table_food:
      switch (tslrnd() % 7)
      {
	case 0: gen = treasure_bread;      break;
	case 1: gen = treasure_meat;       break;
	case 2: gen = treasure_fish;       break;
	case 3: gen = treasure_cheese;     break;
	case 4: gen = treasure_chickpeas;  break;
	case 5: gen = treasure_falafel;    break;
	case 6: gen = treasure_sausage;    break;
      }
      break;

    case item_table_armor:
      switch (tslrnd() % 5)
      {
	case 0:  return build_any_item("cloak", false);
	case 1:  return build_any_item("boots", false);
	case 2:
	case 3:  gen = treasure_filthy_rags;  break;
	case 4:
	case 5:  gen = treasure_leather;      break;
	case 6:  gen = treasure_chainmail;    break;
      }
      break;

    case item_table_weapon:
      switch (tslrnd() % 14)
      {
	case 0:  gen = treasure_cleaver;          break;
	case 1:  gen = treasure_spear;            break;
	case 2:  gen = treasure_wooden_stick;     break;
	case 3:  gen = treasure_bone;             break;
	case 4:  gen = treasure_crowbar;          break;
	case 5:  gen = treasure_steel_pipe;       break;
	case 6:  gen = treasure_big_ugly_knife;   break;
	case 7:  gen = treasure_broken_bottle;    break;
	case 8:  gen = treasure_stiletto;         break;
	case 9:  gen = treasure_plain_boring_sword; break;
	case 10: gen = treasure_machete;          break;
	case 11: gen = treasure_crystal_sword;    break;
	case 12: gen = treasure_quarterstaff;     break;
	case 13: gen = treasure_iron_shod_staff;  break;
      }
      break;

    case item_table_armor_magic:
      gen = pop_uniqitem(UNIQITEM_ARMOR);
      break;

/*      switch (tslrnd() % 8)
      {
	case 0:  gen = artifact_mail_of_life;              break;
	case 1:  gen = artifact_black_plate;               break;
	case 2:  gen = artifact_fire_drake_boots;          break;
	case 3:  gen = artifact_heavy_plate_mail_of_doom;  break;
	case 4:  gen = artifact_battalion_mail;            break;
	case 5:  gen = artifact_crown_of_thorns;           break;
	case 6:  gen = artifact_coral_crown;               break;
	case 7:  gen = artifact_everlasting_lantern;       break;
	}
	break;*/

    case item_table_weapon_magic:
      gen = pop_uniqitem(UNIQITEM_WEAPON);
      break;

/*      switch (tslrnd() % 15)
      {
	case 0:  gen = artifact_macabre_sabre;    break;
	case 1:  gen = artifact_stormhammer;      break;
	case 2:  gen = artifact_starflame_mace;   break;
	case 3:  gen = artifact_grim_cutter;      break;
	case 4:  gen = artifact_avengers_steel;   break;
	case 5:  gen = artifact_blood_fang;       break;
	case 6:  gen = artifact_lifetaker;        break;
	case 7:  gen = artifact_justice;          break;
	case 8:  gen = artifact_pike_of_thon;     break;
	case 9:  gen = artifact_heart_piercer;    break;
	case 10: gen = artifact_headsplitter;     break;
	case 11: gen = artifact_yyterr_durr;      break;
	case 12: gen = artifact_axe_of_the_north; break;
	case 13: gen = artifact_sahiras_cudgel;   break;
	case 14: gen = artifact_glory;            break;
      }
      break;*/

    case item_table_firearm:
      switch (tslrnd() % 6)
      {
	case 0:  gen = treasure_longbow;           break;
	case 1:  gen = treasure_crossbow;          break;
	case 2:  gen = treasure_blowgun;           break;
	case 3:  gen = treasure_hunting_rifle;     break;
	case 4:  gen = treasure_pistol;            break;
	case 5:  gen = treasure_shotgun;           break;
      }
      break;

    case item_table_special:
      switch (tslrnd() % 4)
      {
	case 0:  gen = treasure_blindfold;         break;
	case 1:  gen = treasure_gas_mask;          break;
	case 2:  gen = treasure_electric_prod;     break;
	case 3:  gen = treasure_fire_extinguisher; break;
      }
      break;

    case item_table_potion:
      switch (tslrnd() % 13)
      {
	case 0:  gen = treasure_p_instant_healing;   break;
	case 1:  gen = treasure_p_pain;              break;
	case 2:  gen = treasure_p_healing;           break;
	case 3:  gen = treasure_p_energy;            break;
	case 4:  gen = treasure_p_speed;             break;
	case 5:  gen = treasure_p_slowing;           break;
	case 6:  gen = treasure_p_polymorph;         break;
	case 7:  gen = treasure_p_levitation;        break;
	case 8:  gen = treasure_p_sleep;             break;
	case 9:  gen = treasure_p_poison;            break;
	case 10: gen = treasure_p_yuck;              break;
	case 11: gen = treasure_p_blindness;         break;
	case 12: gen = treasure_elixir;              break;
      }
      break;

    case item_table_scroll:
      return build_any_item("scroll", false);
      
    case item_table_book:
      gen = pop_uniqitem(UNIQITEM_BOOKS);
      break;
      
    case item_table_wand:
      return build_any_item("wand", false);

    case item_table_merman:
      switch (tslrnd() % 4)
      {
	case 0: gen = treasure_trident;             break;
	case 1: gen = treasure_seaweed_cloak;       break;
	case 2: gen = treasure_fish;                break;
	case 3: gen = treasure_fish_scale_armor;    break;
      }
      break;
      
    case item_table_treasure:
      switch (tslrnd() % 1)
      {
	/*case 0:  return build_any_item("amulet");*/
      }
      break;

    case item_table_bodyparts:
      gen = treasure_corpse;
      break;

      switch (tslrnd() % 2)
      {
	case 0: gen = treasure_severed_hand;            break;
	case 1: gen = treasure_decapitated_head;        break;
      }
      break;

    case item_table_bones:
      switch (tslrnd() % 3)
      {
	case 0:  gen = treasure_bone;                   break;
	case 1:  gen = treasure_cranium;                break;
	case 2:  gen = treasure_r_bone_dust;            break;
      }
      break;

    case item_table_max:
      return NULL;
  }

  return build_item(gen);
} /* random_treasure */



/*
  Builds an item suitable for LEVEL.
*/
item_t * item_for_level(const level_t * level)
{
  item_t * ret;
  item_table_t table;

  if (level == NULL)
    return NULL;

  ret = NULL;

  switch(roll(1, 41))
  {
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
    case 6:
    case 7:
    case 8:
      table = item_table_scroll;
      break;
      
    case 9:
    case 10:
      table = item_table_weapon_magic;
      break;

    case 11:
    case 12:
    case 13:
    case 15:
    case 14:
      table = item_table_weapon;
      break;

    case 16:
      table = item_table_armor_magic;
      break;

    case 17:
    case 18:
    case 19:
      table = item_table_armor;
      break;
      
    case 20:
    case 21:
    case 22:
    case 23:
    case 24:
    case 25:
      table = item_table_potion;
      break;
	
    case 26:
    case 27:
    case 28:
      table = item_table_wand;
      break;
      
    case 29:
    case 30:
    case 31:
    case 32:
    case 33:
      table = item_table_ammo;
      break;

    case 34:
    case 35:
    case 36:
      table = item_table_firearm;
      break;

    case 37:
    case 38:
      table = item_table_book;
      break;

    case 39:
    case 40:
    case 41:
    case 42:
      table = item_table_special;
      break;
  }
  
  ret = random_treasure(table);

  return ret;
} /* item_for_level */



void clear_uniqitems(game_t * g)
{
  unsigned int i;
  unsigned int j;

  if (g == NULL)
    return;

  for (i = 0; i < UNIQITEM_SETS; i++)
    for (j = 0; j < UNIQITEM_SET_SIZE; j++)
      g->uniqitem[i][j] = treasure_undefined;

  return;
} /* clear_uniqitems */



void add_uniqitem(game_t * g, const unsigned int set, const treasure_t item)
{
  unsigned int start;
  unsigned int pos;

  if (g == NULL)
    return;

  start = tslrnd() % UNIQITEM_SET_SIZE;

  pos = start + 1;

  while (pos != start)
  {
    if (g->uniqitem[set][pos] == treasure_undefined)
    {
      g->uniqitem[set][pos] = item;
      return;
    }

    pos++;
    pos %= UNIQITEM_SET_SIZE;
  }

  /* Couldn't add :( */

  return;
} /* add_uniqitem */



treasure_t pop_uniqitem(const unsigned int set)
{
  unsigned int pos;
  treasure_t ret;

  for (pos = 0; pos < UNIQITEM_SET_SIZE; pos++)
  {
    if (game->uniqitem[set][pos] != treasure_undefined)
    {
      ret = game->uniqitem[set][pos];
      game->uniqitem[set][pos] = treasure_undefined;
      return ret;
    }
  }

  return treasure_undefined;
} /* pop_uniqitem */
