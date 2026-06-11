#include <stdio.h>
#include <stdlib.h>
#include <string.h>
/*#include <sys/types.h>*/
#include <sys/stat.h>
#include <ctype.h>
#include <time.h>

#ifndef _WIN32
#include <pwd.h>
#include <unistd.h>
#endif

#include "stuff.h"
#include "mt19937ar.h"


/*
  Reads FILENAME into memory and returns a pointer to it. The string
  returned will contain everything in the file, followed by a \0. NULL
  is returned if a) the file doesn't exist or can't be read, b) a \0
  is encountered in the file (which means this function can only read
  "text files").
*/
char * read_file(const char * filename)
{
  char * result;
  struct stat file_stat;
  FILE * f;
  unsigned int size;

  f = fopen(filename, "r");

  if (f == NULL)
    return NULL;

  result = NULL;

  stat(filename, &file_stat);

  size = file_stat.st_size;

  /* We'll make sure to get an extra byte for the \0 */
  result = malloc(size + 1);
  if (result == NULL) out_of_memory();

  fread(result, 1, size, f);

  fclose(f);

  /* Terminate the string */
  result[size] = '\0';

  return result;
} /* read_file */



/*
  Returns a pointer to a string with D and S concatenated. D must be a
  malloced string and will be _freed_ by this function. If S is null,
  returns D.
*/
char * mycat(char * d, const char * s)
{
  char * ret;
  unsigned int d_len;
  unsigned int s_len;

  if (d == NULL)
    return NULL;

  if (s == NULL)
    return d;
  
  d_len = strlen(d);
  s_len = strlen(s);

  ret = malloc(d_len + s_len + 1);

  if (ret == NULL)
    out_of_memory();

  strncpy(ret, d, d_len);
  strncpy(ret + d_len, s, s_len);
  ret[d_len + s_len] = '\0';

  return ret;
} /* mycat */



/*
  Compares strings C1 and C2. If either is NULL, false is returned. If
  the strings match, true is returned. If the strings don't match,
  false is returned.
 */
blean_t mycmp(const char * c1, const char * c2)
{
  if ((c1 == NULL) || (c2 == NULL))
    return false;
  else
  { 
    if (strcmp(c1, c2) == 0)
      return true;
    else
      return false;
  }
} /* mycmp */



/*
  Compares strings C1 and C2, ignoring case. If either is NULL, false
  is returned. If C2 is a substring of C1, true is returned.

  I wish strcasestr() was standard.
*/
blean_t wishcmp(const char * c1, const char * c2)
{
  unsigned int i;
  unsigned int l;
  char * a;
  char * b;

  if ((c1 == NULL) || (c2 == NULL))
    return false;

/*  if (strlen(c1) != strlen(c2))
    return false;*/

  a = mydup(c1);
  b = mydup(c2);

  l = strlen(a);
  for (i = 0; i < l; i++)
    if (isalpha(a[i]))
      a[i] = tolower(a[i]);

  l = strlen(b);
  for (i = 0; i < l; i++)
    if (isalpha(b[i]))
      b[i] = tolower(b[i]);

  if (strstr(a, b) != NULL)
  {
    free(a);
    free(b);
    return true;
  }

  free(a);
  free(b);
  return false;
} /* wishcmp */



/*
  Returns the larger of A and B.
*/
signed long MAX(signed long a, signed long b)
{
  if (a > b)
    return a;
  else
    return b;
} /* MAX */



/*
  Returns the smaller of A and B.
*/
signed long MIN(signed long a, signed long b)
{
  if (a < b)
    return a;
  else
    return b;
} /* MIN */



/*
  Duplicates S. The pointer returned is a newly allocated memory area,
  into which S is copied. If there wasn't enough free memory to create
  the copy, out_of_memory() will be called.
*/
char * mydup(const char *s)
{
  char * t;

  if (s == NULL)
    return NULL;

  t = malloc(sizeof(char) * (strlen(s) + 1));

  if (t == NULL)
    out_of_memory();

  strcpy(t, s);

  return t;
} /* mydup */



/*
  Converts the first character of S to uppercase.
 */
void upperfirst(char *s)
{
  if (s == NULL)
    return;

  s[0] = toupper(s[0]);

  return;
} /* upperfirst */



/*
  Returns true if all characters in the null-terminated string S are
  digits, otherwise false.
 */
blean_t is_number(char * s)
{
  unsigned int i;

  if (s == NULL)
    return false;

  if (s[0] == '\0')
    return false;
  
  for (i = 0; i < strlen(s); i++)
  {
    if (s[i] == '\0')
      break;

    if (isdigit(s[i]) == 0)
      return false;
  }

  return true;
} /* is_number */



/*
  Sometimes true, sometimes not.
*/
blean_t maybe()
{
  if (roll(1, 3) == 1)
    return true;
  else
    return false;
} /* maybe */





char * get_damage_type_label(damage_type_t type)
{
  if ((type < 0) || (type >= DAMAGE_TYPES))
    return NULL;
  else
    return damage_type_label[type];
} /* get_damage_type_label */



/*
  Returns a dir_* (see main.h) constant corresponding to the direction
  represented by MOVE_Y, MOVE_X. These must each be -1, 0 or +1.
 */
unsigned int speed_to_dir(const signed int move_y, const signed int move_x)
{
  if (move_y == -1 && move_x == 0)
    return dir_n;
  else if (move_y == -1 && move_x == +1)
    return dir_ne;
  else if (move_y == 0 && move_x == +1)
    return dir_e;
  else if (move_y == +1 && move_x == +1)
    return dir_se;
  else if (move_y == +1 && move_x == 0)
    return dir_s;
  else if (move_y == +1 && move_x == -1)
    return dir_sw;
  else if (move_y == 0 && move_x == -1)
    return dir_w;
  else if (move_y == -1 && move_x == -1)
    return dir_nw;
  else
    return dir_none;
} /* speed_to_dir */



void dir_to_speed(const dir_t dir, signed int * y_speed, signed int * x_speed)
{
  if ((y_speed == NULL) || (x_speed == NULL))
    return;

  switch (dir)
  {
    case dir_n:  *y_speed = -1; *x_speed = 0;  break;
    case dir_ne: *y_speed = -1; *x_speed = +1; break;
    case dir_e:  *y_speed = 0;  *x_speed = +1; break;
    case dir_se: *y_speed = +1; *x_speed = +1; break;
    case dir_s:  *y_speed = +1; *x_speed = 0;  break;
    case dir_sw: *y_speed = +1; *x_speed = -1; break;
    case dir_w:  *y_speed = 0;  *x_speed = -1; break;
    case dir_nw: *y_speed = -1; *x_speed = -1; break;

    default:
      *y_speed = 0;
      *x_speed = 0;
      break;
  }

  return;
} /* dir_to_speed */



/*
  Usually TSL works in the users home directory. This returns the full
  path to the file FILENAME in this directory. If no home directory
  could be determined, it should (?) fall back to the current working
  directory.
*/
char * get_file_path(const char * filename)
{
  char * ret;
  char * home_dir;

  if (filename == NULL)
    return NULL;

  /* Build a path. */

  /*
    Find the users home directior. Look in $HOME. If $HOME was unset,
    fall back on whatever is in the password file.
  */
  home_dir = getenv("HOME");
  
#ifndef _WIN32
  if (home_dir == NULL)
  {
    struct passwd * pw;
    
    pw = getpwuid(getuid());
    if (pw == NULL) return false;
    home_dir = pw->pw_dir;
  }
#endif

  if (home_dir == NULL)
    return mydup(filename);

  ret = malloc(strlen(home_dir) + strlen(filename) + 2);

  if (ret == NULL)
    out_of_memory();

  if (home_dir[strlen(home_dir) - 1] != '/')
    sprintf(ret, "%s/%s", home_dir, filename);
  else
    sprintf(ret, "%s%s", home_dir, filename);

  return ret;
} /* get_home_dir */



/*
  Wrapper for random number generator. Returns... a random number. I hope.
*/
unsigned long int tslrnd(void)
{
  return genrand_int32();
} /* tslrnd */



/*
  Seeder for RNG.
*/
void tslrnd_seed(void)
{
  unsigned long shit[1];

  shit[0] = time(NULL);
  init_by_array(shit, 1);

  return;
} /* tslrnd_seed */



/*
  
 */
unsigned int intrnd(const unsigned int min, const unsigned int max)
{
  return min + tslrnd() % (max - min + 1);
} /* intrnd */



int get_uid(void)
{
  return next_uid++;
} /* get_uid */
