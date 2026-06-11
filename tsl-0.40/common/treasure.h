/*
  treasure.h
*/

#ifndef _TREASURE_H_
#define _TREASURE_H_

#include "main.h"
#include "item.h"

enum treasure_t
{
  treasure_undefined,

  /* Corpses and bodyparts */
  treasure_corpse,
  treasure_ogre_corpse,
  treasure_carcass,
  treasure_severed_hand,
  treasure_decapitated_head,

  /* Bones and skeletons */
  treasure_cranium,

  /* Food */
  treasure_bread,
  treasure_meat,
  treasure_fish,
  treasure_cheese,
  treasure_chickpeas,
  treasure_falafel,
  treasure_sausage,
  
  /* "Clothes" */
  treasure_filthy_rags,

  /* Armor */
  treasure_leather,
  treasure_chainmail,
  treasure_rune_armor,

  /* Cloaks */
  treasure_fur_pelt,
  treasure_dark_cloak,
  treasure_wool_cloak,
  treasure_lab_coat,

  /* Headgear */
  treasure_blindfold,
  treasure_gas_mask,

  /* Boots */
  treasure_flippers,
  treasure_fur_boots,
  treasure_lead_boots,
  treasure_padded_boots,
  treasure_boots_of_speed,

  /* Ranged weapons */
  treasure_longbow,
  treasure_blowgun,
  treasure_crossbow,
  treasure_pistol,
  treasure_hunting_rifle,
  treasure_shotgun,

  /* Missiles */
  treasure_d_small,
  treasure_d_poison,
  treasure_d_tranq,
  treasure_a_crude,
  treasure_a_sharp,
  treasure_a_masterwork,
  treasure_a_black,
  treasure_a_flaming,
  treasure_b_bullet,
  treasure_b_hollow,
  treasure_s_buckshot,
  treasure_grenade,

  /* Missile piles */
  treasure_d_small_pile,
  treasure_d_poison_pile,
  treasure_d_tranq_pile,
  treasure_a_crude_pile,
  treasure_a_sharp_pile,
  treasure_a_masterwork_pile,
  treasure_s_buckshot_pile,
  treasure_b_bullet_pile,
  treasure_b_hollow_pile,

  /* Melee weapons */
  treasure_cleaver,
  treasure_spear,
  treasure_wooden_stick,
  treasure_bone,
  treasure_crowbar,
  treasure_steel_pipe,
  treasure_big_ugly_knife,
  treasure_broken_bottle,
  treasure_stiletto,
  treasure_trident,
  treasure_machete,
  treasure_plain_boring_sword,
  treasure_crystal_sword,
  treasure_quarterstaff,
  treasure_iron_shod_staff,

  /* Merman equipment */
  treasure_fish_scale_armor,
  treasure_seaweed_cloak,
  treasure_seashell_necklace,
  
  /* Light sources */
  treasure_torch,
  treasure_lantern,

  /* Scrolls */  
  treasure_s_identify,
  treasure_s_magic_mapping,
  treasure_s_trap_detection,
  treasure_s_recharge,
  treasure_s_blink,
  treasure_s_familiar,
/*  treasure_s_wish,*/
  treasure_s_mark,
  treasure_s_recall,
  treasure_s_amnesia,
  treasure_s_magic_weapon,

  /* Books */
  treasure_b_noxious_breath,
  treasure_b_breathe_fire,
  treasure_b_force_bolt,
  treasure_b_frost_ray,
  treasure_b_deathspell,
  treasure_b_flash,

  treasure_b_traps,
  treasure_b_camouflage,
  treasure_b_first_aid,
  treasure_b_pharmacy,

  /* Wands */
  treasure_w_destroy_trap,
  treasure_w_fireball,
  treasure_w_force_bolt,
  treasure_w_frost_ray,
  treasure_w_enslavement,
  treasure_w_pushing,
  treasure_w_phasing,
  treasure_electric_prod,

  /* Potions */
  treasure_elixir,
  treasure_p_healing,
  treasure_p_instant_healing,
  treasure_p_pain,
  treasure_p_energy,
  treasure_p_speed,
  treasure_p_poison,
  treasure_p_slowing,
  treasure_p_polymorph,
  treasure_p_levitation,
  treasure_p_sleep,
  treasure_p_yuck,
  treasure_p_blindness,

  /* Tools */
  treasure_dataprobe,
  treasure_fire_extinguisher,
  treasure_key,
  
  /* Spell reagents */
  treasure_r_bone_dust,
  treasure_r_scales,
  treasure_r_sulfur,
  treasure_r_web,
  treasure_r_eyeball,
  treasure_r_mandrake_root,
  treasure_r_mushroom,

  /* Special spell items; these should never appear on the map. */
  treasure_force_bolt,
  treasure_mudball,
  treasure_fireball,
  treasure_doomblade,
/*  treasure_hungry_book,*/

  /* Debug items */
  treasure_crappy_weapon,

  /* Artifact weapons */
  artifact_macabre_sabre,
  artifact_stormhammer,
  artifact_starflame_mace,
  artifact_grim_cutter,
  artifact_avengers_steel,
  artifact_blood_fang,
  artifact_lifetaker,
  artifact_justice,
  artifact_pike_of_thon,
  artifact_heart_piercer,
  artifact_headsplitter,
  artifact_whip_of_thorns,
  artifact_axe_of_the_north,
  artifact_sahiras_cudgel,
  artifact_glory,
  artifact_yyterr_durr,
  artifact_axe_trollish,

  /* Artifact headgear */
  artifact_crown_of_thorns,
  artifact_coral_crown,

  /* Artifact armor */
  artifact_mail_of_life,
  artifact_black_plate,
  artifact_fire_drake_boots,
  artifact_heavy_plate_mail_of_doom,
  artifact_battalion_mail,

  /* Special */
  artifact_runestaff,
  artifact_dragon_scales,

  /* Crap */
  artifact_ring_of_the_archmage,
  artifact_rogues_ring_escape,
  artifact_archers_gloves,
  artifact_bracers_of_war_spirit,
  artifact_gloves_of_slaying,

  /* Misc. artifact */
  artifact_everlasting_lantern,

  treasure_max
};
typedef enum treasure_t treasure_t;

/*#define ARTIFACTS 30*/
#define ITEMS 200

enum item_table_t
{
  item_table_supply,
  item_table_special,
  item_table_armor,
  item_table_armor_magic,
  item_table_weapon,
  item_table_weapon_magic,
  item_table_firearm,
  item_table_food,
  item_table_potion,
  item_table_scroll,
  item_table_book,
  item_table_wand,
  item_table_treasure,
  item_table_ammo,
  item_table_bodyparts,
  item_table_bones,
  item_table_merman,
  item_table_max
};
typedef enum item_table_t item_table_t;

item_t * build_any_item(const char * name, const blean_t artifacts);
item_t * build_item(const treasure_t item);
item_t * random_treasure(const item_table_t item_table);
item_t * item_for_level(const level_t * level);

void init_item_templates(game_t * g);
void del_item_templates(game_t * g);
item_t * build_item_template(game_t * g, const treasure_t item);

#define UNIQITEM_BOOKS     0
#define UNIQITEM_ARMOR     1
#define UNIQITEM_WEAPON    2
#define UNIQITEM_SET_SIZE  50
#define UNIQITEM_SETS      5

void clear_uniqitems(game_t * g);
void add_uniqitem(game_t * g, const unsigned int set, const treasure_t item);
treasure_t pop_uniqitem(const unsigned int set);

#endif
