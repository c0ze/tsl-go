#include <stdlib.h>

#include "main.h"
#include "stuff.h"
#include "ui.h"
#include "anim.h"
#include "creature.h"
#include "game.h"
#include "fov.h"
#include "options.h"



anim_t * new_anim(const int t)
{
  anim_t * ret;

  ret = malloc(sizeof(anim_t));

  if (ret == NULL)
    out_of_memory();

  ret->param[0] = ret->param[1] =
    ret->param[2] = ret->param[3] =
    ret->param[4] = ret->param[5] =
    ret->param[6] = ret->param[7] =
    0;

  ret->frame = 0;
  ret->type = t;

  ret->next = first_anim;
  first_anim = ret;
  
  return ret;
}



void del_anim(anim_t * anim)
{
  anim_t * temp;
  anim_t * prev;

  prev = NULL;

  for (temp = first_anim; temp != NULL; temp = temp->next)
  {
    if (temp == anim)
    {
      if (temp == first_anim)
	first_anim = temp->next;
      else if (prev)
	prev->next = temp->next;

      free(temp);
      return;
    }

    prev = temp;
  }

  return;
}



/*
  Pass time (one frame) on all animations.
*/
void animate()
{
  anim_t * anim;
  anim_t * n;
  
  for (anim = first_anim; anim != NULL; )
  {
    anim->frame++;

    switch (anim->type)
    {
      case anim_type_damage:
	anim->param[ANIM_POPUP_OFFSET] += 1;
	
	if (anim->frame > 10)
	  anim->frame = -1;
	break;

      case anim_type_deathspell:
      case anim_type_attention:
      case anim_type_alert:
	if (anim->frame > 10)
	  anim->frame = -1;
	break;

      default:
	break;
    }

    if (anim->frame == -1)
    {
      n = anim->next;
      del_anim(anim);
      anim = n;
    }

    if (anim)
      anim = anim->next;
  }

  return;
}



void add_anim(const int t, const int p0, const int p1, const int p2, const int p3)
{
  anim_t * anim;

  if (options.glyph_mode)
    return;

  anim = new_anim(t);

  anim->type = t;

  anim->param[0] = p0;
  anim->param[1] = p1;
  anim->param[2] = p2;
  anim->param[3] = p3;

  return;
} /* add_anim */




blean_t anim_coords(anim_t * anim, int * y, int * x)
{
  creature_t * creature;
  level_t * level;
  int i;

  if (anim == NULL)
    return false;

  level = get_current_level();

  switch (anim->type)
  {
    case anim_type_deathspell:
      *y = anim->param[ANIM_DEATH_Y];
      *x = anim->param[ANIM_DEATH_X];
      return true;

    case anim_type_damage:
    case anim_type_attention:
    case anim_type_alert:
      for (i = 0; i < level->creatures; i++)
      {
	creature = level->creature[i];

	if (creature && creature->uid == anim->param[ANIM_POPUP_UID])
	{
	  /*if (can_see(game->player, creature->y, creature->x) == false)
	    return false;*/
	    
	  *y = creature->y;
	  *x = creature->x;
	  return true;
	}
      }

      *y = anim->param[ANIM_POPUP_BKP_Y];
      *x = anim->param[ANIM_POPUP_BKP_X];
      return true;

    default:
      return false;
  }

  return false;
}



void damage_popup(creature_t * creature, const int amount)
{
  add_anim(anim_type_damage, creature->uid, amount, creature->y, creature->x);
}
