/*
  message.h

  This is the structure used for in-game message queuing (the ones
  displayed at the bottom-left of the screen). The messages are stored
  in a one-way linked list, with the first (oldest) message pointed to
  by FIRST_MESSAGE. LAST_MESSAGE is also provided, so we won't have to
  loop through all old messages just to add one at the end.

  Message flushing will linewrap the buffered messages and split them
  into several screens if there is too much to display at once.
  msgflush_wait() is used to grab the players attention and always
  prints [MORE] and waits for a keypress, even if there wasn't enough
  in the buffer to fill the message area. msgflush_nowait() just
  prints whatever was in the buffer, wrapping and splitting but
  without waiting at the end. This is used when we want more input
  from the player.
*/
#ifndef _MESSAGE_H_
#define _MESSAGE_H_

#include "main.h"

struct message_t
{
  char * text;
  message_t * next_message;
};

message_t * first_message;
message_t * last_message;

message_t * message_history;
unsigned int history_index;

#define MORE_MARKER "[MORE]"

void history(void);
void queue_msg(const char * text);
void init_messages(void);
void clear_messages(void);
void _msgflush_internal(const blean_t force_wait);
signed int prompt_yn(const char * msg);
void clear_msgbar(void);
void msg_glue(const char * s1, const char * s2);

#define msgflush_wait()    _msgflush_internal(true)
#define msgflush_nowait()  _msgflush_internal(false)

#endif
