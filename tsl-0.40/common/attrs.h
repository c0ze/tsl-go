/*
  attrs.h

  This is where we set up the attribute definitions.

  attrs.c also implements functions that are prototyped in creature.h.
*/

#ifndef _ATTRS_H_
#define _ATTRS_H_

attr_info_t * build_attr_info(const char * new_name);
void del_all_attr_info(void);
void init_all_attr_info(void);

#endif
