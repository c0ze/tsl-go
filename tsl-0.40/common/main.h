/*
  main.h

  Throughout the code are TODO and RFE notes. TODOs are things that
  are known to be broken, ugly or inefficient, or where we just
  haven't decided what to do. RFEs are things that work as intended,
  could be improved further, but are low priority.
*/

#ifndef _MAIN_H_
#define _MAIN_H_

#define TSL_NAME        "The Slimy Lichmummy"
#define TSL_VERSION     "0.40"
#define SAVE_FILENAME   "TSL-SAVE"
#define CONFIG_NAME     ".tsl_conf"
#define MORGUE_NAME     "morgue.txt"

extern const char * bug_string;

typedef struct item_t item_t;
typedef signed int attribute_t;
typedef struct tile_info_t tile_info_t;
typedef struct level_t level_t;
typedef struct trap_t trap_t;
typedef struct creature_t creature_t;
typedef struct message_t message_t;
typedef struct effect_t effect_t;
typedef struct area_t area_t;
typedef struct career_opt_t career_opt_t;
typedef struct game_t game_t;


/*
  This is a global structure for keeping track of why room generation
  fails. We only need one of these.
*/
struct
{
  unsigned int calls;       /* How many calls to alloc_level() that have been made */
  unsigned int generated;   /* How many levels have been generated */
  unsigned int traversable; /* How many that had insufficient traversable area */
  unsigned int connected;   /* How many that had disjoint areas */ 
  unsigned int exits;       /* How many that lacked exits (stairs, portal) */
} map_test;



/*
  This is a global structure to keep track of how many structures of
  each kind are currently allocated. Whenever a structure listed here
  is allocated or freed, the corresponding member variable should be
  incremented or decremented. If any of these are != 0 at program
  exit, there is an error somewhere in the program (whoa, now *that*
  was specific). We only need one of these.
*/
struct
{
  unsigned int chars;
  unsigned int creatures;
  unsigned int items;
  unsigned int levels;
  unsigned int tiles;
  unsigned int traps;
  unsigned int messages;
  unsigned int effects;
  unsigned int attributes;
  unsigned int games;
} mem_alloc;



#undef true
#undef false
#undef maybe

enum blean_t
{
  false = 0,
  true = 1
};
typedef enum blean_t blean_t;



#include "rolls.h"



enum gore_t
{
  gore_none,
  gore_unarmed,
  gore_crush,
  gore_chop,
  gore_burn,
  gore_stab,
  gore_cut,
  gore_electro,
  gore_impale,
  gore_claw
};
typedef enum gore_t gore_t;



enum parts_t
{
  part_head = 1,
  part_eyes = 2,
  part_ribs = 4,
  part_neck = 8,
  part_bowel = 16,
  part_teeth = 32,
  part_tail = 64,
  part_skull = 128,
  part_throat = 256,
  part_
};
typedef enum parts_t parts_t;



enum lightning_type_t
{
  lightning_bolt,
  lightning_trap,
  lightning_wand
};
typedef enum lightning_type_t lightning_type_t;



enum monster_t
{
  monster_ghoul,
  monster_graveling,
  monster_gnoblin,
  monster_ratman,
  monster_slime,
  monster_hellhound,
  monster_severed_hand,
  monster_burning_skull,
  monster_sludge_dweller,
  monster_crypt_vermin,
  monster_gloom_lord,
  monster_imp,
  monster_drowned_one,
  monster_frostling,
  monster_chainsaw_ogre,
  monster_goatman,
  monster_giant_slimy_toad,
  monster_electric_snake,
  monster_chrome_angel,
  monster_scarecrow,
  monster_merman,
  monster_nameless_horror,
  monster_elder_mummylich,
  monster_floating_demon_genitalia,
  monster_sentinel,
  monster_technician,

  /* Put all monsters we *don't* want to shapeshift into after this point. */
  monster_max,

  monster_player,

  monster_phantasm,
  monster_floating_brain,
  monster_tentacle,
  monster_flame_spirit,
  monster_mimic,

  unique_first,
  unique_argor,
  unique_caeltzan,
  unique_lognac,
  unique_gaoler,
  unique_necromancer,
  unique_dragon,
  unique_ghrazghaar,
  unique_ybznek,
  unique_sulkor,
  unique_chaajd,
  unique_king_of_worms,
  unique_lurker,
  unique_last,

  /* These are *only* available as shapeshifts */
  monster_silver_wolf,
  monster_moon_wolf,

  monster_undefined,

  monsters
};
typedef enum monster_t monster_t;



enum damage_type_t
{
  damage_general,
  damage_fire,
  damage_cold,
  damage_acid,
  damage_electricity,
  damage_poison
};
typedef enum damage_type_t damage_type_t;

#define DAMAGE_TYPES 7
extern char * damage_type_label[];



extern char * item_letters;



/* RFE: I think this should be removed in favour of dir_to_speed()! */
#define DIRECTIONS 9
extern signed int move_mat[DIRECTIONS][2];



enum dir_t
{
  dir_none,
  dir_n,
  dir_ne,
  dir_e,
  dir_se,
  dir_s,
  dir_sw,
  dir_w,
  dir_nw
};
typedef enum dir_t dir_t;



enum attr_index_t
{
  attr_killed,  /* Set to nonzero if creature killed */

  /* How much we're capable of carrying before burdened; hard limit is the double */
  attr_carrying_capacity,

  attr_health,
/*  attr_fatigue,
    attr_fatigue_limit,*/
  attr_damage_total, /* Total damage taken in the whole game */
  attr_recovery, /* How fast we regenerate heath (usually zero) */

  attr_ep_current,
  attr_ep_max,

  attr_player_ally,

  attr_swim_fatigue, /* How many turns we've been swimming; reset on land/in air */
  attr_swimming, /* How many turns before we take damage from swimming */
  attr_free_swim, /* Remain in water indefinitely */

  attr_poisoned,
  attr_poison_resistance,

  attr_wound_immunity,
  attr_wounded,

  attr_nonbreathing, /* Doesn't breathe; immune to gas, drowning, etc */
  attr_gas_immunity, /* Immune to gas; can still drown, etc. */

  attr_p_sleep, /* Immunity to sleep */
  
  attr_speed,
  attr_stealth,
  attr_perception,

  attr_magic,
  attr_dodge,
  attr_attack,

  attr_backstab_bonus, /* Cumulative with (and only with) weapon backstab */
  attr_backstab_immunity, /* Completely prevents backstabbing */

  attr_vision, /* How many steps we can see */

  attr_throw_range, /* How many steps we can throw */

  attr_dualwield, /* Allow dualwield */

  /* Setting these nonzero prevent us from performing certain actions */
  attr_p_pick_up,
  attr_p_open_doors,
  attr_p_throw,
  attr_p_move,
  attr_p_eat,
  attr_p_drink,
  attr_p_invoke,
  attr_p_read,

  attr_blindness, /* Cannot see */
  attr_levitate, /* Floating; temporary */
  attr_permafloat, /* Floating; permanent */
  attr_permaswim, /* Prefers not getting out of the water. Not the same as attr_free_swim */
  attr_unchanging, /* Cannot shapeshift */
  
  attr_feet_protected, /* Protects against certain traps */

  attr_absorption, /* Static damage reduction */

  attr_fire_resistance,
  attr_cold_resistance,
  attr_acid_resistance,
  attr_electricity_resistance,
  
  /* Skills */
  attr_s_trap_detection,

  /*
    These only apply to items and are never needed for
    get_attr_current(), etc.
  */
  attr_i_precision, /* Accuracy modifier, only applies when using this weapon */
  attr_i_drain, /* Chance in % of draining 1 HP from enemy */
  attr_i_blink, /* Chance in % of blinking enemy away */
  attr_i_knockback, /* Chance in % of knocking enemy back */
  attr_i_instadeath, /* Chance in % of instantly killing enemy */
  attr_i_stun, /* Chance in % of stunning enemy */
  attr_i_wound, /* For melee weapons: chance in % of wounding enemy, for missiles: binary */
  attr_i_backstab, /* Chance in % of backstabbing enemy */
  attr_i_poison, /* Binary; will this weapon poison the enemy? */
  /* TODO: attr_i_horror */ /* Chance in % of enemy being affected by horror */

  /* attr_a_* (abilities) and attr_m_* (magic): these are binary (you have them, or you don't). */
  attr_a_leap,
  attr_a_shape_native,
  attr_a_aimed_shot,
  attr_a_dash,
  attr_a_yell,
  attr_a_interface,
  attr_a_hide,

  attr_m_shapeshift_wolf,
  attr_m_mudball,
  attr_m_cure_poison,
  attr_m_fireball,
  attr_m_first_aid,
  attr_m_frost_ray,
  attr_m_deathspell,
  attr_m_teleport,
  attr_m_phase,
  attr_m_destroy_trap,
  attr_m_recharge,
  attr_m_reveal_traps,
  attr_m_blink,
  attr_m_magic_mapping,
  attr_m_force_bolt,
  attr_m_magic_weapon,
  attr_m_summon_familiar,
  attr_m_bone_crush,
  attr_m_wish,
  attr_m_enslave,
  attr_m_mark,
  attr_m_recall,
  attr_m_identify,
  attr_m_noxious_breath,
  attr_m_breathe_fire,
  attr_m_flash,
  attr_m_sticky_web,
  attr_m_amnesia,
  attr_m_push,
  attr_m_shock,

  attr_dissolve
};
typedef enum attr_index_t attr_index_t;

#define ATTRIBUTES 200



struct attr_info_t
{
  char * name;
  signed long min;
  signed long max;
  blean_t percent;

  /* Should this be displayed in the morgue? */
  blean_t morgue;

  /* Should this be displayed in item inspection? */
  blean_t item_data;

  /*
    Player only: Should this be inherited from the native shape, even
    when shapeshifted?
  */
  blean_t inherit;
  
  /* Default value when building new creatures. */
  signed long def;

  /*
    Should point to a function to be called when an ability is
    invoked. It should return true if the ability was used; e.g. the
    user completed the action, or a source item lost one charge. It
    should return false if the action was cancelled (many wands are
    stingy about this, as they will often cost a charge even if the
    targetting was cancelled - this is the cost of zap-IDing). The
    return value does not need to reflect whether the *effects* of the
    ability were successful or not, only that an attempt was made.
    Should be NULL for passive attributes.
  */
  blean_t (* invoke) (creature_t * caster, item_t * source, signed int param);

  /*
    The EP cost for using this ability (invoked items ignore this, but
    usually have charges instead).
  */
  unsigned int cost;

  /*
    The key used to select this ability - no real use outside
    select_ability().
  */
  char key;
} * attr_info[ATTRIBUTES];
typedef struct attr_info_t attr_info_t;


/* Colors */
enum color_t
{
  color_normal,
  color_brown,
  color_blue,
  color_red,
  color_green,
  color_cyan,
  color_magenta,
  color_black
};
typedef enum color_t color_t;




/* Memory functions */
void check_unfree_memory(void);
void out_of_memory(void);

/* Start/stop */
void init_static(void);
void del_static(void);
void shutdown_everything(void);

#endif
