/*
  inscript.h
 */

#ifndef _INSCRIPT_H_
#define _INSCRIPT_H_

#include "main.h"
#include "traps.h"

extern char * inscr_type[];
extern char * inscr_message[];

#define INSCR_TYPE 0
#define INSCR_MESSAGE 1

#define INSCR_MESSAGES 10

/*#define INSCR_ENGRAVE   0
#define INSCR_DUST      1
#define INSCR_BLOOD     2
#define INSCR_SCRIBBLED 3
#define INSCR_RUNES     4
#define INSCR_CRYPTIC   5*/
#define INSCR_TYPES     7

void inscription(const unsigned int type, const unsigned int message);

#endif
