#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>

#include "main.h"
#include "saveload.h"
#include "ui.h"
#include "string.h"
#include "stuff.h"



/*
  Tries to load the players savefile. If successful, a pointer to the
  loaded game is returned, otherwise NULL.
*/
game_t * try_to_load_game(void)
{
  char * savefile_path;
  game_t * loaded_game;
  FILE * savefile;

  loaded_game = NULL;

  /* Where is the savefile? */
  savefile_path = get_file_path(SAVE_FILENAME);

  /* Try to open it... */

  /*
    Since we want to read raw structs, we need to open the file as
    binary. This is ignored by *nix but needed on Windows. Thanks to
    Abdullah A. Hassan for pointing this out.
  */
  savefile = fopen(savefile_path, "rb");

  /* We don't need this anymore. */
  free(savefile_path);
  savefile_path = NULL;

  /* Could we open the savefile? */
  if (savefile != NULL)
  {
    /* Yes; try to load a game from it. */
    loaded_game = restore_savefile(savefile);
    fclose(savefile);

    /*
      If we succeeded loading the game, we should remove the
      savefile.
    */
    if (loaded_game != NULL)
    {
      delete_savefile();
    }
  }

  return loaded_game;
} /* try_to_load_game */



/*
  Deletes the players savefile.
*/
void delete_savefile()
{
  char * savefile_path;
  signed int ret;

  savefile_path = get_file_path(SAVE_FILENAME);

  ret = remove(savefile_path);

  free(savefile_path);

  if (ret == -1)
  {
    saveload_abort("Couldn't delete old savefile.", false);
  }

  return;
} /* delete_savefile */



/*
  Attempts to save the current game.
*/
void try_to_save_game()
{
  blean_t success;
  char * savefile_path;
  FILE * savefile;
 
  /* Where is the savefile? */
  savefile_path = get_file_path(SAVE_FILENAME);

  build_saveload_table();
  add_game_to_save_table(game);

  /* Open the savefile */
  savefile = fopen(savefile_path, "wb");

  if (savefile == NULL)
  {
    success = false;
  }
  else
  {   
    success = write_savefile(savefile);
    fclose(savefile);
  }

  /* We don't need this anymore. */
  free(savefile_path);
  savefile_path = NULL;
  
  /*
    We won't *wipe* the table since the items are still in-game and
    will be taken care of by shutdown_everything().
  */
  del_saveload_table(false);
    
  if (success == false)
  {
    /*
      Game wasn't saved - tell the player, then resume playing. It's
      not fair to crash at this point. :-(
    */
    queue_msg("Couldn't save game!");
  }
  else
  {
    /* Game saved - tell the player, then exit.*/
    queue_msg("Game saved.");
    msgflush_wait();
    
    shutdown_everything();
    exit(0);
  }

  return;
} /* try_to_save_game */



/*
  Writes the things in saved_stuff and saved_type to the file PATH.
*/
blean_t write_savefile(FILE * savefile)
{
  unsigned int i;
  unsigned int temp;
  blean_t success;

  if (savefile == NULL)
    return false;
  
  /* Save the number of things (excluding the null) */
  write_number_as_bytes(savefile, saveload_items - 1);

  /*
    Write everything in saved_stuff to the savefile, preceded by a
    type indicator byte (see SAVE_* in saveload.h. We won't write
    index 0, since this is always NULL.
  */
  for (i = 1; i < saveload_items; i++)
  {
    fputc(saveload_type[i], savefile);

    switch (saveload_type[i])
    {
      case SAVE_STRING:
	/*
	  Writing strings to savefile: First store the string length
	  (excluding the terminating \0) as a series of bytes (the
	  same method is used as for the number of entries in the
	  file, then write the actual string (no \0 needed since we
	  already know the length).
	*/
	temp = strlen(saveload_stuff[i]);
	write_number_as_bytes(savefile, temp);

	if (fwrite(saveload_stuff[i], 1, temp, savefile) < temp)
	  success = false;
	else
	  success = true;

	break;

      case SAVE_ITEM:
	success = write_item_to_savefile(savefile, (item_t *)saveload_stuff[i]);
	break;

      case SAVE_EFFECT:
	success = write_effect_to_savefile(savefile, (effect_t *)saveload_stuff[i]);
	break;

      case SAVE_CREATURE:
	success = write_creature_to_savefile(savefile, (creature_t *)saveload_stuff[i]);
	break;

      case SAVE_LEVEL:
	success = write_level_to_savefile(savefile, (level_t *)saveload_stuff[i]);
	break;

      case SAVE_TRAP:
	success = write_trap_to_savefile(savefile, (trap_t *)saveload_stuff[i]);
	break;

      case SAVE_GAME:
	success = write_game_to_savefile(savefile, (game_t *)saveload_stuff[i]);
	break;

      default:
	saveload_abort("BUG: Trying to save unknown type.", false);
	success = false;
    	break;
    }

    if (success == false)
      return true;
  }

  /* All writes succeeded! */
  return true;
} /* write_savefile */



/*
  Sets up an empty save table. Make sure saved_stuff and saved_type
  don't point to any allocated memory, since they will be changed.
*/
void build_saveload_table()
{
  saveload_items = 0;
  saveload_stuff = NULL;
  saveload_type = NULL;

  add_to_saveload_table(NULL, SAVE_NULL);

  return;
} /* build_saveload_table */



/*
  Adds the trap T to the save table.
*/
void add_trap_to_save_table(trap_t * t)
{
  if (t == NULL)
    return;

  add_trap_to_save_table(t->next_trap);

  add_to_saveload_table(t, SAVE_TRAP);

  return;
} /* add_trap_to_save_table */



/*
  Adds the effect E to the save table.
*/
void add_effect_to_save_table(effect_t * e)
{
  if (e == NULL)
    return;

  add_effect_to_save_table(e->next_effect);
  /*add_to_saveload_table(e->expire, SAVE_STRING);*/
  add_to_saveload_table(e, SAVE_EFFECT);

  return;
} /* add_effect_to_save_table */



/*
  Adds the creature C to the save table.
*/
void add_creature_to_save_table(creature_t * c)
{
  if (c == NULL)
    return;

  add_to_saveload_table(c, SAVE_CREATURE);
/*  add_to_saveload_table(c->name, SAVE_STRING);*/
  add_to_saveload_table(c->death_msg, SAVE_STRING);
  add_item_to_save_table(c->first_item);
/*  add_item_to_save_table(c->magic_weapon);*/
  add_effect_to_save_table(c->first_effect);
} /* add_creature_to_save_table */



/*
  Adds the item I to the save table.
*/
void add_item_to_save_table(item_t * i)
{
  if (i == NULL)
    return;

  add_to_saveload_table(i, SAVE_ITEM);
  add_to_saveload_table(i->label, SAVE_STRING);
  add_to_saveload_table(i->single_unid_name, SAVE_STRING);
  add_to_saveload_table(i->plural_unid_name, SAVE_STRING);
  add_to_saveload_table(i->single_id_name, SAVE_STRING);
  add_to_saveload_table(i->plural_id_name, SAVE_STRING);
  add_to_saveload_table(i->description, SAVE_STRING);
  add_item_to_save_table(i->next_item);
  add_item_to_save_table(i->child);

  return;
} /* add_item_to_save_table */



/*
  Adds the level L to the save table.
*/
void add_level_to_save_table(level_t * l)
{
  unsigned int i;

  if (l == NULL)
    return;

  add_to_saveload_table(l, SAVE_LEVEL);
  add_to_saveload_table(l->name, SAVE_STRING);
  add_to_saveload_table(l->message, SAVE_STRING);
  add_trap_to_save_table(l->first_trap);
  add_item_to_save_table(l->first_item);

  for (i = 0; i < l->creatures; i++)
    add_creature_to_save_table(l->creature[i]);

  return;
} /* add_level_to_save_table */



/*
  Adds the game G to the save table.
*/
void add_game_to_save_table(game_t * g)
{
  unsigned int i;
  
  if (g == NULL)
    return;

  add_to_saveload_table(g, SAVE_GAME);
  add_creature_to_save_table(g->native);
  add_creature_to_save_table(g->player);
  add_item_to_save_table(g->first_facet);

  for (i = 0; i < LEVELS; i++)
    add_level_to_save_table(game->level_list[i]);

  for (i = 0; i < ITEMS; i++)
    add_item_to_save_table(game->item_template[i]);

  return;
} /* add_game_to_save_table */



/*
  Adds pointer P to the save/load table. TYPE should be one of the
  SAVE_* constants in saveload.h, and control what type the pointer
  will be cast back into.
 */
void add_to_saveload_table(void * p, const unsigned int type)
{
  if (get_saveload_index(p) != -1)
    return;

  saveload_items++;

  saveload_stuff = realloc(saveload_stuff, sizeof(void *) * saveload_items);
  
  if (saveload_stuff == NULL)
    out_of_memory();

  saveload_type = realloc(saveload_type, sizeof(unsigned int) * saveload_items);
  
  if (saveload_type == NULL)
    out_of_memory();

  saveload_stuff[saveload_items - 1] = p;
  saveload_type[saveload_items - 1] = type;
  
  return;
} /* add_to_saveload_table */



/*
  Returns the index of pointer P in the save/load table. If the
  pointer isn't in the table, -1 is returned.
*/
saveptr_t get_saveload_index(void * p)
{
  unsigned int i;

  for (i = 0; i < saveload_items; i++)
  {
    if (saveload_stuff[i] == p)
      return i;
  }

  return -1;
} /* get_saveload_index */



/*
  Writes item I to SAVEFILE. All pointers will be replaced with their
  index in the save/load table. Returns true on success, false on
  failure. SAVEFILE must be an open and writeable file.
*/
blean_t write_item_to_savefile(FILE * savefile, item_t * i)
{
  item_t local;
  unsigned int written;

  memcpy(&local, i, sizeof(item_t));

  local.label = (char *)get_saveload_index(local.label);
  local.single_unid_name = (char *)get_saveload_index(local.single_unid_name);
  local.plural_unid_name = (char *)get_saveload_index(local.plural_unid_name);
  local.single_id_name = (char *)get_saveload_index(local.single_id_name);
  local.plural_id_name = (char *)get_saveload_index(local.plural_id_name);
  local.description = (char *)get_saveload_index(local.description);
  local.inventory = (creature_t *)get_saveload_index(local.inventory);
  local.location = (level_t *)get_saveload_index(local.location);
  local.stack = (item_t *)get_saveload_index(local.stack);
  local.child = (item_t *)get_saveload_index(local.child);
  local.prev_item = (item_t *)get_saveload_index(local.prev_item);
  local.next_item = (item_t *)get_saveload_index(local.next_item);

  written = fwrite(&local, sizeof(item_t), 1, savefile);
  
  if (written < 1)
    return false;
  else
    return true;
} /* write_item_to_savefile */



/*
  Writes effect E to SAVEFILE. See write_item_to_savefile().
*/
blean_t write_effect_to_savefile(FILE * savefile, effect_t * e)
{
  effect_t local;
  unsigned int written;

  memcpy(&local, e, sizeof(effect_t));

  local.affecting = (creature_t *)get_saveload_index(local.affecting);
  local.next_effect = (effect_t *)get_saveload_index(local.next_effect);
  /*local.expire = (char *)get_saveload_index(local.expire);*/

  written = fwrite(&local, sizeof(effect_t), 1, savefile);

  if (written < 1)
    return false;
  else
    return true;
} /* write_effect_to_savefile */



/*
  Writes creature C to SAVEFILE. See write_item_to_savefile().
*/
blean_t write_creature_to_savefile(FILE * savefile, creature_t * c)
{
  creature_t local;
  unsigned int written;

  memcpy(&local, c, sizeof(creature_t));

/*  local.name = (char *)get_saveload_index(local.name);*/
  local.death_msg = (char *)get_saveload_index(local.death_msg);
  local.location = (level_t *)get_saveload_index(local.location);
  local.first_item = (item_t *)get_saveload_index(local.first_item);
/*  local.magic_weapon = (item_t *)get_saveload_index(local.magic_weapon);*/
  local.first_effect = (effect_t *)get_saveload_index(local.first_effect);

  written = fwrite(&local, sizeof(creature_t), 1, savefile);
  
  if (written < 1)
    return false;
  else
    return true;
} /* write_creature_to_savefile */



/*
  Writes level L to SAVEFILE. See write_item_to_savefile().
*/
blean_t write_level_to_savefile(FILE * savefile, level_t * l)
{
  level_t local;
  unsigned int i;
  unsigned int y;
  unsigned int written;
  creature_t * tp;

  memcpy(&local, l, sizeof(level_t));

  if (local.map == NULL)
    local.map = 0;

  local.name = (char *)get_saveload_index(local.name);
  local.message = (char *)get_saveload_index(local.message);
  local.first_item = (item_t *)get_saveload_index(local.first_item);
  local.first_trap = (trap_t *)get_saveload_index(local.first_trap);

  written = fwrite(&local, sizeof(level_t), 1, savefile);

  if (written < 1)
    return false;
  
  for (i = 0; i < local.creatures; i++)
  {
    tp = (creature_t *)get_saveload_index(local.creature[i]);
    written = fwrite(&tp, sizeof(creature_t *), 1, savefile);

    if (written < 1)
      return false;
  }

  if (local.map != 0)
  {
    for (y = 0; y < local.size_y; y++)
    {
      written = fwrite(local.map[y], sizeof(tile_t), local.size_x, savefile);
      written += fwrite(local.memory[y], sizeof(tile_t), local.size_x, savefile);
      
      if (written < local.size_x * 2)
	return false;
    }
  }
  
  written = fwrite(local.encounter, sizeof(encounter_t), local.encounters, savefile);

  if (written < local.encounters)
    return false;

  /* We have successfully written the level to disk. */
  return true;
} /* write_level_to_savefile */



/*
  Writes trap T to SAVEFILE. See write_item_to_savefile().
*/
blean_t write_trap_to_savefile(FILE * savefile, trap_t * t)
{
  trap_t local;
  unsigned int written;

  memcpy(&local, t, sizeof(trap_t));

  local.location = (level_t *)get_saveload_index(local.location);
  local.next_trap = (trap_t *)get_saveload_index(local.next_trap);
  local.prev_trap = (trap_t *)get_saveload_index(local.prev_trap);

  written = fwrite(&local, sizeof(trap_t), 1, savefile);
  
  if (written < 1)
    return false;
  else
    return true;
} /* write_trap_to_savefile */




/*
  Writes game G to SAVEFILE. See write_item_to_savefile().
*/
blean_t write_game_to_savefile(FILE * savefile, game_t * g)
{
  unsigned int i;
  game_t local;
  unsigned int written;

  memcpy(&local, g, sizeof(game_t));

  for (i = 0; i < ITEMS; i++)
    local.item_template[i] = (item_t *)get_saveload_index(local.item_template[i]);

  local.native = (creature_t *)get_saveload_index(local.native);
  local.player = (creature_t *)get_saveload_index(local.player);
  local.first_facet = (item_t *)get_saveload_index(local.first_facet);

  for (i = 0; i < LEVELS; i++)
    local.level_list[i] = (level_t *)get_saveload_index(local.level_list[i]);

  written = fwrite(&local, sizeof(game_t), 1, savefile);

  if (written < 1)
    return false;

  /* Succeeded writing game to file. */
  return true;
} /* write_game_to_savefile */



/*
  Loads a complete game from SAVEFILE. On success, a pointer to the
  game_t is returned. On error, the game will crash in a controlled
  manner. SAVEFILE should be an open and readable file. This will
  create a new save/load table, so any such already in existance
  should be freed first.
 */
game_t * restore_savefile(FILE * savefile)
{
  game_t * loaded_game;
  unsigned int i;
  unsigned int blocks_to_read;
  signed int type;
  unsigned int len;

  char * s;

  loaded_game = NULL;

  if (savefile == NULL)
    return NULL;

  rewind(savefile);

  /* Read the number of things in this file. */
  blocks_to_read = read_number_as_bytes(savefile);

  build_saveload_table();
  
  for (i = 1; i < blocks_to_read + 1; i++)
  {
    type = fgetc(savefile);

    if (type == EOF)
      saveload_abort("Corrupt savefile.", true);

    switch (type)
    {
      case SAVE_STRING:
	len = read_number_as_bytes(savefile);
	s = malloc(len + 1);
	mem_alloc.chars += len + 1;
	fread(s, sizeof(char), len, savefile);
	s[len] = '\0';
	add_to_saveload_table(s, type);
/*	fprintf(stderr, "%s888\n", s);*/
	break;

      case SAVE_GAME:
	read_game_from_savefile(savefile);
	break;

      case SAVE_LEVEL:
	read_level_from_savefile(savefile);
	break;

      case SAVE_TRAP:
	read_trap_from_savefile(savefile);
	break;

      case SAVE_CREATURE:
	read_creature_from_savefile(savefile);
	break;

      case SAVE_ITEM:
	read_item_from_savefile(savefile);
	break;

      case SAVE_EFFECT:
	read_effect_from_savefile(savefile);
	break;
    }
  }

  loaded_game = restore_saved_pointers();

  del_saveload_table(false);

  return loaded_game;
} /* restore_savefile */



/*
  Reads an item_t from SAVEFILE and adds it to the save/load table. On
  error, saveload_abort() will be called and the game will exit.
*/
void read_item_from_savefile(FILE * savefile)
{
  item_t * local;
  unsigned int r;
  
  local = malloc(sizeof(item_t));
  
  mem_alloc.items++;
  
  if (local == NULL)
    out_of_memory();
  
  add_to_saveload_table(local, SAVE_ITEM);

  r = fread(local, sizeof(item_t), 1, savefile);

  if (r < 1)
    saveload_abort("Corrupt savefile.", true);

  return;
} /* read_item_from_savefile */



/*
  Reads an effect_t from SAVEFILE. See read_item_from_savefile().
*/
void read_effect_from_savefile(FILE * savefile)
{
  effect_t * local;
  unsigned int r;

  local = malloc(sizeof(effect_t));

  if (local == NULL)
    out_of_memory();

  mem_alloc.effects++;

  add_to_saveload_table(local, SAVE_EFFECT);

  r = fread(local, sizeof(effect_t), 1, savefile);

  if (r < 1)
    saveload_abort("Corrupt savefile.", true);

  return;
} /* read_effect_from_savefile */



/*
  Reads a trap_t from SAVEFILE. See read_item_from_savefile().
*/
void read_trap_from_savefile(FILE * savefile)
{
  trap_t * local;
  unsigned int r;

  local = malloc(sizeof(trap_t));

  mem_alloc.traps++;

  if (local == NULL)
    out_of_memory();

  add_to_saveload_table(local, SAVE_TRAP);

  r = fread(local, sizeof(trap_t), 1, savefile);
  
  if (r < 1)
    saveload_abort("Corrupt savefile.", true);

  return;
} /* read_trap_from_savefile */



/*
  Reads a creature_t from SAVEFILE. See read_item_from_savefile().
*/
void read_creature_from_savefile(FILE * savefile)
{
  creature_t * local;
  unsigned int r;

  local = malloc(sizeof(creature_t));

  mem_alloc.creatures++;

  if (local == NULL)
    out_of_memory();

  add_to_saveload_table(local, SAVE_CREATURE);

  r = fread(local, sizeof(creature_t), 1, savefile);

  if (r < 1)
    saveload_abort("Corrupt savefile.", true);

  return;
} /* read_creature_from_savefile */



/*
  Reads a game_t from SAVEFILE. See read_item_from_savefile().
*/
void read_game_from_savefile(FILE * savefile)
{
  game_t * local;
  unsigned int r;

  local = malloc(sizeof(game_t));

  if (local == NULL)
    out_of_memory();

  mem_alloc.games++;

  add_to_saveload_table(local, SAVE_GAME);

  r = fread(local, sizeof(game_t), 1, savefile);

  if (r < 1)
    saveload_abort("Corrupt savefile.", true);

  return;
} /* read_game_from_savefile */



/*
  Reads a level_t from SAVEFILE. See read_item_from_savefile().
*/
void read_level_from_savefile(FILE * savefile)
{
  level_t * local;
  unsigned int y;
  unsigned int r;

  local = malloc(sizeof(level_t));

  if (local == NULL)
    out_of_memory();

  mem_alloc.levels++;

  add_to_saveload_table(local, SAVE_LEVEL);

  r = fread(local, sizeof(level_t), 1, savefile);

  if (r < 1)
    saveload_abort("Corrupt savefile.", true);

  local->creature = malloc(sizeof(creature_t *) * local->creatures);

  if (local->creature == NULL)
    out_of_memory();

  r = fread(local->creature, sizeof(creature_t *), local->creatures, savefile);

  if (r < local->creatures)
    saveload_abort("Corrupt savefile.", true);

  if (local->map != 0)
  {
    /*
      Whatever is in map is the old address, i.e. garbage. We're just
      checking for nonzero to see if there WAS a map when we saved.
    */

    local->map = malloc(sizeof(tile_t *) * local->size_y);
    if (local->map == NULL) out_of_memory();
    local->memory = malloc(sizeof(tile_t *) * local->size_y);
    if (local->memory == NULL) out_of_memory();
    
    for (y = 0; y < local->size_y; y++)
    {
      local->map[y] = malloc(sizeof(tile_t) * local->size_x);
      if (local->map[y] == NULL) out_of_memory();
      local->memory[y] = malloc(sizeof(tile_t) * local->size_x);
      if (local->memory[y] == NULL) out_of_memory();
      
      fread(local->map[y], sizeof(tile_t), local->size_x, savefile);
      fread(local->memory[y], sizeof(tile_t), local->size_x, savefile);
    }
  }
  else
  {
    local->map = NULL;
    local->memory = NULL;
  }

  if (local->encounters == 0)
  {
    local->encounter = NULL;
  }
  else
  {
    local->encounter = malloc(sizeof(encounter_t) * local->encounters);

    if (local->encounter == NULL)
      out_of_memory();
    
    r = fread(local->encounter, sizeof(encounter_t), local->encounters, savefile);
    
    if (r < local->encounters)
      saveload_abort("Corrupt savefile.", true);
  }
  
  return;
} /* read_level_from_savefile */



/*
  Goes through all structures in the save/load table, replacing the
  pointer indices with real addresses. Returns a pointer to a complete
  game_t structure that contains all the stuff in the save/load table.
  If the save/load table lacks a game_t structure, NULL is returned.
*/
/*
  TODO: So what if there are more than one game_t in the save/load
  table, or structures that aren't referenced anywhere?
*/
game_t * restore_saved_pointers()
{
  unsigned int i;
  unsigned int j;

  trap_t * trap;
  level_t * level;
  creature_t * creature;
  effect_t * effect;
  item_t * item;
  game_t * loaded_game;

  loaded_game = NULL;

  for (i = 1; i < saveload_items; i++)
  {
    switch (saveload_type[i])
    {
      case SAVE_EFFECT:
	effect = saveload_stuff[i];
	effect->affecting = (creature_t *)get_saveload_pointer((saveptr_t)effect->affecting);
	effect->next_effect = (effect_t *)get_saveload_pointer((saveptr_t)effect->next_effect);
	/*effect->expire = (char *)get_saveload_pointer((unsigned int)effect->expire);*/
	break;

      case SAVE_LEVEL:
	level = saveload_stuff[i];
	level->name = (char *)get_saveload_pointer((saveptr_t)level->name);
	level->message = (char *)get_saveload_pointer((saveptr_t)level->message);
	level->first_item = (item_t *)get_saveload_pointer((saveptr_t)level->first_item);
	level->first_trap = (trap_t *)get_saveload_pointer((saveptr_t)level->first_trap);

	for (j = 0; j < level->creatures; j++)
	  level->creature[j] = (creature_t *)get_saveload_pointer((saveptr_t)level->creature[j]);
	break;

      case SAVE_CREATURE:
	creature = saveload_stuff[i];
/*	creature->name = (char *)get_saveload_pointer((saveptr_t)creature->name);*/
	creature->death_msg = (char *)get_saveload_pointer((saveptr_t)creature->death_msg);
	creature->location = (level_t *)get_saveload_pointer((saveptr_t)creature->location);
	creature->first_item = (item_t *)get_saveload_pointer((saveptr_t)creature->first_item);
/*	creature->magic_weapon = (item_t *)get_saveload_pointer((saveptr_t)creature->magic_weapon);*/
	creature->first_effect = (effect_t *)get_saveload_pointer((saveptr_t)creature->first_effect);
	break;

      case SAVE_TRAP:
	trap = saveload_stuff[i];
	trap->location = (level_t *)get_saveload_pointer((saveptr_t)trap->location);
	trap->next_trap = (trap_t *)get_saveload_pointer((saveptr_t)trap->next_trap);
	trap->prev_trap = (trap_t *)get_saveload_pointer((saveptr_t)trap->prev_trap);
	break;

      case SAVE_GAME:
	if (loaded_game != NULL)
	  break;

	loaded_game = saveload_stuff[i];

	loaded_game->native = (creature_t *)get_saveload_pointer((saveptr_t)loaded_game->native);
	loaded_game->player = (creature_t *)get_saveload_pointer((saveptr_t)loaded_game->player);
	loaded_game->first_facet =
	  (item_t *)get_saveload_pointer((saveptr_t)loaded_game->first_facet);

	for (j = 0; j < ITEMS; j++)
	  loaded_game->item_template[j] =
	    (item_t *)get_saveload_pointer((saveptr_t)loaded_game->item_template[j]);

	for (j = 0; j < LEVELS; j++)
	{
	  loaded_game->level_list[j] =
	    (level_t *)get_saveload_pointer((saveptr_t)loaded_game->level_list[j]);
	}
	break;

      case SAVE_ITEM:
	item = saveload_stuff[i];
        item->label = (char *)get_saveload_pointer((saveptr_t)item->label);
        item->single_unid_name =
	  (char *)get_saveload_pointer((saveptr_t)item->single_unid_name);
        item->plural_unid_name =
	  (char *)get_saveload_pointer((saveptr_t)item->plural_unid_name);
        item->single_id_name = (char *)get_saveload_pointer((saveptr_t)item->single_id_name);
        item->plural_id_name = (char *)get_saveload_pointer((saveptr_t)item->plural_id_name);
        item->description = (char *)get_saveload_pointer((saveptr_t)item->description);
        item->inventory = (creature_t *)get_saveload_pointer((saveptr_t)item->inventory);
        item->location = (level_t *)get_saveload_pointer((saveptr_t)item->location);
        item->stack = (item_t *)get_saveload_pointer((saveptr_t)item->stack);
        item->child = (item_t *)get_saveload_pointer((saveptr_t)item->child);
        item->prev_item = (item_t *)get_saveload_pointer((saveptr_t)item->prev_item);
        item->next_item = (item_t *)get_saveload_pointer((saveptr_t)item->next_item);
	break;
    }
  }

  return loaded_game;
} /* restore_saved_pointers */



/*
  Writes NUMBER to WHERE encoded as a string of bytes. The number will
  be divided into any number of bytes necessary, split into chunks of
  255, followed by the remainder. Returns false on error, true on
  success. WHERE should be an open and writeable file. See also
  read_number_as_bytes().
*/
blean_t write_number_as_bytes(FILE * where, const unsigned int number)
{
  unsigned int temp;

  if (where == NULL)
    return false;

  temp = number;

  while (temp > 255)
  {
    if (fputc(255, where) == EOF)
      return false;

    temp -= 255;
  }
  
  if (fputc(temp, where) == EOF)
    return false;
  
  return true;
} /* write_number_as_bytes */



/*
  Reads a number from WHERE encoded as a string of bytes. All bytes
  read will be summed until (including) one less than 255 is
  encountered. On error, -1 is returned. WHERE should be an open and
  readable file. See also write_number_as_bytes().
*/
signed int read_number_as_bytes(FILE * where)
{
  signed int temp;
  unsigned int total;

  total = 0;

  temp = 255;

  while (temp == 255)
  {
    temp = fgetc(where);
 
    if (temp == EOF)
      return -1;

    total += temp;
  }
  
  return total;
} /* read_number_as_bytes */



/*
  Returns the address of entry INDEX in the save/load table.
*/
void * get_saveload_pointer(unsigned int index)
{
  if (index < saveload_items)
  {
    return saveload_stuff[index];
  }

  return NULL;
} /* get_saveload_pointer */



/*
  WIPE will be passed directly to del_saveload_table() - if there's
  data in saveload_stuff that's not referenced anywhere else (typical
  when loading), it should be true. If the data is also accessible
  through game (typical when saving, since we just create copies
  anyway), it should be false.
 */
void saveload_abort(const char * error, const blean_t wipe)
{
  shutdown_everything();
  
  del_saveload_table(wipe);
  
  printf("Error: %s\n", error);

  exit(1);
} /* saveload_abort */



/*
  Deletes the saveload table. If WIPE is true, all pointers will be
  free()d. If false, the memory will remain allocated but the table
  itself will still be removed.
*/
void del_saveload_table(const blean_t wipe)
{
  unsigned int i;

  if (wipe)
  {
    for (i = 0; i < saveload_items; i++)
    {
      free(saveload_stuff[i]);
    }
  }

  free(saveload_stuff);
  saveload_stuff = NULL;
  free(saveload_type);
  saveload_type = NULL;
  saveload_items = 0;
  
  return;
} /* del_saveload_table */
