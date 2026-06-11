/*
  item.h - what items are and how they work.

  There are special item types that never appear in
  inventories.

  item_type_spell is used for instantly fired missiles and other
  temporary items when casting spells.

  item_type_career is used internally for keeping track of careers the
  player has taken. IF WE WERE USING OOP THIS COULD BE ABSTRACTED INTO
  A SUPERCLASS WITH ONLY ATTRIBUTE MODIFIERS + NAME AND WE WOULDN'T
  HAVE TO WASTE A FULL ITEM STRUCTURE. Too bad we aren't.

  We're hardly 100% strict about terminology around here, but "item"
  usually refers to any item in general (or the item_t structure
  itself), while "treasure" means a specific instance of a certain
  item number, or the treasure index itself.

  See also: inventory.*, treasure.*, artifact.*, identify.*, stacks.*
*/

#ifndef _ITEM_H_
#define _ITEM_H_

#include "main.h"
#include "gent.h"
#include "identify.h"


typedef blean_t (*use_func_t)(creature_t *, item_t *);


#define MKATKSEQ(var, a, b, c, d, e, f, g, h, i, j) \
  var->custom[WEAPON_ATKSEQ + 0] = a; \
  var->custom[WEAPON_ATKSEQ + 1] = b; \
  var->custom[WEAPON_ATKSEQ + 2] = c; \
  var->custom[WEAPON_ATKSEQ + 3] = d; \
  var->custom[WEAPON_ATKSEQ + 4] = e; \
  var->custom[WEAPON_ATKSEQ + 5] = f; \
  var->custom[WEAPON_ATKSEQ + 6] = g; \
  var->custom[WEAPON_ATKSEQ + 7] = h; \
  var->custom[WEAPON_ATKSEQ + 8] = i; \
  var->custom[WEAPON_ATKSEQ + 9] = j; \
  var->custom[WEAPON_ATKSEQ + 10] = 0;


/* I'm not sure if this should really be called "articles", but never mind. */
extern char * item_article_a[]; /* there is "an" item here */
extern char * item_article_of[]; /* there is a "pile of" items here */
extern char * item_article_is[]; /* there "is" an item */
extern char * item_article_it[]; /* "it" is yellow */
extern char * item_article_that[]; /* "that" item is yellow */
extern char * item_article_has[]; /* that item "has" a particular odor to it */



enum item_article_t
{
  article_i_a,
  article_i_an,
  article_i_a_pair_of,
  article_i_a_set_of,
  article_i_some_is,
  article_i_some_are,
  article_i_none,
  article_i_a_pile_of,
  article_i_a_n_x,
  article_i_the
};
typedef enum item_article_t item_article_t;



enum item_type_t
{
  item_type_none,
  item_type_m_weapon, /* melee */
  sort_type_bow,
  sort_type_arrow,
  sort_type_shotgun,
  sort_type_shell,
  sort_type_crossbow,
  sort_type_dart,
  sort_type_pistol,
  sort_type_bullet,
  item_type_r_weapon, /* ranged */
  item_type_ammo,
  item_type_body,
  item_type_feet,
  item_type_head,
  item_type_cloak,
  item_type_scroll,
  item_type_book,
  item_type_potion,
  item_type_light,
  item_type_tool,
  item_type_wand,
  item_type_misc,
  item_type_food,
  item_type_spell, /* internal */
  item_type_facet, /* internal */
  item_type_aug, /* internal */
  item_type_nofilter, /* internal */
  item_types
};
typedef enum item_type_t item_type_t;



#define CUSTOM_FIELDS 50

#define BOOK_ABILITY 0

#define WEAPON_MIN_DAMAGE     0
#define WEAPON_MAX_DAMAGE     1
#define WEAPON_DAMAGE_TYPE    2
#define WEAPON_TWOHANDED      4
#define WEAPON_ATTACK_STRING  8
#define WEAPON_AMMO_TYPE      9
#define WEAPON_RANGE          10
#define WEAPON_BREAK          11
#define WEAPON_FINISH         13
#define WEAPON_FINISH_ALT     14

#define WEAPON_ATKSEQ         16
#define WEAPON_ATKSEQ_ID      40

#define WEAPON_ATKSEQLEN      11

#define ARMOR_PROTECTION      0
#define ARMOR_DURABILITY      1
#define ARMOR_MAX_DURABILITY  2

#define CUSTOM_LIGHT_TICKS        14
#define CUSTOM_LIGHT_MAX_TICKS    15

#define CUSTOM_AMMO_DTYPE       2
#define CUSTOM_AMMO_ATYPE       3
#define CUSTOM_AMMO_EXPLOSIVE   15




enum ammo_types
{
  ammo_type_arrow,
  ammo_type_bolt,
  ammo_type_dart,
  ammo_type_bullet,
  ammo_type_shell,
  ammo_type_rock,
  ammo_type_grenade
};



enum attack_string_t
{
  attack_string_hits,
  attack_string_claws,
  attack_string_bites,
  attack_string_stings,
  attack_string_slaps,
  attack_string_slimes,
  attack_string_burns,
  attack_string_kicks,
  attack_string_stabs,
  attack_string_punches,
  attack_string_saws,
  attack_string_whips,
  attack_string_touches
};
typedef enum attack_string_t attack_string_t;



enum item_verb_t
{
  item_verb_bug,
  item_verb_remove,
  item_verb_put_on,
  item_verb_take_off,
  item_verb_wield,
  item_verb_put_away,
  item_verb_ready,
  item_verb_use,
  item_verb_drink,
  item_verb_read,
  item_verb_eat,
  item_verb_zap,
  item_verb_light,
  item_verb_put_out
};
typedef enum item_verb_t item_verb_t;

extern char * item_verbs[];



struct item_t
{
  /*
    Identifies what kind of item this is. This is just so that each
    item has a unique number so we can easily tell them apart, there's
    no hidden magic behind the number.

    For careers this is instead the career index.
  */
  unsigned int item_number;
  
  /* What this item should be displayed as on the board. */
  gent_t gent;

  /* true if only one of this item should ever be generated. */
  blean_t artifact;

  /* If true this item can't be wished for. */
  blean_t no_wish;

  /*
    What type of item this is; e.g. a weapon, a scroll, etc. See
    item_type_* for a complete list.
  */
  item_type_t item_type;
  item_type_t sort_type;

  /*
    What letter this item has in any inventory it happens to be in at
    the time. If it's not in an inventory, the letter could be
    anything and should not be used in any way.
  */
  char letter;

  /* The player-assigned label for this item. */
  char * label;

  /*
    These are the names that this item will appear as before
    (*_unid_name) and after (*_id_name) it has been
    identified. single_* should be used when there's just one item,
    plural_* when there are two or more. Converting from single to
    plural should be trivial, but there are non-trivial exceptions
    that discourage us from doing this automatically. We'll just
    manually enter both the single and plural name for each item
    instead.
  */
  char * single_unid_name;
  char * plural_unid_name;
  char * single_id_name;
  char * plural_id_name;

  item_article_t single_unid_article;
  item_article_t plural_unid_article;
  item_article_t single_id_article;
  item_article_t plural_id_article;

  /* This description will be displayed when inspecting an identified item. */
  char * description;

  /*
    How many ticks this item should remain in the game. This is
    currently just used for conjured items; items on the ground or in
    a creatures inventory *don't* care about this. If zero, the item
    is permanent. If nonzero, its lifetime will be decreased each
    tick; if it then *reaches* zero, it will be destroyed.
  */
  unsigned long lifetime;

  /*
    Where is this item? Only one of INVENTORY, LOCATION and STACK
    should normally be set. If it's an augmentation/facet none will be
    set, but we will handle those specially.
  */

  /*
    It is carried by the creature pointed to by INVENTORY. If set, it
    should always be possible to reverse-lookup the item in the
    creatures item list (only as single item or stack parent, never as
    a stack child - STACK should be used in that case).
    
    Notable exceptions are conjured weapons that belong to a creature,
    but are not actually in its inventory.
  */
  creature_t * inventory;

  /*
    It is somewhere on LOCATION. If LOCATION is set, Y and X should
    have sane values as well.
  */
  level_t * location;
  unsigned int y;
  unsigned int x;

  /*
    It is attached to STACK (of course, the stack parent could in
    turn be on a level or in an inventory).
  */
  item_t * stack;

  /*
    If this item is a stack parent, CHILD should point to the second
    item in the stack.
  */
  item_t * child;

  /*
    The next item in a linked list of items (can be in an inventory,
    on a level or in a stack). If PREV_ITEM is NULL, it is the first
    item, if NEXT_ITEM is NULL, it is the last.
  */
  item_t * prev_item;
  item_t * next_item;

  /*
    Which properties of this item that have been identified. This is a
    bitmask of id_status_t. See identify.h
  */
  unsigned int id;
  
  /* true if this item should ricochet off walls when thrown/fired (e.g. grenades). */
  blean_t ricochets;

  /* true if this item is equipped by the creature carrying it. */
  blean_t equipped;

  /* true if this item cannot be destroyed in-game. */
  blean_t indestructible;

  /* true if this item can be eaten. */
  blean_t edible;

  /* true if item should automatically be known_name when picked up. */
  blean_t auto_id;
  
  /*
    Set this to true to prohibit stacking for this item even if the
    item type usually stacks.
  */
  blean_t prohibit_stacking;

  /*
    Custom parameters that behave differently depending on item
    type. See CUSTOM_*.
  */
  signed int custom[CUSTOM_FIELDS];

  /* The attribute modifiers applied to a creature that has this item equipped. */
  signed int attr_mod[ATTRIBUTES];

  /*
    INVOKE_POWER should be 0 (disabled) or the index of the power this
    item should have. CHARGES is how many times it can be
    activated. If RECHARGEABLE is true, it can be recharged (e.g. with
    a Recharge scroll).
  */
  unsigned int invoke_power;
  signed int charges;
  blean_t rechargeable;

  /* How much space this item takes up in an inventory. */
  unsigned int weight;
};



/* (De)allocation */
item_t * alloc_item(void);
item_t * clone_item(const item_t * original);
void del_item(item_t * item);

/* Attaching & detaching */
item_t * detach_item(item_t * item);
item_t * attach_item_to_level(level_t * level, item_t * item);
item_t * attach_item_to_stack(item_t * parent, item_t * new_child);

/* Coordinates & placing */
void place_item(item_t * item, const unsigned int y, const unsigned int x);
void find_random_item_spot(item_t * item);
void find_nearest_item_spot(item_t * item,
			    const unsigned int intended_y,
			    const unsigned int intended_x);
blean_t put_item_or_destroy(level_t * level, item_t * item,
			    const unsigned int y,
			    const unsigned int x);

unsigned int get_weight(const item_t * item);

/* Name and statistics presentation - these are all in itemtext.c */
void set_item_single_name(item_t * item, const char * new_unid, const char * new_id);
void set_item_plural_name(item_t * item, const char * new_unid, const char * new_id);
void set_item_description(item_t * item, const char * new_desc);
item_article_t get_item_article(const item_t * item);
char * get_item_name(const item_t * item);
char * get_inv_item_name(const item_t * item);
char * _get_item_name(const item_t * item, const blean_t brief);
char * get_item_data(const item_t * item);
void label_item(item_t * item, const char * new_label);
char * get_default_verb(const item_t * item);
void in_use_str(char * dest, const item_t * item);

/* Item properties - these are in itemprop.c */
blean_t is_equipable(const item_t * item);
blean_t is_cursable(const item_t * item);
blean_t is_rechargeable(const item_t * item);
blean_t is_edible(const item_t * item);
blean_t is_readable(const item_t * item);
blean_t is_equipped(const item_t * item);
blean_t is_drinkable(const item_t * item);
blean_t almost_wand(const item_t * item);
/*blean_t is_invokable(const item_t * item);*/
blean_t can_activate(const item_t * item);
blean_t can_apply(const item_t * item);
blean_t is_food(const item_t * item);
blean_t explosive(const item_t * item);

/* Attribute related */
signed int get_item_mod(const item_t * item, const attr_index_t attr_index);
unsigned int get_min_damage(const item_t * item);
unsigned int get_max_damage(const item_t * item);
unsigned int random_damage(const creature_t * creature, const item_t * weapon, const item_t * missile);
damage_type_t get_damage_type(const item_t * item);
unsigned int get_weapon_range(const item_t * weapon);
signed int weapon_seq(const item_t * item, const unsigned int pos);

#endif
