/*
  anim.h - animation hints.
*/

#ifndef _ANIM_H_
#define _ANIM_H_

typedef struct anim_t anim_t;
struct anim_t
{
  int param[8];
  int type;
  int frame;
  anim_t * next;
};

#define ANIM_POPUP_UID     0
#define ANIM_POPUP_VALUE   1
#define ANIM_POPUP_BKP_Y   2
#define ANIM_POPUP_BKP_X   3
#define ANIM_POPUP_OFFSET  5

#define ANIM_ATTN_UID    0
#define ANIM_ATTN_BKP_Y  2
#define ANIM_ATTN_BKP_X  3

#define ANIM_DEATH_Y  0
#define ANIM_DEATH_X  1


anim_t * first_anim;

anim_t * new_anim(const int t);

void del_anim(anim_t * anim);

void animate(void);

blean_t anim_coords(anim_t * anim, int * y, int * x);

#endif
