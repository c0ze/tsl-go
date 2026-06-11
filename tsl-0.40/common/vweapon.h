/*
  vweapon.h
*/

#ifndef _VWEAPON_H_
#define _VWEAPON_H_

/*
  Virtual weapons: When fighting unarmed, one of these items should be
  used to represent the "weapon" (be it fists, claws, tentacles,
  whatever) of the creature. They are called "virtual" because they
  are not really items; they cannot be picked up or held in any
  inventory; only a single unique instance of each weapon exists in
  the whole game (accessible through the global array of pointers
  known as virtual_weapon[], using a value from virtual_weapon_t
  constants as subscript). Every creature_t created should have a
  proper virtual weapon set. The list should never be modified except
  by init_virtual_weapons() and del_virtual_weapons().
*/
#define VIRTUAL_WEAPONS 30

item_t * virtual_weapon[VIRTUAL_WEAPONS];

enum virtual_weapon_t
{
  virtual_fists,
  virtual_claws,
  virtual_blade_hands,
  virtual_fangs,
  virtual_poison_fangs,
  virtual_tentacle,
  virtual_cabbage_hands,
  virtual_cold_touch,
  virtual_ghoul_touch,
  virtual_slime,
  virtual_slap,
  virtual_sting,
  virtual_kick,
  virtual_draining_touch,
  virtual_book,
  virtual_flame_hands,
  virtual_chainsaw,
  virtual_mimic_fangs,
  virtual_dragon,
  virtual_hungry_book
};
typedef enum virtual_weapon_t virtual_weapon_t;

void init_virtual_weapons(void);
void del_virtual_weapons(void);

#endif
