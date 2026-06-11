#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "main.h"
#include "vweapon.h"
#include "item.h"



/*
  Initiates the list of virtual weapons.
*/
void init_virtual_weapons()
{
  int i;
  struct item_t * temp;

  for (i = 0; i < VIRTUAL_WEAPONS; i++)
  {
    virtual_weapon[i] = alloc_item();
    
    if (virtual_weapon[i] == NULL)
      out_of_memory();

    virtual_weapon[i]->id = known_name;
    virtual_weapon[i]->indestructible = true;
  }


  temp = virtual_weapon[virtual_fists];
  set_item_single_name(temp, "fists", "fists");
  temp->single_unid_article = temp->single_id_article = article_i_none;
  temp->item_type = item_type_m_weapon;
  temp->custom[WEAPON_DAMAGE_TYPE]    = damage_general;
  temp->custom[WEAPON_ATTACK_STRING]  = attack_string_punches;
  temp->custom[WEAPON_FINISH]         = gore_unarmed;

  MKATKSEQ(temp, -2, -2, -1, 1, 2, 0, 0, 0, 0, 0)


  temp = virtual_weapon[virtual_slap];
  set_item_single_name(temp, "Slap o' Death", "Slap o' Death");
  temp->single_unid_article = temp->single_id_article = article_i_none;
  temp->item_type = item_type_m_weapon;
  temp->custom[WEAPON_DAMAGE_TYPE]    = damage_general;
  temp->custom[WEAPON_ATTACK_STRING]  = attack_string_slaps;

  MKATKSEQ(temp, -2, -2, -1, 3, 5, 0, 0, 0, 0, 0)
      

  temp = virtual_weapon[virtual_cabbage_hands];
  set_item_single_name(temp, "cabbage hands", "cabbage hands");
  temp->single_unid_article = temp->single_id_article = article_i_none;
  temp->item_type = item_type_m_weapon;
  temp->custom[WEAPON_DAMAGE_TYPE]    = damage_general;
  temp->custom[WEAPON_ATTACK_STRING]  = attack_string_slaps;

  MKATKSEQ(temp, -2, -2, -1, 2, 5, 7, 9, 0, 0, 0)


  temp = virtual_weapon[virtual_claws];
  set_item_single_name(temp, "claws", "claws");
  temp->single_unid_article = temp->single_id_article = article_i_none;
  temp->item_type = item_type_m_weapon;
  temp->attr_mod[attr_i_wound]               = 10;
  temp->custom[WEAPON_DAMAGE_TYPE]    = damage_general;
  temp->custom[WEAPON_ATTACK_STRING]  = attack_string_claws;
  temp->custom[WEAPON_FINISH]         = gore_claw;

  MKATKSEQ(temp, -2, -2, -1, 2, 3, 4, 0, 0, 0, 0)


  temp = virtual_weapon[virtual_blade_hands];
  set_item_single_name(temp, "blade hands", "blade hands");
  temp->single_unid_article = temp->single_id_article = article_i_none;
  temp->item_type = item_type_m_weapon;
  temp->attr_mod[attr_i_wound]        = 20;
  temp->custom[WEAPON_DAMAGE_TYPE]    = damage_general;
  temp->custom[WEAPON_ATTACK_STRING]  = attack_string_claws;
  temp->custom[WEAPON_FINISH]         = gore_claw;

  MKATKSEQ(temp, -2, -2, -1, 3, 6, 9, 12, 15, 0, 0)


  temp = virtual_weapon[virtual_fangs];
  set_item_single_name(temp, "fangs", "fangs");
  temp->single_unid_article = temp->single_id_article = article_i_none;
  temp->item_type = item_type_m_weapon;
  temp->attr_mod[attr_i_wound]               = 10;
  temp->custom[WEAPON_DAMAGE_TYPE]    = damage_general;
  temp->custom[WEAPON_ATTACK_STRING]  = attack_string_bites;

  MKATKSEQ(temp, -2, -2, -1, 2, 2, 6, 0, 0, 0, 0)


  temp = virtual_weapon[virtual_poison_fangs];
  set_item_single_name(temp, "poison fangs", "poison fangs");
  temp->single_unid_article = temp->single_id_article = article_i_none;
  temp->item_type = item_type_m_weapon;
  temp->custom[WEAPON_DAMAGE_TYPE]    = damage_poison;
  temp->custom[WEAPON_ATTACK_STRING]  = attack_string_bites;

  MKATKSEQ(temp, -2, -2, -1, 4, 2, 6, 0, 0, 0, 0)


  temp = virtual_weapon[virtual_slime];
  set_item_single_name(temp, "slime", "slime");
  temp->single_unid_article = temp->single_id_article = article_i_none;
  temp->item_type = item_type_m_weapon;
  temp->custom[WEAPON_DAMAGE_TYPE]    = damage_acid;
  temp->custom[WEAPON_ATTACK_STRING]  = attack_string_slimes;

  MKATKSEQ(temp, -2, -2, -1, 2, 4, 8, 16, 32, 0, 0)


  temp = virtual_weapon[virtual_cold_touch];
  set_item_single_name(temp, "cold touch", "cold touch");
  temp->single_unid_article = temp->single_id_article = article_i_none;
  temp->item_type = item_type_m_weapon;
  temp->custom[WEAPON_DAMAGE_TYPE]    = damage_cold;
  temp->custom[WEAPON_ATTACK_STRING]  = attack_string_touches;

  MKATKSEQ(temp, -2, -2, -1, 6, 3, 2, 0, 0, 0, 0)


  temp = virtual_weapon[virtual_ghoul_touch];
  set_item_single_name(temp, "ghoul touch", "ghoul touch");
  temp->single_unid_article = temp->single_id_article = article_i_none;
  temp->item_type = item_type_m_weapon;
  temp->custom[WEAPON_DAMAGE_TYPE]    = damage_cold;
  temp->custom[WEAPON_ATTACK_STRING]  = attack_string_touches;

  MKATKSEQ(temp, -2, -2, -1, 4, 2, 4, 0, 0, 0, 0)


  temp = virtual_weapon[virtual_draining_touch];
  set_item_single_name(temp, "draining touch", "draining touch");
  temp->single_unid_article = temp->single_id_article = article_i_none;
  temp->item_type = item_type_m_weapon;
  temp->attr_mod[attr_i_drain]               = 100;
  temp->custom[WEAPON_DAMAGE_TYPE]    = damage_general;
  temp->custom[WEAPON_ATTACK_STRING]  = attack_string_touches;

  MKATKSEQ(temp, -2, -2, -1, 4, 2, 4, 0, 0, 0, 0)


  temp = virtual_weapon[virtual_sting];
  set_item_single_name(temp, "sting", "sting");
  temp->single_unid_article = temp->single_id_article = article_i_none;
  temp->item_type = item_type_m_weapon;
  temp->custom[WEAPON_DAMAGE_TYPE]    = damage_poison;
  temp->custom[WEAPON_ATTACK_STRING]  = attack_string_stings;

  temp = virtual_weapon[virtual_kick];
  set_item_single_name(temp, "kick", "kick");
  temp->single_unid_article = temp->single_id_article = article_i_none;
  temp->item_type = item_type_m_weapon;
  temp->attr_mod[attr_i_knockback] = 50;
  temp->custom[WEAPON_DAMAGE_TYPE]    = damage_general;
  temp->custom[WEAPON_ATTACK_STRING]  = attack_string_kicks;

  temp = virtual_weapon[virtual_flame_hands];
  set_item_single_name(temp, "flame hands", "flame hands");
  temp->single_unid_article = temp->single_id_article = article_i_none;
  temp->item_type = item_type_m_weapon;
  temp->custom[WEAPON_DAMAGE_TYPE]    = damage_fire;
  temp->custom[WEAPON_ATTACK_STRING]  = attack_string_burns;

  MKATKSEQ(temp, -2, -2, -1, 3, 5, 5, 5, 0, 0, 0)


  temp = virtual_weapon[virtual_book];
  set_item_single_name(temp, "book", "book");
  temp->single_unid_article = temp->single_id_article = article_i_none;
  temp->item_type = item_type_m_weapon;
  temp->custom[WEAPON_DAMAGE_TYPE]    = damage_general;
  temp->custom[WEAPON_ATTACK_STRING]  = attack_string_hits;

  MKATKSEQ(temp, -1, -1, -1, 1, 1, 1, 1, 1, 2, 0)


  temp = virtual_weapon[virtual_chainsaw];
  set_item_single_name(temp, "chainsaw", "chainsaw");
  temp->single_unid_article = temp->single_id_article = article_i_none;
  temp->item_type = item_type_m_weapon;
  temp->attr_mod[attr_i_wound] = 50;
  temp->custom[WEAPON_DAMAGE_TYPE]    = damage_general;
  temp->custom[WEAPON_ATTACK_STRING]  = attack_string_saws;

  MKATKSEQ(temp, -2, -1, -1, 3, 3, 5, 5, 7, 7, 0)


  temp = virtual_weapon[virtual_mimic_fangs];
  set_item_single_name(temp, "mimic fangs", "mimic fangs");
  temp->single_unid_article = temp->single_id_article = article_i_none;
  temp->item_type = item_type_m_weapon;
  temp->custom[WEAPON_DAMAGE_TYPE]    = damage_general;
  temp->custom[WEAPON_ATTACK_STRING]  = attack_string_bites;
  temp->attr_mod[attr_i_stun]                = 70;

  MKATKSEQ(temp, -2, -2, -1, 2, 2, 4, 4, 8, 8, 0)


  temp = virtual_weapon[virtual_dragon];
  set_item_single_name(temp, "dragon claws", "dragon claws");
  temp->single_unid_article = temp->single_id_article = article_i_none;
  temp->item_type = item_type_m_weapon;
  temp->custom[WEAPON_DAMAGE_TYPE]    = damage_general;
  temp->custom[WEAPON_ATTACK_STRING]  = attack_string_claws;
  temp->attr_mod[attr_i_knockback]    = 80;

  MKATKSEQ(temp, -2, -1, -1, 3, 6, 12, 0, 0, 0, 0)



  temp = virtual_weapon[virtual_hungry_book];
  set_item_single_name(temp, "hungry book", "hungry book");
  temp->single_unid_article = temp->single_id_article = article_i_none;
  temp->item_type = item_type_m_weapon;
  temp->custom[WEAPON_DAMAGE_TYPE]    = damage_general;
  temp->custom[WEAPON_ATTACK_STRING]  = attack_string_hits;

  MKATKSEQ(temp, -1, -1, -1, 1, 1, 1, 1, 1, 2, 0)

  temp = NULL;

  return;
} /* init_virtual_weapons */



/*
  Deletes all virtual weapons. This should be called after all
  creatures in the world have been deleted, since they otherwise would
  point to invalid memory locations.
*/
void del_virtual_weapons()
{
  int i;

  for (i = 0; i < VIRTUAL_WEAPONS; i++)
  {
    del_item(virtual_weapon[i]);
    virtual_weapon[i] = NULL;
  }

  return;
} /* del_virtual_weapons */
