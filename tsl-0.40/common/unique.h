#ifndef _UNIQUE_H_
#define _UNIQUE_H_

#include "main.h"
#include "level.h"


/*
  unique_status keeps track of each unique; not_present means it will
  not spawn in this game at all, available means it may spawn but
  hasn't yet, spawned means it has been created and is in play, slain
  means it has been vanquished.
*/
enum unique_status_t
{
  unique_status_not_present,
  unique_status_available,
  unique_status_spawned,
  unique_status_slain
};
typedef enum unique_status_t unique_status_t;

unique_status_t unique_status[monsters];

creature_t * build_unique(const monster_t id);
void init_uniques(void);

#endif
