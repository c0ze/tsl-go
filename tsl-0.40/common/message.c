#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>

#include "main.h"
#include "stuff.h"
#include "message.h"
#include "ui.h"


/*
  Initialises the message buffer and history.
*/
void init_messages()
{
  first_message = NULL;
  last_message = NULL;
  message_history = NULL;
  history_index = 0;

  return;
} /* init_messages */



/*
  Delete everything in the message buffer and history.
*/
void clear_messages(void)
{
  message_t * temp;

  while (message_history)
  {
    temp = message_history;
    message_history = temp->next_message;

    mem_alloc.chars -= strlen(temp->text) + 1;
    free(temp->text);
    free(temp);
    mem_alloc.messages--;
  }
} /* clear_messages */



/*
  Adds an in-game message to the message queue. Messages will be
  displayed in the order they are added.
*/
void queue_msg(const char * text)
{
  message_t * new_msg;

  if (text == NULL)
    return;

  /* Zero-length messages won't be displayed anyway */
  if (strlen(text) == 0)
    return;

  /* Allocate memory for the new message */
  new_msg = malloc(sizeof(message_t));
  if (new_msg == NULL) out_of_memory();
  mem_alloc.messages++;

  new_msg->text = mydup(text);
  if (new_msg->text == NULL) out_of_memory();
  mem_alloc.chars += strlen(new_msg->text) + 1;

  /*
    This will be NULL for now; if another message is added to the
    queue after this one, it will point to that one.
  */
  new_msg->next_message = NULL;

  if (last_message == NULL)
  {
    /*
      If there are currently no messages in the queue, this will be
      both the first and the last one.
    */
    first_message = new_msg;
    last_message = new_msg;
  }
  else
  {
    /* There are other messages; attach */
    last_message->next_message = new_msg;
    last_message = new_msg;
  }
  
  return;
} /* queue_msg */






/*
  Flushes all queued in-game messages.  If there is more text than
  what can be displayed on a single line, the function will pause
  between each one.  If FORCE_WAIT is true, this will also be done
  after the last message.

  Note: You probably don't want to call this manually; instead, use
  the msgflush_wait() and msgflush_nowait() macros.
*/
void _msgflush_internal(const blean_t force_wait)
{
  message_t * msg;
  unsigned int line_length;
  unsigned int more_length;
  unsigned int print_offset;
  unsigned int print_pos;
  unsigned int temp_pos;
  unsigned int l;
  blean_t wait;
  char * line;
  char temp_line[80];

  /* First check that we actually have messages to flush. */
  if (first_message == NULL)
    return;

  /*
    We're going to smash all messages together into one huge string,
    then split it up again in screen-wide chunks.
  */

  /* Find the total length of the concatenated string */
  line_length = 1; /* We need at least 1 char for the trailing null */

  /* Loop through the message list and calculate how much space we need */
  for (msg = first_message; msg != NULL; msg = msg->next_message)
  {
    /* +1 is for the padding spaces between messages and at the end */
    line_length += strlen(msg->text) + 1;
  }

  /* Allocate memory to hold the concatenated string */
  line = malloc(sizeof(char) * line_length);
  if (line == NULL) out_of_memory();
  mem_alloc.chars += line_length;

  /* Empty the string */
  strcpy(line, "");

  /* Concatenate each message to the big string */
  while (first_message != NULL)
  {
    strcat(line, first_message->text);
    
    /* Add a space between messages and at the end. */
    strcat(line, " ");
    
    /* Move to the next message and delete the old one */
    msg = first_message; /* Save where the old message is */
    first_message = first_message->next_message; /*Move on to the next message*/
    
    /* Move the old message to the message history. */
    msg->next_message = message_history;
    message_history = msg;

    /* Delete the old message; we won't need it anymore */
/*    mem_alloc.chars -= strlen(msg->text) + 1;
    free(msg->text);
    free(msg);
    mem_alloc.messages--;*/
    
    msg = NULL;
  }
  
  /* Reset these; they aren't valid any more */
  last_message = NULL;
  first_message = NULL;
  
  /* It's time to print the message... */

  /* How much space do we need for "[MORE]"? */
  more_length = strlen(MORE_MARKER);

  /*
    A "window" will iterate across the concatenated string. The
    window starts (print_offset) at the beginning of the string, and
    extends for a number of characters to the right (print_pos).
    
    During each iteration, the window size will be set to a value
    close to the width of the screen. The window will then keep being
    decreased by 1 character until a space is encountered at the right
    end of the window (this is done so we will split the string
    between whole words rather than in the middle of a word). The text
    currently in the window will be displayed and the window shifted
    print_pos characters to the right (so the new left edge will
    "touch" the old right edge). The whole thing will then be repeated
    until we reach the end of the string.
  */

  /* Position the window start at character 0 */
  print_offset = 0;

  /* We'll start printing on the first line */
  l = 0;

  /* -1 since the \0 is included in line_length */
  while (print_offset < line_length - 1)
  {
    /*
      The message should be cleared every time we start over from the
      top.
    */
    if (l == 0)
      mb_erase();

    /*
      Set the window size to something close to the width of the
      screen, or to the length of the remaining string if it is
      smaller than the screen.
    */

    /*
      If we're on the last line of the message buffer, we need to
      reserve room for the more marker as well.
    */

    /* Try to print a full line. */
    print_pos = MIN(line_length - print_offset - 1, MESSAGE_BAR_WIDTH);

    wait = false;

    if ((l == MESSAGE_LINES - 1) &&
	force_wait)
    {
      wait = true;
    }

    /*
      If we're on the last line and the remaining text (in line) won't
      fit on a single line.
    */
    if ((l == MESSAGE_LINES - 1) &&
	(line_length - 1 - print_offset >= MESSAGE_BAR_WIDTH - 1))
    {
      wait = true;
    }
    
    /* Reserve room for the more marker if it's needed. */
    if (wait == true)
      print_pos = MESSAGE_BAR_WIDTH - more_length - 1;

    /* Find the last space in this substring. */
    temp_pos = print_pos;
    
    while ((line[print_offset + temp_pos] != ' ') && (temp_pos > 0))
    {
      temp_pos--;
    }

    /*
      If we found a space, temp_pos will be more than zero. If no
      space was found, there was a very long word which we should just
      wrap between lines so we'll just leave print_pos at the end of
      the line.
    */
    if (temp_pos > 0)
      print_pos = temp_pos;
    
    /* Retrieve the text in the current window. */
    strncpy(temp_line, &line[print_offset], print_pos);
    temp_line[print_pos] = '\0';
    
    /* Print the text we're currently viewing. */
    mb_move(l, 0);
    mb_addstr(temp_line);
/*    wclrtoeol(message_bar);*/

    /* Shift the window forward. +1 to skip the space. */
    print_offset += print_pos + 1;

    if ((print_offset >= line_length - 1) &&
	force_wait)
    {
      wait = true;
    }

    if (wait)
    {
      /* We should display a "more" marker and pause on this line. */
      mb_more();
      wait_continue();

      /* Restart printing at the top line */
      l = 0;
    }
    else
    {
      mb_flush();

      /* Move to the next line */
      l++;
    }
  }

  mem_alloc.chars -= line_length;
  free(line);
  
  return;
} /* _msgflush_internal */



/*
  Steps through previous messages and displays them in the message
  bar. history_index remembers the position we're at in the buffer;
  this is incremented for each call to this function, so subsequent
  calls will display the next message, then the one after that, and so
  on. To restart the history browsing, set history_index to 0 (this is
  currently done at the start of each player turn, and upon pressing
  the "cancel" key).
*/
/*
  RFE: This works, but it doesn't handle line wrapping. I'm also not
  sure how to handle individual messages that are bigger than the
  message bar. Perhaps a variation of a queue_msg/msgflush_* call can
  be achieved that doesn't re-queue the message.

  The best would probably be to save each "page" of three lines, so
  that the history is displayed in the same way as the original
  messages.
*/
void history(void)
{
  unsigned int i;
  message_t * temp;

  temp = message_history;

  /* If we don't have any messages in our history, say so. */
  if (temp == NULL)
  {
  no_more:
    mb_erase();
    mb_addstr("(No more messages in history)");
    mb_erase();
    return;
  }

  /*
    history_index is the message we wish to see. Browse the history
    until we reach our wanted message or the end of the list.
  */
  for (i = 0; i < history_index; i++)
  {
    temp = temp->next_message;

    if (temp == NULL)
    {
      /* We got to the end of the list; nothing more to be done here */
      goto no_more;
    }
  }

  /* Display the message */
  mb_erase();
  constrained_wrap(20, 0, 39, temp->text);/*mb_addstr(temp->text);*/
  mb_flush();

  /*
    Move to the next message; the next time this function is called,
    we'll look for this index instead.
  */
  history_index++;

  return;
} /* history */



/*
  Displays a message and waits for the user to respond with a (y)es or
  a (n)o.
  
  Returns:
    >= 1 if the user accepted
    -1 if the used cancelled
    0 if the user answered no

  Note: The -1 return value should probably be considered a "no", so
  testing for (ret < 1) rather than == 0 might be a good idea.
*/
signed int prompt_yn(const char * msg)
{
  int key = 0;
  
  if (msg != NULL)
    queue_msg(msg);

  queue_msg("(y/n)");

  msgflush_nowait();

  while (1)
  {
    key = get_keypress();

    if (key == 'y' || key_to_action(key) == action_select)
    {
      clear_msgbar();
      return 1;
    }
    else if (key == 'n')
    {
      clear_msgbar();
      return 0;
    }
    else if (key_to_action(key) == action_cancel)
    {
      clear_msgbar();
      return -1;
    }
  }

  /* We'll never get here */
  /*return -1;*/
} /* prompt_yn */



/*
  Clears the message bar.
*/
void clear_msgbar()
{
  mb_erase();
  mb_flush();

  return;
} /* clear_msgbar */



/*
  Glues (implodes, concatenates) S1 and S2 together (with a space
  between) and adds it to the message queue. The first character will
  be made uppercase.
 */
void msg_glue(const char * s1, const char * s2)
{
  char * temp;

  if ((s1 == NULL) || (s2 == NULL))
    return;

  temp = malloc(strlen(s1) + strlen(s2) + 2);

  if (temp == NULL)
    out_of_memory();

  sprintf(temp, "%s %s", s1, s2);
  upperfirst(temp);
  queue_msg(temp);

  free(temp);

  return;
} /* glue_msg */
