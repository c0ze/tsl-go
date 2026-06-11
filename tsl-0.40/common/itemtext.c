#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "main.h"
#include "stuff.h"
#include "item.h"
#include "stacks.h"
#include "creature.h"
#include "game.h"

/*
  See item.h for prototypes.
*/

char * item_verbs[] =
{
  [item_verb_bug] = "BUG",
  [item_verb_take_off] = "take off",
  [item_verb_put_on] = "put on",
  [item_verb_put_away] = "put away",
  [item_verb_ready] = "ready",
/*  [item_verb_equip] = "equip",*/
  [item_verb_wield] = "wield",
  [item_verb_use] = "use",
  [item_verb_drink] = "drink",
  [item_verb_read] = "read",
  [item_verb_eat] = "eat",
  [item_verb_zap] = "zap",
  [item_verb_light] = "light",
  [item_verb_put_out] = "put out"
};



/*
  Returns a pointer to a string describing the default action for ITEM
  ("drink" for potions, "equip" for armor, etc).
*/
char * get_default_verb(const item_t * item)
{
  if (item == NULL)
    return item_verbs[item_verb_bug];

  switch (item->item_type)
  {
    case item_type_potion:
      return item_verbs[item_verb_drink];

    case item_type_wand:
      return item_verbs[item_verb_zap];

    case item_type_book:
    case item_type_scroll:
      return item_verbs[item_verb_read];

    case item_type_tool:
      return item_verbs[item_verb_use];

    case item_type_food:
      return item_verbs[item_verb_eat];

    case item_type_m_weapon:
      if (item->equipped)
	return item_verbs[item_verb_put_away];
      else
	return item_verbs[item_verb_wield];
      
    case item_type_r_weapon:
    case item_type_ammo:
      if (item->equipped)
	return item_verbs[item_verb_put_away];
      else
	return item_verbs[item_verb_ready];

    case item_type_light:
      if (item->equipped)
	return item_verbs[item_verb_put_out];
      else
	return item_verbs[item_verb_light];

    default:
      if (is_equipable(item))
      {
	if (item->equipped)
	  return item_verbs[item_verb_take_off];
	else
	  return item_verbs[item_verb_put_on];
      }
  }

  return item_verbs[item_verb_bug];
} /* get_default_verb */



void in_use_str(char * dest, const item_t * item)
{
  if (item == NULL)
    return;
  
  switch (item->item_type)
  {
    case item_type_m_weapon:
      strcpy(dest, "(wielded)");
      break;

    case item_type_r_weapon:
      strcpy(dest, "(held)");
      break;

    case item_type_ammo:
      strcpy(dest, "(ready)");
      break;

    case item_type_body:
    case item_type_feet:
    case item_type_head:
    case item_type_cloak:
      strcpy(dest, "(worn)");
      break;

    case item_type_tool:
    case item_type_wand:
      strcpy(dest, "(held)");
      break;

    default:
      strcpy(dest, "(held)");
      break;
  }

  return;
} /* in_use_str */



/*
  Sets the "unidentified" and "identified" names of ITEM to NEW_UNID
  and NEW_ID, respectively. If any of the values is omitted (== NULL),
  that name will be skipped.
*/
void set_item_single_name(item_t * item, const char * new_unid, const char * new_id)
{
  if (item == NULL)
    return;

  if (new_unid != NULL)
  {
    if (item->single_unid_name != NULL)
    {
      mem_alloc.chars -= strlen(item->single_unid_name) + 1;
      free(item->single_unid_name);
    }
    
    item->single_unid_name = mydup(new_unid);
    if (item->single_unid_name == NULL) out_of_memory();
    mem_alloc.chars += strlen(item->single_unid_name) + 1;
  }
  
  if (new_id != NULL)
  {
    if (item->single_id_name != NULL)
    {
      mem_alloc.chars -= strlen(item->single_id_name) + 1;
      free(item->single_id_name);
    }
    
    item->single_id_name = mydup(new_id);
    if (item->single_id_name == NULL) out_of_memory();
    mem_alloc.chars += strlen(item->single_id_name) + 1;
  }
  
  return;
} /* set_item_single_name */



/*
  Sets the "unidentified" and "identified" *plural* names of ITEM to
  NEW_UNID and NEW_ID, respectively. If any of the values is omitted
  (== NULL), that name will be skipped. This is the name that will be
  used when items are stacked. For items that don't stack, this can be
  omitted.
*/
void set_item_plural_name(item_t * item, const char * new_unid, const char * new_id)
{
  if (item == NULL)
    return;

  if (new_unid != NULL)
  {
    if (item->plural_unid_name != NULL)
    {
      mem_alloc.chars -= strlen(item->plural_unid_name) + 1;
      free(item->plural_unid_name);
    }
    
    item->plural_unid_name = mydup(new_unid);
    if (item->plural_unid_name == NULL) out_of_memory();
    mem_alloc.chars += strlen(item->plural_unid_name) + 1;
  }
  
  if (new_id != NULL)
  {
    if (item->plural_id_name != NULL)
    {
      mem_alloc.chars -= strlen(item->plural_id_name) + 1;
      free(item->plural_id_name);
    }
    
    item->plural_id_name = mydup(new_id);
    if (item->plural_id_name == NULL) out_of_memory();
    mem_alloc.chars += strlen(item->plural_id_name) + 1;
  }
  
  return;
} /* set_item_plural_name */



/*
  Sets the description of ITEM to NEW_DESC.
*/
void set_item_description(item_t * item, const char * new_desc)
{
  if (item == NULL)
    return;

  if (new_desc != NULL)
  {
    if (item->description != NULL)
    {
      mem_alloc.chars -= strlen(item->description) + 1;
      free(item->description);
    }
    
    item->description = mydup(new_desc);
    if (item->description == NULL) out_of_memory();
    mem_alloc.chars += strlen(item->description) + 1;
  }
  
  return;
} /* set_item_description */



/*
  Returns what article should be used before ITEM, based on the number
  in the stack and the (un)identified state of the item name.

  See also:
    item_article_t in item.h
    item_t in item.h
*/
item_article_t get_item_article(const item_t * item)
{
  if (item == NULL)
    return article_i_a;

  if (item->child == NULL)
  {
    if (identified(item) & known_name)
      return item->single_id_article;
    else
      return item->single_unid_article;
  }
  else
  {
    if (identified(item) & known_name)
      return item->plural_id_article;
    else
      return item->plural_unid_article;
  }
} /* get_item_article */



/*
  Returns a pointer to a string suitable for use in sentences. The
  returned value must be free()d, See _get_item_name().
*/
char * get_item_name(const item_t * item)
{
  return _get_item_name(item, false);
} /* get_item_name */



/*
  Returns a pointer to a string suitable for use in inventories. The
  returned value must be free()d, See _get_item_name().
*/
char * get_inv_item_name(const item_t * item)
{
  return _get_item_name(item, true);
} /* get_inv_item_name */



/*
  Returns a pointer to a name string for ITEM. The value returned has
  been malloced and must be freed after use. If BRIEF is true, the
  item name will be shortened as much as possible.
*/
char * _get_item_name(const item_t * item, const blean_t brief)
{
  char final[80];
  char temp[80];
  char * ret;
  id_status_t id;
  unsigned quantity;
  blean_t illiterate;

  if (item == NULL)
    return mydup("BUG: NULL item");

  illiterate = attr_current(game->player, attr_p_read);

  id = identified(item);
  quantity = stack_size(item);

  strcpy(final, "");

  if (quantity != 1)
  {
    sprintf(final, "%d ", quantity);
  }

  if (item->item_type == item_type_light)
  {
    if (item->item_number == artifact_everlasting_lantern)
    {
      if (identified(item) & known_name)
      {
	strcat(final, "the ");
      }
      else
      {
	sprintf(temp, "%sunused %s", (brief ? "" : "an "), is_equipped(item) ? "lit " : "");
	strcat(final, temp);
      }
    }
    else if (item->custom[CUSTOM_LIGHT_TICKS] > 0)
    {
      if (item->custom[CUSTOM_LIGHT_TICKS] < item->custom[CUSTOM_LIGHT_MAX_TICKS] * 0.25)
	sprintf(temp, "%snearly spent ", (brief ? "" : "a "));
      else if (item->custom[CUSTOM_LIGHT_TICKS] < item->custom[CUSTOM_LIGHT_MAX_TICKS] * 0.75)
	sprintf(temp, "%shalf-spent ", (brief ? "" : "a "));
      else if (item->custom[CUSTOM_LIGHT_TICKS] < item->custom[CUSTOM_LIGHT_MAX_TICKS])
	sprintf(temp, "%salmost unused ", (brief ? "" : "an "));
      else if (item->custom[CUSTOM_LIGHT_TICKS] >= item->custom[CUSTOM_LIGHT_MAX_TICKS])
	sprintf(temp, "%sunused ", (brief ? "" : "an "));

      strcat(final, temp);

      if (is_equipped(item))
	strcat(final, "lit ");
    }
  }
  else if (brief == false)
  {
    if (quantity == 1)
      strcat(final, item_article_a[get_item_article(item)]);

    strcat(final, item_article_of[get_item_article(item)]);
  }

  if (illiterate && item->item_type == item_type_scroll)
  {
    /* When you cannot read, all scrolls show up the same. */
    if (quantity > 1)
      strcat(final, "scrolls");
    else
      strcat(final, "scroll");
  }
  else if (id & known_name)
  {
    if (item->child == NULL)
    {
      if (item->single_id_name != NULL)
	strcat(final, item->single_id_name);
      else
	strcat(final, "BUG");
    }
    else
    {
      if (item->plural_id_name != NULL)
	strcat(final, item->plural_id_name);
      else
	strcat(final, "BUG");
    }
  }
  else
  {
    if (item->child == NULL)
    {
      if (item->single_unid_name != NULL)
	strcat(final, item->single_unid_name);
      else
	strcat(final, "BUG");
    }
    else
    {
      if (item->plural_unid_name != NULL)
	strcat(final, item->plural_unid_name);
      else
	strcat(final, "BUG");
    }
  }
  
  /*
    Display charges remaining for items that have invoke powers (but
    not scrolls, since they always have one "charge".
  */
  if ((item->invoke_power != 0) &&
      (id & known_charges) &&
      (item->item_type != item_type_scroll))
  {
    if (item->charges == -1)
      strcat(final, " [inf]");
    else
    {
      sprintf(temp, " [%d]", item->charges);
      strcat(final, temp);
    }
  }

  /* If the player has made any notes for this item. */
  if (illiterate == false && item->label != NULL)
  {
    strcat(final, " {");
    strcat(final, item->label);
    strcat(final, "}");
  }
  
  ret = mydup(final);

  if (ret == NULL)
    out_of_memory();

  return ret;
} /* _get_item_name */



/*
  Returns a string describing what is known about ITEM. The string
  needs to be free()d.
*/
char * get_item_data(const item_t * item)
{
  char * ret;
  char temp[500];
  char line[100];
  char who[100];
  int i;
  int len;

  if (item == NULL)
    return mydup("BUG: NULL item");
  
  strcpy(temp, "");

  if (item->item_type == item_type_m_weapon)
    strcpy(who, "wielder");
  else
    strcpy(who, "wearer");

  if (item->item_type == item_type_m_weapon)
  {
    sprintf(line, "  Damage:");
    strcat(temp, line);
    
    for (i = 0; i < WEAPON_ATKSEQLEN; i++)
    {
      if (item->custom[WEAPON_ATKSEQ + i] == 0)
      {
	break;
      }
      else if (item->custom[WEAPON_ATKSEQ + i] == -1)
      {
	strcat(temp, " _");
      }
      else if (item->custom[WEAPON_ATKSEQ + i] > 0)
      {
	if ((identified(item) & known_name) ||
	    item->custom[WEAPON_ATKSEQ_ID + i])
	{
	  sprintf(line, " %d", item->custom[WEAPON_ATKSEQ + i]);
	  strcat(temp, line);
	}
	else
	{
	  strcat(temp, " ?");
	  break;
	}
      }
    }
  }

  if (identified(item) & known_name)
  {
    if (item->item_type == item_type_wand && identified(item) & known_name)
    {
      if (identified(item) & known_charges)
      {
	sprintf(line, "  Holds %d charge%s of %s\n", item->charges,
		(item->charges != 1 ? "s" : ""),
		attr_info[item->invoke_power]->name);
      }
      else
      {
	sprintf(line, "  Holds charges of %s.\n",
		attr_info[item->invoke_power]->name);
      }
      
      strcat(temp, line);
    }
    
    if (explosive(item))
    {
      sprintf(line, "  Explosive damage: %d-%d\n", get_min_damage(item), get_max_damage(item));
      strcat(temp, line);
    }

    switch(item->item_type)
    {
      case item_type_ammo:

	if (get_min_damage(item) && explosive(item) == false)
	{
	  sprintf(line, "  Damage: +%d\n", get_min_damage(item));
	  strcat(temp, line);
	}

	break;

      case item_type_m_weapon:
	strcat(temp, "\n");
	
	if (item->item_type == item_type_m_weapon)
	{
	  if (item->custom[WEAPON_TWOHANDED] == true)
	  {
	    strcat(temp, "  Two-handed\n");
 	  }

	  if (get_item_mod(item, attr_i_knockback) > 0)
	  {
	    sprintf(line, "  %d%% chance of knocking target back\n",
		    get_item_mod(item, attr_i_knockback));
	    strcat(temp, line);
	  }
	  
	  if (get_item_mod(item, attr_i_drain) > 0)
	  {
	    sprintf(line, "  %d%% chance of draining targets health\n",
		    get_item_mod(item, attr_i_drain));
	    strcat(temp, line);
	  }
	  
	  if (get_item_mod(item, attr_i_blink) > 0)
	  {
	    sprintf(line, "  %d%% chance of target blinking away\n",
		    get_item_mod(item, attr_i_blink));
	    strcat(temp, line);
	  }
	  
	  if (get_item_mod(item, attr_i_stun) > 0)
	  {
	    sprintf(line, "  %d%% chance of stunning target\n",
		    get_item_mod(item, attr_i_stun));
	    strcat(temp, line);
	  }

	  if (get_item_mod(item, attr_i_wound) > 0)
	  {
	    sprintf(line, "  %d%% chance of wounding target\n",
		    get_item_mod(item, attr_i_wound));
	    strcat(temp, line);
	  }

	  if (get_item_mod(item, attr_i_backstab) > 0)
	  {
	    sprintf(line, "  %d%% chance of backstabbing target\n",
		    get_item_mod(item, attr_i_backstab));
	    strcat(temp, line);
	  }
	}
	break;

      case item_type_r_weapon:
	sprintf(line, "  Damage: %d-%d\n", get_min_damage(item), get_max_damage(item));
	strcat(temp, line);
	sprintf(line, "  Range: %d\n", get_weapon_range(item));
	strcat(temp, line);
	break;

      case item_type_body:
	sprintf(line, "  Protection: %d (%d)\n", item->custom[ARMOR_PROTECTION], item->custom[ARMOR_DURABILITY]);
	strcat(temp, line);
	break;
     
      default:
	break;
    }

    if (item->indestructible)
      strcat(temp, "  Indestructible.\n");

    for (i = 0; i < ATTRIBUTES; i++)
    {
      signed int mod;

      if (attr_info[i] == NULL)
	continue;
      
      mod = get_item_mod(item, i);

      if (mod == 0)
	continue;
      
      if (attr_info[i]->invoke != NULL)
      {
	sprintf(line, "  Ability: %s\n", attr_info[i]->name);
	strcat(temp, line);

	continue;
      }

      if (attr_info[i]->item_data == false)
	continue;

      switch(i)
      {
	case attr_blindness:
	  sprintf(line, "  Blinds %s\n", who);
	  break;
	  
	case attr_carrying_capacity:
	  sprintf(line, "  Allows %s to carry more weight\n", who);
	  break;
	  
	case attr_gas_immunity:
	  sprintf(line, "  Protects %ss respiratory system\n", who);
	  break;
	  
	case attr_levitate:
	  sprintf(line, "  Makes %s levitate\n", who);
	  break;
	  
	case attr_s_trap_detection:
	  sprintf(line, "  Enhances %ss ability to detect traps\n", who);
	  break;
	  
  	case attr_feet_protected:
	  sprintf(line, "  Protects %ss feet\n", who);
	  break;
	  
	case attr_p_pick_up:
	  sprintf(line, "  Prevents %s from picking up items\n", who);
	  break;
	  
	case attr_p_open_doors:
	  sprintf(line, "  Prevents %s from opening doors\n", who);
	  break;
	  
	case attr_p_throw:
	  sprintf(line, "  Prevents %s from throwing\n", who);
	  break;
	  
	case attr_p_move:
	  sprintf(line, "  Prevents %s from moving\n", who);
	  break;
	  
	case attr_p_eat:
	  sprintf(line, "  Prevents %s from eating\n", who);
	  break;
	  
	case attr_p_drink:
	  sprintf(line, "  Prevents %s from drinking\n", who);
	  break;
	  
	case attr_p_invoke:
	  sprintf(line, "  Prevents %s from invoking\n", who);
	  break;
	  
	case attr_p_read:
	  sprintf(line, "  Makes %s illiterate\n", who);
	  break;
	  
	case attr_backstab_bonus:
	  sprintf(line, "  %d%% bonus to backstabbing\n", mod);
	  break;

	default:
	  if (attr_info[i]->percent)
	    sprintf(line, "  %s: %+d%%\n", attr_info[i]->name, mod);
	  else
	    sprintf(line, "  %s: %+d\n", attr_info[i]->name, mod);
	  break;
      }
      
      strcat(temp, line);
    } /* for */
  } /* identified */

  len = strlen(temp);
  
  for (i = 0; i < len - 1; i++)
  {
    if (temp[i] == '\n')
      temp[i] = ';';
  }

  if ((identified(item) & known_name) &&
      (item->description != NULL))
  {
    strcat(temp, "\n  ");
    strcat(temp, item->description);
    strcat(temp, "\n");
  }

  ret = mydup(temp);
  
  if (ret == NULL)
    out_of_memory();

  return ret;
} /* get_item_data */




/*
  Gives ITEM the player-defined label NEW_LABEL. The string will be
  duplicated: there is no need to reserve memory for it, this function
  will take care of it.
*/
void label_item(item_t * item, const char * new_label)
{
  item_t * temp;

  if (item == NULL)
    return;

  if (item->label != NULL)
  {
    free(item->label);
  }

  if (new_label != NULL)
    item->label = mydup(new_label);
  else
    item->label = NULL;

  /* If this is a stack parent, label each individual item in the stack as well. */
  temp = item->child;

  while (temp != NULL)
  {
    label_item(temp, new_label);
    temp = temp->next_item;
  }

  return;
} /* label_item */
