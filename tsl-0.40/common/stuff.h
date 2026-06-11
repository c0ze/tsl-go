/*
  stuff.h - general-purpose functions to make life easier.
*/

#ifndef _STUFF_H_
#define _STUFF_H_

#include "main.h"

char * mycat(char * d, const char * s);
blean_t mycmp(const char * c1, const char * c2);
blean_t wishcmp(const char * c1, const char * c2);
void upperfirst(char *s);
char * mydup(const char *s);
char * read_file(const char * filename);
signed long MAX(signed long a, signed long b);
signed long MIN(signed long a, signed long b);
blean_t is_number(char * s);
char * get_damage_type_label(damage_type_t type);
char * get_file_path(const char * filename);
blean_t maybe(void);

unsigned int speed_to_dir(const signed int move_y, const signed int move_x);
void dir_to_speed(const dir_t dir, signed int * y_speed, signed int * x_speed);

unsigned long int tslrnd(void);
void tslrnd_seed(void);
unsigned int intrnd(const unsigned int min, const unsigned int max);

int next_uid;
int get_uid(void);

#endif
