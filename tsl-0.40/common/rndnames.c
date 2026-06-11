#include <stdlib.h>

#include "stuff.h"
#include "rndnames.h"

char * potion_name[] =
{
  "muddy potion",
  "blood-red potion",
  "slimy green potion",
  "milky potion",
  "clear potion", /* 5th */
  "clotted potion",
  "golden potion",
  "silver potion",
  "shimmering potion",
  "brown potion", /* 10th */
  "sickly yellow potion",
  "effervescent potion",
  "bubbly potion",
  "rusty potion",
  "black potion", /* 15th */
  "crystal bottle",
  "cyan potion",
  "tear-shaped vial",
  "small crystal flask",
  "violet potion" /* 20th */
};

gent_t potion_gent[] =
{
  gent_potion,
  gent_potion,
  gent_potion,
  gent_potion,
  gent_potion,
  gent_potion,
  gent_potion,
  gent_potion,
  gent_potion,
  gent_potion,
  gent_potion,
  gent_potion,
  gent_potion,
  gent_potion,
  gent_potion,
  gent_potion,
  gent_potion,
  gent_potion,
  gent_potion,
  gent_potion,

/*  gent_potion_muddy,
  gent_potion_blood,
  gent_potion_slimy,
  gent_potion_milky,
  gent_potion_clear,
  gent_potion_clotted,
  gent_potion_golden,
  gent_potion_silver,
  gent_potion_shimmering,
  gent_potion_brown,
  gent_potion_sickly,
  gent_potion_effervescent,
  gent_potion_bubbly,
  gent_potion_rusty,
  gent_potion_black,
  gent_potion_crystal_bottle,
  gent_potion_cyan,
  gent_potion_tear,
  gent_potion_crystal_flask,
  gent_potion_violet*/
};

item_article_t potion_article[] =
{
  article_i_a,
  article_i_a,
  article_i_a,
  article_i_a,
  article_i_a, /* 5th */
  article_i_a,
  article_i_a,
  article_i_a,
  article_i_a,
  article_i_a, /* 10th */
  article_i_a,
  article_i_an,
  article_i_a,
  article_i_a,
  article_i_a, /* 15th */
  article_i_a,
  article_i_a,
  article_i_a,
  article_i_a,
  article_i_a /* 20th */
};

char * wand_name[] =
{
  "bone wand",
  "silver wand",
  "iron wand",
  "golden wand",
  "crystal wand", /* 5th */
  "ornamented wand",
  "jewelled wand",
  "black wand",
  "wooden wand",
  "ivory wand", /* 10th */
  "barbed wand",
  "peculiar wand",
  "twisted wand",
  "plain wand",
  "" /* 15th */
};

item_article_t wand_article[] =
{
  article_i_a,
  article_i_a,
  article_i_an,
  article_i_a,
  article_i_a, /* 5th */
  article_i_an,
  article_i_a,
  article_i_a,
  article_i_a,
  article_i_an, /* 10th */
  article_i_a,
  article_i_a,
  article_i_a,
  article_i_a,
  article_i_a /* 15th */
};

char * scroll_name[] =
{
  "scroll labeled DOOM",
  "scroll labeled HAHAHA",
  "scroll labeled CURSES",
  "scroll labeled FOR MY LOVE",
  "scroll labeled READ ME", /* 5th */
  "scroll labeled LOLWUT",
  "scroll labeled MORE",
  "scroll labeled LESS",
  "scroll labeled DEAR JOURNAL",
  "scroll labeled THE END", /* 10th */
  "scroll labeled TROLOLOLO",
  "scroll labeled WEXJUU",
  "scroll labeled VIED EMPINA",
  "scroll labeled SCIZZORZ",
  "scroll labeled SCROLLORZ", /* 15th */
  "scroll labeled KAOZ",
  "scroll labeled SIGSEGV",
  "scroll labeled MOVE ZIG",
  "scroll labeled INVOICE",
  "scroll labeled Q9000" /* 20th */
};

char * scroll_name_plural[] =
{
  "scrolls labeled DOOM",
  "scrolls labeled HAHAHA",
  "scrolls labeled CURSES",
  "scrolls labeled FOR MY LOVE",
  "scrolls labeled READ ME", /* 5th */
  "scrolls labeled LOLWUT",
  "scrolls labeled MORE",
  "scrolls labeled LESS",
  "scrolls labeled DEAR JOURNAL",
  "scrolls labeled THE END", /* 10th */
  "scrolls labeled TROLOLOLO",
  "scrolls labeled WEXJUU",
  "scrolls labeled VIED EMPINA",
  "scrolls labeled SCIZZORZ",
  "scrolls labeled SCROLLORZ", /* 15th */
  "scrolls labeled KAOZ",
  "scrolls labeled SIGSEGV",
  "scrolls labeled MOVE ZIG",
  "scrolls labeled INVOICE",
  "scrolls labeled Q9000" /* 20th */
};

item_article_t scroll_article[] =
{
  article_i_a,
  article_i_a,
  article_i_a,
  article_i_a,
  article_i_a, /* 5th */
  article_i_a,
  article_i_a,
  article_i_a,
  article_i_a,
  article_i_a, /* 10th */
  article_i_a,
  article_i_a,
  article_i_a,
  article_i_a,
  article_i_a, /* 15th */
  article_i_a,
  article_i_a,
  article_i_a,
  article_i_a,
  article_i_a /* 20th */
};



/*
  Returns the name to be used for an unidentified potion with item
  number ITEM. This is fetched from the list of random potion
  appearances. If the item doesn't have any random appearance already,
  one will be randomly selected.
*/
char * get_potion_name(const unsigned int item)
{
  signed int i;

  /* Try to find any name associatied with ITEM. */
  for (i = 0; i < POTION_APPEARANCES; i++)
  {
    if (potion_map[i] == item)
      return potion_name[i];
  }

  /* If we got here, no name was found. Try to assign one. */
  i = randomize_potion(item);

  /* If randomize_potion returned a valid result, return that name. */
  if ((i >= 0) &&
      (i < POTION_APPEARANCES))
  {
    return potion_name[i];
  }

  /* We failed to select a name for ITEM. */
  return "buggy potion";
} /* get_potion_name */



/*
  Returns the graphical entity to be used for a potion with item
  number ITEM. This is fetched from the list of random potion
  appearances. If the item doesn't have any random appearance already,
  one will be randomly selected.
*/
gent_t get_potion_gent(const unsigned int item)
{
  signed int i;

  /* Try to find any gent associatied with ITEM. */
  for (i = 0; i < POTION_APPEARANCES; i++)
  {
    if (potion_map[i] == item)
      return potion_gent[i];
  }

  /* If we got here, no gent was found. Try to assign one. */
  i = randomize_potion(item);

  /* If randomize_potion returned a valid result, return that gent. */
  if ((i >= 0) &&
      (i < POTION_APPEARANCES))
  {
    return potion_gent[i];
  }

  /* We failed to select a gent for ITEM. */
  return gent_undefined;
} /* get_potion_name */



/*
  Returns the article to be used for the unidentified single name of
  ITEM. This is fetched from the list of random potion appearances. If
  the item doesn't have any random appearance already, one will be
  randomly selected.
*/
item_article_t get_potion_article(const unsigned int item)
{
  signed int i;

  /* Try to find any article associatied with ITEM. */
  for (i = 0; i < POTION_APPEARANCES; i++)
  {
    if (potion_map[i] == item)
      return potion_article[i];
  }

  /* If we got here, no article was found. Try to assign one. */
  i = randomize_potion(item);

  /* If randomize_potion returned a valid result, return that article. */
  if ((i >= 0) &&
      (i < POTION_APPEARANCES))
  {
    return potion_article[i];
  }

  /* We failed to select an article for ITEM. */
  return article_i_none;
} /* get_potion_article */



/*
  Assigns a unique name/article to all potions with item number
  ITEM. Returns the index chosen (to be used in potion_map[]), or -1
  on failure.
*/
signed int randomize_potion(const unsigned int item)
{
  unsigned int pos;
  unsigned int start;
  
  /* Pick a random index to start at. */
  start = tslrnd() % (POTION_APPEARANCES);
  pos = start + 1;

  while (true)
  {
    /* Move to the next item. Wrap around the end of the array if needed. */
    pos++;
    pos %= POTION_APPEARANCES;

    /* Is this index taken? */
    if (potion_map[pos] == 0)
    {
      /* It's not; reserve it for our item. */
      potion_map[pos] = item;    
      return pos;
    }

    /* Are we back where we started? No more indices to check... */
    if (pos == start)
      break;
  }

  /* Mission failed! */
  return -1;
} /* randomize_potion */



/*
  Returns the name of an unidentified wand with item number ITEM. This
  is fetched from the list of random wand appearances. If the item
  doesn't have any random appearance already, one will be randomly
  selected.
*/
char * get_wand_name(const unsigned int item)
{
  signed int i;

  /* Try to find any name associatied with ITEM. */
  for (i = 0; i < WAND_APPEARANCES; i++)
  {
    if (wand_map[i] == item)
      return wand_name[i];
  }

  /* If we got here, no name was found. Try to assign one. */
  i = randomize_wand(item);

  /* If randomize_wand returned a valid result, return that name. */
  if ((i >= 0) &&
      (i < WAND_APPEARANCES))
  {
    return wand_name[i];
  }

  /* We failed to select a name for ITEM. */
  return "buggy wand";
} /* get_wand_name */



/*
  Returns the article to be used for the unidentified single name of
  ITEM. This is fetched from the list of random wand appearances. If
  the item doesn't have any random appearance already, one will be
  randomly selected.
*/
item_article_t get_wand_article(const unsigned int item)
{
  signed int i;

  /* Try to find any article associatied with ITEM. */
  for (i = 0; i < WAND_APPEARANCES; i++)
  {
    if (wand_map[i] == item)
      return wand_article[i];
  }

  /* If we got here, no article was found. Try to assign one. */
  i = randomize_wand(item);

  /* If randomize_wand returned a valid result, return that article. */
  if ((i >= 0) &&
      (i < WAND_APPEARANCES))
  {
    return wand_article[i];
  }

  /* We failed to select an article for ITEM. */
  return article_i_none;
} /* get_wand_article */



/*
  Assigns a unique name/article to all wands with item number
  ITEM. Returns the index chosen (to be used in wand_map[]), or -1 on
  failure.
*/
signed int randomize_wand(const unsigned int item)
{
  unsigned int pos;
  unsigned int start;
  
  /* Pick a random index to start at. */
  start = tslrnd() % (WAND_APPEARANCES);
  pos = start + 1;

  while (true)
  {
    /* Move to the next item. Wrap around the end of the array if needed. */
    pos++;
    pos %= WAND_APPEARANCES;

    /* Is this index taken? */
    if (wand_map[pos] == 0)
    {
      /* It's not; reserve it for our item. */
      wand_map[pos] = item;    
      return pos;
    }

    /* Are we back where we started? No more indices to check... */
    if (pos == start)
      break;
  }

  /* Mission failed! */
  return -1;
} /* randomize_wand */



/*
  Returns the name of an unidentified scroll with item number
  ITEM. This is fetched from the list of random scroll appearances. If
  the item doesn't have any random appearance already, one will be
  randomly selected.
*/
char * get_scroll_name(const unsigned int item)
{
  signed int i;

  /* Try to find any name associatied with ITEM. */
  for (i = 0; i < SCROLL_APPEARANCES; i++)
  {
    if (scroll_map[i] == item)
      return scroll_name[i];
  }

  /* If we got here, no name was found. Try to assign one. */
  i = randomize_scroll(item);

  /* If randomize_scroll returned a valid result, return that name. */
  if ((i >= 0) &&
      (i < SCROLL_APPEARANCES))
  {
    return scroll_name[i];
  }

  /* We failed to select a name for ITEM. */
  return "buggy scroll";
} /* get_scroll_name */



/*
  Returns the plural name of an unidentified scroll with item number
  ITEM. This is fetched from the list of random scroll appearances. If
  the item doesn't have any random appearance already, one will be
  randomly selected.
*/
char * get_scroll_name_plural(const unsigned int item)
{
  signed int i;

  /* Try to find any name associatied with ITEM. */
  for (i = 0; i < SCROLL_APPEARANCES; i++)
  {
    if (scroll_map[i] == item)
      return scroll_name_plural[i];
  }

  /* If we got here, no name was found. Try to assign one. */
  i = randomize_scroll(item);

  /* If randomize_scroll returned a valid result, return that name. */
  if ((i >= 0) &&
      (i < SCROLL_APPEARANCES))
  {
    return scroll_name_plural[i];
  }

  /* We failed to select a name for ITEM. */
  return "buggy scrolls";
} /* get_scroll_name */



/*
  Returns the article to be used for the unidentified single name of
  ITEM. This is fetched from the list of random scroll appearances. If
  the item doesn't have any random appearance already, one will be
  randomly selected.
*/
item_article_t get_scroll_article(const unsigned int item)
{
  signed int i;

  /* Try to find any article associatied with ITEM. */
  for (i = 0; i < SCROLL_APPEARANCES; i++)
  {
    if (scroll_map[i] == item)
      return scroll_article[i];
  }

  /* If we got here, no article was found. Try to assign one. */
  i = randomize_scroll(item);

  /* If randomize_scroll returned a valid result, return that article. */
  if ((i >= 0) &&
      (i < WAND_APPEARANCES))
  {
    return wand_article[i];
  }

  /* We failed to select an article for ITEM. */
  return article_i_none;
} /* get_scroll_article */



/*
  Assigns a unique name/article to all scrolls with item number
  ITEM. Returns the index chosen (to be used in scroll_map[]), or -1
  on failure.
*/
signed int randomize_scroll(const unsigned int item)
{
  unsigned int pos;
  unsigned int start;
  
  /* Pick a random index to start at. */
  start = tslrnd() % (SCROLL_APPEARANCES);
  pos = start + 1;

  while (true)
  {
    /* Move to the next item. Wrap around the end of the array if needed. */
    pos++;
    pos %= SCROLL_APPEARANCES;

    /* Is this index taken? */
    if (scroll_map[pos] == 0)
    {
      /* It's not; reserve it for our item. */
      scroll_map[pos] = item;    
      return pos;
    }

    /* Are we back where we started? No more indices to check... */
    if (pos == start)
      break;
  }
  
  /* Mission failed! */
  return -1;
} /* randomize_scroll */



char * get_book_name(const unsigned int item)
{
  return "book";
} /* get_book_name */
