/*
  rolls.h

  Functions related to die rolling.
*/

#ifndef _ROLLS_H_
#define _ROLLS_H_

signed int sroll(const char * roll_str);
unsigned int roll(unsigned int n, unsigned int sides);
blean_t roll_xn(signed int x, signed int n);

#endif
