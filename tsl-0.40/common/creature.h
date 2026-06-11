/*
  creature.h
*/

#ifndef _CREATURE_H_
#define _CREATURE_H_

#include "main.h"
#include "treasure.h"
#include "magic.h"
#include "unique.h"
#include "gent.h"
#include "vweapon.h"

#define FOV_RANGE 40


enum ai_state_t
{
  ai_idle,
  ai_mimic,
  ai_offensive,
  ai_retreat,
  ai_target,
  ai_player
};
typedef enum ai_state_t ai_state_t;


/*enum alignment_t
{
  alignment_enemy,
  alignment_player,
  alignment_chaotic,
  alignment_neutral
};
typedef enum alignment_t alignment_t;*/


enum altitude_t
{
  altitude_walking,
  altitude_floating,
  altitude_swimming
};
typedef enum altitude_t altitude_t;


struct creature_t
{
  /*
    _only is the name of the species. _one and _the should include
    indefinitive singular and definitive article. For uniques all
    fields should be identical and usually start with "the".
    See set_creature_name().    
  */
  char name_one[31];
  char name_the[31];
  char name_only[31];

  monster_t id; /* Monster type */

  int uid; /* Unique for this individual */

  gent_t gent;

  /* */
  signed int attack_pos;
  signed int next_attack;

  uint16_t parts;

  /*
    When the creature is killed in combat, this message will be
    concatenated to the creatures name.
  */
  char * death_msg;

  /* AI-related */
  ai_state_t ai_state;
  unsigned int target_y;
  unsigned int target_x;

  blean_t fov[FOV_RANGE * 2 + 1][FOV_RANGE * 2 + 1];

  blean_t detected;

  altitude_t altitude;

  treasure_t corpse;

  level_t * location;

  blean_t retreats;
  blean_t pacifist;
  blean_t humanoid;

  item_t * first_item;
/*  item_t * magic_weapon;*/
  
  unsigned int y;
  unsigned int x;

  unsigned int lifetime;

/*  alignment_t alignment;*/

  signed int move_counter;
  unsigned int fatigue_counter;
  unsigned int poison_counter;
  unsigned int ep_counter;

  attribute_t attr_base[ATTRIBUTES];

  virtual_weapon_t unarmed;

  effect_t * first_effect;

  unsigned int multiaction;
  unsigned int multi_param;
};

#define MULTIACTION_NONE          0
#define MULTIACTION_REST          1
#define MULTIACTION_BREAK_REST    2
#define MULTIACTION_AIM           3


/* (De)allocation */
creature_t * alloc_creature(void);
void del_creature(creature_t * creature);
creature_t * clone_creature(const creature_t * creature);

/* Name related */
void set_creature_name(creature_t * creature, const char * new_one, const char * new_the, const char * new_only);

/* Checking misc. things. These are all in checks.c */
blean_t is_player(const creature_t * creature);
blean_t is_unique(const creature_t * creature);
blean_t is_helpless(const creature_t * creature);
blean_t is_humanoid(const creature_t * creature);
blean_t is_blinded(const creature_t * creature);
blean_t is_stunned(const creature_t * creature);
blean_t is_awake(const creature_t * creature);
blean_t is_confused(const creature_t * creature);
blean_t is_mimic(const creature_t * creature);
blean_t enemies(const creature_t * a, const creature_t * b);

/* Getting/setting attributes - these are in attrs.c */
attribute_t attr_base(const creature_t * creature,
		      const attr_index_t attr_index);
signed int attr_facet(const creature_t * creature,
		      const attr_index_t attr_index);
signed int attr_eq_mods(const creature_t * creature,
			const attr_index_t attr_index,
			const blean_t discriminate);
signed int attr_fx_mods(const creature_t * creature,
			const attr_index_t attr_index);
attribute_t attr_known(const creature_t * creature,
		       const attr_index_t attr_index);
attribute_t attr_current(const creature_t * creature,
			 const attr_index_t attr_index);
void set_attr(creature_t * creature,
	      const attr_index_t attr_index,
	      const attribute_t value);

/* Attaching and detaching */
creature_t * attach_creature(level_t * level, creature_t * creature);
creature_t * detach_creature(creature_t * creature);

/* Coordinates & placing */
blean_t move_creature(creature_t * creature,
		      const signed int move_y,
		      const signed int move_x,
		      const blean_t forced);
void set_creature_coordinates(creature_t * creature,
			      const unsigned int y, const unsigned int x);
void find_random_free_spot(creature_t * creature);
void find_nearest_free_spot(creature_t * creature,
			    const unsigned int desired_y,
			    const unsigned int desireed_x);

/* Game mechanics - most of this should probably be migrated to actions.* */
unsigned int heal(creature_t * creature, const unsigned int amount);
signed int damage(creature_t * creature, const unsigned int amount);
unsigned int regain_ep(creature_t * creature, const unsigned int amount);
blean_t killed(const creature_t * creature);
void kill_creature(creature_t * creature, const blean_t bleed);
void spend_ep(creature_t * creature, const unsigned int amount);

void set_temp_weapon(creature_t * creature,
		     const signed int vweapon,
		     const unsigned int ttl);

void set_death_msg(creature_t * creature, const char * s);

item_t * get_unarmed_weapon(const creature_t * creature);

#endif
