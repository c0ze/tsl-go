#include <stdio.h>
#include <string.h>

#include "clipbrd.h"


/*
  There is some problem with these prototyes.
  Forcibly include them!
*/
FILE *popen(const char *command, const char *type);
int pclose(FILE *stream);



/*
  Puts DATA on the clipboard, if available.
*/
void put_clipboard(char * data)
{
  FILE * output;

  output = popen("xsel", "w");
  
  if (output == NULL)
    return;
  
  fwrite(data, strlen(data), sizeof(char), output);

  pclose(output);

  return;
} /* put_clipboard */
