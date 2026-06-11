#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "stuff.h"
#include "main.h"



/*
  Given a pointer ROLL_STR to a string of the form "nds[+/-m]" (a
  positive integer, followed by a literal 'd', another positive
  integer, and optionally a +/- modifier), returns a random roll with
  n s-sided dice, adding m to the sum.

  If the input string follows any other pattern I'm not sure what
  would happen. Probably return 0, but who knows?
*/
signed int sroll(const char * roll_str)
{
  int n_dice;
  int dice_size;
  int mod;
  int temp;
  int ret;

  n_dice = 0;
  dice_size = 0;
  mod = 0;

  if (roll_str == NULL)
    return 0;

  temp = sscanf(roll_str, "%dd%d%d", &n_dice, &dice_size, &mod);

  if (temp < 2)
    return 0;

  /* Now roll */
  ret = roll(n_dice, dice_size) + mod;

  return ret;
} /* sroll */



/*
  Rolls N dice of size SIDES and returns the sum.
*/
unsigned int roll(unsigned int n, unsigned int sides)
{
  int sum;
  int i;
  
  sum = 0;

  for (i = 0; i < n; i++)
  {
    sum += (1 + tslrnd() % sides);
  }

  return sum;
} /* roll */



/*
 */
blean_t roll_xn(signed int x,
		signed int n)
{
  if (x < 1)
  {
    n += (1 - x);
    x = 1;
  }

  if (n < 1)
  {
    x += (1 - n);
    n = 1;
  }

  if ((tslrnd() % (x + n)) < x)
    return true;
  else
    return false;
} /* roll_xn */
