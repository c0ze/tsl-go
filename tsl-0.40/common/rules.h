/*
  rules.h - constants that control various aspects of the
  game. Anything that needs to be balanced should be in here.
  
  Note that there is no rules.c!
*/

#ifndef _RULES_H_
#define _RULES_H_

#define POTION_OF_ENERGY 10

#define CROWN_OF_THORNS_WOUNDING  8
#define TROLLISH_ILLITERACY       250

/* */
#define WEAPON_BREAK_SCALE 10000
#define WEAPON_BREAK_INCREASE 1

/* How many random floor tiles we have. Only affects SDL. */
#define FLOOR_TYPES 10

/**/
#define PATHFINDING 15

/* How much faster poison works for enemies than the player */
#define ENEMY_POISON_FACTOR 6

/**/
#define FORCE_BOLT_DIFFICULTY 1
#define FORCE_BOLT_MIN_DAMAGE 5
#define FORCE_BOLT_MAX_DAMAGE 10
#define FORCE_BOLT_RANGE 4

#define FROST_RAY_DIFFICULTY 1
#define FROST_RAY_MIN_DAMAGE 2
#define FROST_RAY_MAX_DAMAGE 6
#define FROST_RAY_RANGE 3

#define FIREBALL_DAMAGE 20
#define EXPLOSION_DAMAGE 10
#define EXPLOSION_DEFAULT 5
#define MUDBALL_DAMAGE 6
#define NOXIOUS_BREATH_DAMAGE "1d2"
#define BREATHE_FIRE_DAMAGE "2d4"
#define BOOBY_TRAP_DAMAGE "4d6"
#define SHOCK_DAMAGE "1d6"
#define PLATE_DAMAGE "1d4+1"
#define WAND_OF_ENSLAVEMENT 5
#define FIREBALL_DIFFICULTY 3

/* How much walking in lava and forcefields hurts */
#define LAVA_DAMAGE "1d6+1"
#define FORCEFIELD_DAMAGE "1d8"


#define RESPAWN_TIME  15000
#define RESPAWN_SPEED 7

/* What to use as attack % for dart traps */
#define TRAP_MISSILE_SKILL 50

/* How much fire resistance the Dragon (and its scales) has */
#define DRAGON_FIRE_RESISTANCE 14

/* How much items "of trap detection" should improve the skill */
#define TRAP_DETECTION_BONUS 1

/* How long a summoned creature lives by default */
#define DEFAULT_SUMMON_LIFETIME 500

/* How far we can throw by default */
#define BASE_THROW_RANGE 2

/* How far we can leap by default */
#define LEAP_RANGE 4

#define WEB_RAY_RANGE 4

/* At what percentage of its max HP a creature will retreat from battle */
#define HP_RETREAT         (0.7f)

#define PUSH_RANGE 3
#define PUSH_ROLL "1d3+1"

/*
  At what percentage of its max HP a creature will resume battle -
  this should be higher than HP_RETREAT since enemies should only
  resume fighting if they feel somewhat ok.
*/
#define HP_RESUME_BATTLE   (0.3f)

#define MEDKIT_HEAL_AMOUNT  5
/*
  How many ticks worth of something we must accumulate to regain or
  lose a point of something.
*/
#define BASE_SPEED             100
#define HASTE_AMOUNT           30
#define HASTE_LENGTH           20
#define ROGUE_SPEED_BONUS      30
#define SMALL_SPEED_BONUS      10

#define TURN_TIME              1000
#define FATIGUE_RECOVERY_TIME  1000
#define ENERGY_RECOVERY_TIME   500
#define POISON_TIME            500

#define NOXIOUS_BREATH_POISON  12
#define MISSILE_POISON         10
#define WEB_DURATION           27
#define WEB_STRUGGLE           6
#define DEFAULT_POISON_TIME    25
#define DEFAULT_BLIND_TIME     33
#define MELEE_WOUND_TIME       12
#define MISSILE_WOUND_TIME     7
#define STUN_LENGTH            "2d4+4"
/*#define CONFUSION_TIME         20*/

#define TORCH_LIFETIME         350
#define LANTERN_LIFETIME       650
#define WARNING_FLICKERS       80
#define WARNING_GO_OUT         30

#define SLOW_LENGTH            20
#define SLEEP_DURATION         25
#define POTION_OF_HEALING      30
#define SHAPESHIFT_DURATION    85
#define LEVITATE_TIME          20
#define HIDE_DURATION          10

#define DEFAULT_EFFECT_TTL     10

/* How long a magic weapon should remain in game. */
#define MAGIC_WEAPON_LIFETIME  22
#define HUNGRY_BOOK_LIFETIME   100

/* What roll to make to avoid doors drawing attention. */
#define DOOR_STEALTH           2

/*
  How many notches of poison resistance there are, with this being the
  one where a creature becomes immune.
*/
#define POISON_IMMUNITY 5

/* How much we're allowed to carry without becoming burdened. */
#define DEFAULT_CARRYING_CAPACITY 400
#define LOW_CARRYING_CAPACITY 300
#define STRENGTH_AUG 200

/* How much we slow down a creature that is carrying too much */
#define BURDENED_FACTOR (0.5)

/* Default weights */
#define WEIGHT_WAND      21
#define WEIGHT_POTION    7
#define WEIGHT_SCROLL    4
#define WEIGHT_TOOL      12
#define WEIGHT_KEY       1
#define WEIGHT_DATAPROBE 5
#define WEIGHT_FOOD      8
#define WEIGHT_CORPSE    599

#define WEIGHT_MISSILE  1

#define WEIGHT_DAGGER        18
#define WEIGHT_SWORD_SMALL   22
#define WEIGHT_SWORD_LARGE   28
#define WEIGHT_CLUB          26
#define WEIGHT_MACE          28
#define WEIGHT_AXE           30
#define WEIGHT_STAFF         25

#define WEIGHT_BOW           20
#define WEIGHT_FIREARM       30

#define WEIGHT_HAT           20
#define WEIGHT_CLOAK         25
#define WEIGHT_LIGHT_ARMOR   40
#define WEIGHT_HEAVY_ARMOR   80
#define WEIGHT_BOOTS         35



#define MSG_UNABLE_TO_DRINK  "You are unable to drink!"
#define MSG_UNABLE_TO_EAT    "You are unable to eat!"

#endif
