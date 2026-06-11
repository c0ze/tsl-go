#include <stdlib.h>
#include <stdio.h>

#include "allui.h"

const unsigned int gent_width = 2;


/*
  Starts Allegro, loads bitmaps.
*/
void init_ui()
{
  init_messages();

  set_glyph_mode(options.glyph_mode);

  scroll_limit_y = 3;
  scroll_limit_x = 3;

  first_anim = NULL;

  realdisp = NULL;
  disp = NULL;
  tileset = NULL;
  tileset_rev = NULL;
  tileset_dim = NULL;
  small_font = NULL;

  /* We need this for text selection. */
  init_glyph_map();

  if (al_init() == 0 ||
      al_install_keyboard() == 0 ||
      al_install_mouse() == 0 ||
      al_init_image_addon() == 0 || /* We need this to load sprites */
      (timer = al_create_timer(0.05)) == NULL || /* For animations */
      (event_queue = al_create_event_queue()) == NULL ||
      al_install_joystick() == 0)
  {
    printf("Failed to initialize Allegro.");
    goto it_broke;
  }
  
  
  /*fprintf(stderr, "Joysticks: %d\n", al_get_num_joysticks());*/
  
  
  /*
    We will use a buffer that we scale up. This is the actual
    display.
  */
  
  al_set_new_display_flags(ALLEGRO_RESIZABLE);
  realdisp = al_create_display(SCREEN_W, SCREEN_H);

  if (realdisp == NULL)
  {
    printf("Failed to create display.");
    goto it_broke;
  }
  
  /* Now that we have a display we can set the title. */
  al_set_window_title(realdisp, TSL_NAME " " TSL_VERSION);

  /* This should conclude 1:1 for now. */
  configure_scaler();

  /*
    Only for d3d/ios/android???
    al_set_new_bitmap_flags(ALLEGRO_NO_PRESERVE_TEXTURE);
  */

  /* This is the buffer. */
  disp = al_create_bitmap(SCREEN_W, SCREEN_H);

  if (disp == NULL)
  {
    printf("Failed to set up buffer.");
    goto it_broke;
  }

  if (load_graphics() == false)
    goto it_broke;

  al_init_font_addon();

  if (load_fonts() == false)
  {
    printf("Error: failed to load fonts.");
    goto it_broke;
  }

  /* Set up the event queue */
  al_register_event_source(event_queue, al_get_display_event_source(realdisp));
  al_register_event_source(event_queue, al_get_keyboard_event_source());
  al_register_event_source(event_queue, al_get_timer_event_source(timer));
  al_register_event_source(event_queue, al_get_joystick_event_source()); 
  al_register_event_source(event_queue, al_get_mouse_event_source());

  al_start_timer(timer);

  /*
    anim_t * anim;
    anim = new_anim(5, 5);
    anim->type = anim_type_damage;
  */
  
  return;

it_broke:
  shutdown_everything();
  exit(1);
  
  return;
} /* init_ui */



/*
  Loads all graphics we need. Return false on failure.
*/
blean_t load_graphics()
{
/*  tileset = al_load_bitmap("assets/tileset.png");
  tileset_rev = al_load_bitmap("assets/tilerev.png");
  tileset_dim = al_load_bitmap("assets/tiledim.png");
  mainfont = al_load_bitmap("assets/font.png");
  mainfont_rev = al_load_bitmap("assets/fontrev.png");
  mainfont_dim = al_load_bitmap("assets/fontdim.png");*/
  
  if (load_bitmap(&tileset, "assets/tileset.png") == false ||
      load_bitmap(&tileset_rev, "assets/tilerev.png") == false ||
      load_bitmap(&tileset_dim, "assets/tiledim.png") == false ||
      load_bitmap(&mainfont, "assets/font.png") == false ||
      load_bitmap(&mainfont_rev, "assets/fontrev.png") == false ||
      load_bitmap(&mainfont_dim, "assets/fontdim.png") == false)
  {
    return false;
  }
  
  return true;
} /* load_graphics */



blean_t load_bitmap(ALLEGRO_BITMAP ** dest, const char * file)
{
  if (dest == NULL || file == NULL)
    return false;

  *dest = al_load_bitmap(file);

  if (*dest == NULL)
  {
    printf("Error: couldn't load \"%s\". Sorry.\n", file);
    return false;
  }

  return true;
} /* load_bitmap */



/*
  Loads fonts we need. Return false on failure.
*/
blean_t load_fonts(void)
{
  ALLEGRO_BITMAP * temp_bitmap;
  int range[2];

  temp_bitmap = al_load_bitmap("assets/smallfont.tga");

  if (temp_bitmap == NULL)
    return false;

  range[0] = 48;
  range[1] = 57;

  small_font = al_grab_font_from_bitmap(temp_bitmap, 1, range);

  al_destroy_bitmap(temp_bitmap);

  return true;
} /* load_fonts */



/*
  Shuts down Allegro, frees bitmaps.
*/
void shutdown_ui()
{
  clear_messages();

  if (tileset)
  {
    al_destroy_bitmap(tileset);
    tileset = NULL;
  }

  if (tileset_rev)
  {
    al_destroy_bitmap(tileset_rev);
    tileset_rev = NULL;
  }

  if (tileset_dim)
  {
    al_destroy_bitmap(tileset_dim);
    tileset_dim = NULL;
  }

  if (mainfont)
  {
    al_destroy_bitmap(mainfont);
    mainfont = NULL;
  }

  if (mainfont_rev)
  {
    al_destroy_bitmap(mainfont_rev);
    mainfont_rev = NULL;
  }

  if (mainfont_dim)
  {
    al_destroy_bitmap(mainfont_dim);
    mainfont_dim = NULL;
  }

  if (small_font)
  {
    al_destroy_font(small_font);
    small_font = NULL;
  }

  if (event_queue)
    al_destroy_event_queue(event_queue);

  al_uninstall_system();

  while (first_anim)
    del_anim(first_anim);

  return;
} /* shutdown_ui */



/*
  Stores at D_X and D_Y the X and Y coordinates of the terminal cell
  the mouse pointer is hovering over. Returns false if the mouse is
  outside the terminal area.
*/
blean_t mouse_pos(int * d_x, int * d_y)
{
  ALLEGRO_MOUSE_STATE mouse_state;

  int mouse_x;
  int mouse_y;

  al_get_mouse_state(&mouse_state);

  mouse_x = ((mouse_state.x - scaler_x_offset) / scaler_ratio);
  mouse_y = ((mouse_state.y - scaler_y_offset) / scaler_ratio);

  if (mouse_x < 0 || mouse_y < 0 || mouse_x > SCREEN_W || mouse_y > SCREEN_H)
    return false;

  *d_x = mouse_x / FONT_W;
  *d_y = mouse_y / FONT_H;

  return true;
} /* get_mouse_tile */



/*
  Updates the text selection rectangle.
*/
void select_text()
{
  term_sel.t = MIN(term_sel.down_y, term_sel.up_y);
  term_sel.b = MAX(term_sel.down_y, term_sel.up_y);
  term_sel.l = MIN(term_sel.down_x, term_sel.up_x);
  term_sel.r = MAX(term_sel.down_x, term_sel.up_x);

  return;
}



/*
  Copies what's in the selection rectangle to the clipboard (if available).
*/
void copy_text()
{
  int y;
  int x;
  int cell;
  int flags;

  char * buf;
  int p;
  char * seq;

  /*
    We want this buffer to be 4x terminal size. We need 1 extra
    character per row for line breaks and 1 for terminating NULL.
    This will let us store a full screen of UTF-8 - we won't even get
    close to filling it most of the time though.
  */
  buf = malloc((TERM_W + 1) * (TERM_H) * 4 + 1);

  if (buf == NULL)
    out_of_memory();

  /*
    We'll go through the selection rectangle cell by cell, convert
    each character to UTF-8 and try to push it onto the clipboard.
  */

  p = 0;

  for (y = term_sel.t; y <= term_sel.b ; y++)
  {
    /*
      Count how much padding (-1 characters) this row has, add a space
      at the start of the row in the buffer. This should make the map
      right-aligned.
    */
    for (x = term_sel.l; x <= term_sel.r; x++)
    {
      if (terminal[TERM_MAP(y, x)] == -1)
      {
	buf[p] = ' ';
	p++;
      }
    }
    
    /* Now start processing for real. */
    for (x = term_sel.l; x <= term_sel.r; x++)
    {
      cell = terminal[TERM_MAP(y, x)];
      flags = terminal_flags[TERM_MAP(y, x)];

      if (cell == -1)
      {
	/* Skip padding; we already have these at the left edge. */
	continue;
      }
      else if (flags & TERM_GENT)
      {
	/* This cell is a gent. */

	if (cell >= gent_max)
	  continue;

	/* Is there a UTF-8 encoding prepared? */
	seq = glyph_convert[cell];
	
	if (*seq != '\0')
	{
	  /* There is. Append it to the buffer. */

	  while (*seq)
	  {
	    buf[p] = *seq;
	    seq++;
	    p++;
	  }
	}
	else
	{
	  /* There isn't. Just use the ASCII glyph. */
	  buf[p] = glyph_map[cell];
	  p++;
	}
      }
      else if (flags & TERM_EXTENDED)
      {
	/* This cell is an extended character. Try to convert it to UTF-8. */

	switch (cell)
	{
	  case ST_VLINE:
	    buf[p] = (char)0xE2;  buf[p+1] = (char)0x94;  buf[p+2] = (char)0x82;  p += 3;
	    break;

	  case ST_HLINE:
	    buf[p] = (char)0xE2;  buf[p+1] = (char)0x94;  buf[p+2] = (char)0x80;  p += 3;
	    break;

	  case ST_SE:
	    buf[p] = (char)0xE2;  buf[p+1] = (char)0x94;  buf[p+2] = (char)0x8C;  p += 3;
	    break;

	  case ST_SW:
	    buf[p] = (char)0xE2;  buf[p+1] = (char)0x94;  buf[p+2] = (char)0x90;  p += 3;
	    break;

	  case ST_NE:
	    buf[p] = (char)0xE2;  buf[p+1] = (char)0x94;  buf[p+2] = (char)0x94;  p += 3;
	    break;

	  case ST_NW:
	    buf[p] = (char)0xE2;  buf[p+1] = (char)0x94;  buf[p+2] = (char)0x98;  p += 3;
	    break;

	  case ST_CROSS:
	    buf[p] = (char)0xE2;  buf[p+1] = (char)0x94;  buf[p+2] = (char)0xBC;  p += 3;
	    break;

	  case ST_LTEE:
	    buf[p] = (char)0xE2;  buf[p+1] = (char)0x94;  buf[p+2] = (char)0x9C;  p += 3;
	    break;

	  case ST_RTEE:
	    buf[p] = (char)0xE2;  buf[p+1] = (char)0x94;  buf[p+2] = (char)0xA4;  p += 3;
	    break;

	  case ST_TTEE:
	    buf[p] = (char)0xE2;  buf[p+1] = (char)0x94;  buf[p+2] = (char)0xAC;  p += 3;
	    break;

	  case ST_BTEE:
	    buf[p] = (char)0xE2;  buf[p+1] = (char)0x94;  buf[p+2] = (char)0xB4;  p += 3;
	    break;

	  case ST_FLOOR:
	    buf[p] = '.'; p++;
	    break;

	  default:
	    buf[p] = '?';
	    p++;
	    break;
	}
      }
      else
      {
	/* This is just a regular (presumably ASCII) character. */
	buf[p] = terminal[y * TERM_W + x];
	p++;
      }
    }

    /* We need newlines in the buffer */
    buf[p] = '\n';
    p++;
  }

  buf[p] = '\0';

  /* Platform-specific code neatly hidden away */
  put_clipboard(buf);

  free(buf);

  return;
} /* copy_text */



key_token_t get_keypress()
{
  ALLEGRO_EVENT ev;

  int cell_x;
  int cell_y;
  int ret;

  joy_press = 0;

  while (1)
  {
    if (al_is_event_queue_empty(event_queue))
      render_terminal();

    al_wait_for_event(event_queue, &ev);
    
    if (ev.type == ALLEGRO_EVENT_TIMER)
    {
      if (options.diagonals == DIAGONALS_SLOPPY)
      {
	if (!x_held && x_move > 0)
	  x_move--;
	else if (!x_held && x_move < 0)
	  x_move++;
	
	if (!y_held && y_move > 0)
	  y_move--;
	else if (!y_held && y_move < 0)
	  y_move++;
      }

      /* mb_t = (mb_t + 1) % MB_CURSOR_CYCLE; */
      animate();
    }
    else if (ev.type == ALLEGRO_EVENT_MOUSE_BUTTON_DOWN)
    {
      if (mouse_pos(&cell_x, &cell_y))
      {
	term_sel.active = true;
	term_sel.down_x = cell_x;
	term_sel.down_y = cell_y;
	term_sel.up_x = cell_x;
	term_sel.up_y = cell_y;
      }

      select_text();
    }
    else if (ev.type == ALLEGRO_EVENT_MOUSE_BUTTON_UP)
    {
      term_sel.active = false;

      copy_text();
    }
    else if (term_sel.active && ev.type == ALLEGRO_EVENT_MOUSE_AXES)
    {
      if (mouse_pos(&cell_x, &cell_y))
      {
	term_sel.up_x = cell_x;
	term_sel.up_y = cell_y;
      }

      select_text();
    }
    else if (ev.type == ALLEGRO_EVENT_MOUSE_LEAVE_DISPLAY)
    {
      term_sel.active = false;
    }
/*    else if (ev.type == ALLEGRO_EVENT_KEY_UP)
    {
      continue;
      }*/
    else if (ev.type == ALLEGRO_EVENT_KEY_CHAR &&
	     ev.keyboard.keycode == ALLEGRO_KEY_ENTER &&
	     ev.keyboard.modifiers & ALLEGRO_KEYMOD_ALT)
    {
      if (al_get_display_flags(realdisp) & ALLEGRO_FULLSCREEN_WINDOW)
	al_toggle_display_flag(realdisp, ALLEGRO_FULLSCREEN_WINDOW, false);
      else
	al_toggle_display_flag(realdisp, ALLEGRO_FULLSCREEN_WINDOW, true);

//      configure_scaler();
    }
    else if (options.diagonals &&
	     ev.type == ALLEGRO_EVENT_KEY_DOWN &&
	     (ev.keyboard.keycode == ALLEGRO_KEY_UP ||
	      ev.keyboard.keycode == ALLEGRO_KEY_DOWN ||
	      ev.keyboard.keycode == ALLEGRO_KEY_LEFT ||
	      ev.keyboard.keycode == ALLEGRO_KEY_RIGHT))
    {
      /*fprintf(stderr, "Down: %d\n", ev.keyboard.keycode);*/

      if (ev.keyboard.keycode == ALLEGRO_KEY_LEFT)
      {
	x_move = -5;
	x_held++;
      }
      else if (ev.keyboard.keycode == ALLEGRO_KEY_RIGHT)
      {
	x_move = 5;
	x_held++;
      }
      else if (ev.keyboard.keycode == ALLEGRO_KEY_UP)
      {
	y_move = -5;
	y_held++;
      }
      else if (ev.keyboard.keycode == ALLEGRO_KEY_DOWN)
      {
	y_move = 5;
	y_held++;
      }

      /*fprintf(stderr, "x_move: %d, y_move: %d\n", x_move, y_move);*/
      
      joy_press = 1;
    }
    else if (options.diagonals &&
	     ev.type == ALLEGRO_EVENT_KEY_CHAR &&
	     (ev.keyboard.keycode == ALLEGRO_KEY_UP ||
	      ev.keyboard.keycode == ALLEGRO_KEY_DOWN ||
	      ev.keyboard.keycode == ALLEGRO_KEY_LEFT ||
	      ev.keyboard.keycode == ALLEGRO_KEY_RIGHT))
    {
      continue;
    }
    else if (options.diagonals &&
	     ev.type == ALLEGRO_EVENT_KEY_UP &&
	     (ev.keyboard.keycode == ALLEGRO_KEY_UP ||
	      ev.keyboard.keycode == ALLEGRO_KEY_DOWN ||
	      ev.keyboard.keycode == ALLEGRO_KEY_LEFT ||
	      ev.keyboard.keycode == ALLEGRO_KEY_RIGHT))
    {
      /*fprintf(stderr, "Up: %d\n", ev.keyboard.keycode);
	fprintf(stderr, "x_move: %d, y_move: %d\n", x_move, y_move);*/

      if (ev.keyboard.keycode == ALLEGRO_KEY_LEFT ||
	  ev.keyboard.keycode == ALLEGRO_KEY_RIGHT)
      {
	x_held--;

	if (options.diagonals == DIAGONALS_STEP && !x_held)
	{
	  x_move = 0;
	}
      }
      else if (ev.keyboard.keycode == ALLEGRO_KEY_UP ||
	       ev.keyboard.keycode == ALLEGRO_KEY_DOWN)
      {
	y_held--;

	if (options.diagonals == DIAGONALS_STEP && !x_held)
	{
	  y_move = 0;
	}
      }

      if (options.diagonals != DIAGONALS_STEP && !y_held && !x_held)
      {
	goto check_fake_diagonals;
      }
    }
    else if (ev.type == ALLEGRO_EVENT_KEY_CHAR)
    {
      /*fprintf(stderr, "Char: %d\n", ev.keyboard.keycode);*/

      if (options.diagonals == DIAGONALS_STEP && ev.keyboard.keycode == ALLEGRO_KEY_PGDN)
      {
	goto check_fake_diagonals;
      }

      if (ev.keyboard.modifiers & ALLEGRO_KEYMOD_SHIFT)
      {
	switch (ev.keyboard.keycode)
	{
	  case ALLEGRO_KEY_PAD_1: return kt_shift_np1;
	  case ALLEGRO_KEY_PAD_2: return kt_shift_np2;
	  case ALLEGRO_KEY_PAD_3: return kt_shift_np3;
	  case ALLEGRO_KEY_PAD_4: return kt_shift_np4;
	  case ALLEGRO_KEY_PAD_5: return kt_shift_np5;
	  case ALLEGRO_KEY_PAD_6: return kt_shift_np6;
	  case ALLEGRO_KEY_PAD_7: return kt_shift_np7;
	  case ALLEGRO_KEY_PAD_8: return kt_shift_np8;
	  case ALLEGRO_KEY_PAD_9: return kt_shift_np9;
	  case ALLEGRO_KEY_PAD_0: return kt_shift_np0;

	  case ALLEGRO_KEY_UP:
//	    if (options.fake_diagonals == DIAGONALS_OFF)
	      return kt_shift_dir_up;
//	    else continue;

	  case ALLEGRO_KEY_DOWN:  return kt_shift_dir_down;
	  case ALLEGRO_KEY_LEFT:  return kt_shift_dir_left;
	  case ALLEGRO_KEY_RIGHT: return kt_shift_dir_right;
	}
      }
      else
      {
	switch (ev.keyboard.keycode)
	{
	  case ALLEGRO_KEY_PAD_1: return kt_np1;
	  case ALLEGRO_KEY_PAD_2: return kt_np2;
	  case ALLEGRO_KEY_PAD_3: return kt_np3;
	  case ALLEGRO_KEY_PAD_4: return kt_np4;
	  case ALLEGRO_KEY_PAD_5: return kt_np5;
	  case ALLEGRO_KEY_PAD_6: return kt_np6;
	  case ALLEGRO_KEY_PAD_7: return kt_np7;
	  case ALLEGRO_KEY_PAD_8: return kt_np8;
	  case ALLEGRO_KEY_PAD_9: return kt_np9;
	  case ALLEGRO_KEY_PAD_0: return kt_np0;
	  case ALLEGRO_KEY_UP:    return kt_dir_up;
	  case ALLEGRO_KEY_DOWN:  return kt_dir_down;
	  case ALLEGRO_KEY_LEFT:  return kt_dir_left;
	  case ALLEGRO_KEY_RIGHT: return kt_dir_right;
	}
      }

      switch (ev.keyboard.keycode)
      {
	case ALLEGRO_KEY_ENTER:      return '\n';
	case ALLEGRO_KEY_F1:         return kt_f1;
	case ALLEGRO_KEY_BACKSPACE:  return kt_backspace;
	case ALLEGRO_KEY_ESCAPE:     return kt_escape;
	case ALLEGRO_KEY_PGUP:       return kt_page_up;
	case ALLEGRO_KEY_PGDN:       return kt_page_down;
      }
      
      return ev.keyboard.unichar;
    }
    else if (ev.type == ALLEGRO_EVENT_JOYSTICK_BUTTON_DOWN)
    {
      /* fprintf(stderr, "Button: %d\n", ev.joystick.button); */

      if (options.diagonals == DIAGONALS_STEP && ev.joystick.button == 0)
      {
	goto check_fake_diagonals;
      }

      if (ev.joystick.button < 10)
	return kt_joy0 + ev.joystick.button;
    }
    else if (ev.type == ALLEGRO_EVENT_JOYSTICK_AXIS)
    {
      /*fprintf(stderr, "Axis: %d @ %f\n", ev.joystick.axis, ev.joystick.pos);*/

      if (ev.joystick.axis == 0)
      {
	if (ev.joystick.pos < -0.1)
	{
	  x_move = -5;
	  x_held = 1;
	}
	else if (ev.joystick.pos > 0.1)
	{
	  x_move = 5;
	  x_held = 1;
	}
	else
	{
	  x_held = 0;

	  if (options.diagonals == DIAGONALS_STEP)
	    x_move = 0;
	}

	joy_press = 1;
      }
      else if (ev.joystick.axis == 1)
      {
	if (ev.joystick.pos < -0.1)
	{
	  y_move = -5;
	  y_held = 1;
	}
	else if (ev.joystick.pos > 0.1)
	{
	  y_move = 5;
	  y_held = 1;
	}
	else
	{
	  y_held = 0;

	  if (options.diagonals == DIAGONALS_STEP)
	    y_move = 0;
	}

	joy_press = 1;
      }

/*      if (options.fake_diagonals == DIAGONALS_SLOPPY)
      {
	if (!y_held && !x_held)
	{
	  x_move = 0;
	  y_move = 0;
	  }
	  }*/
      if (/* options.diagonals == DIAGONALS_SLOPPY &&*/
	  joy_press && !y_held && !x_held)
      {
	goto check_fake_diagonals;
      }
    }
    else if (ev.type == ALLEGRO_EVENT_DISPLAY_RESIZE)
    {
//      al_stop_timer(timer);
      
      if (ev.display.width < SCREEN_W || ev.display.height < SCREEN_H)
	al_resize_display(realdisp, SCREEN_W, SCREEN_H);
      else
	al_acknowledge_resize(realdisp);
      
      configure_scaler();

//      al_start_timer(timer);

//      render_terminal();
    }
    else if (ev.type == ALLEGRO_EVENT_DISPLAY_CLOSE)
    {
      shutdown_everything();
      exit(0);
    }

    continue;

  check_fake_diagonals:
    if (y_move < 0 && x_move < 0)
      ret = kt_joy_nw;
    else if (y_move < 0 && x_move > 0)
      ret = kt_joy_ne;
    else if (y_move > 0 && x_move < 0)
      ret = kt_joy_sw;
    else if (y_move > 0 && x_move > 0)
      ret = kt_joy_se;
    else if (y_move < 0)
      ret = kt_joy_n;
    else if (y_move > 0)
      ret = kt_joy_s;
    else if (x_move < 0)
      ret = kt_joy_w;
    else if (x_move > 0)
      ret = kt_joy_e;

    /*
      Clear the direction if we got here from a joystick axis
      event. This doesn't happen with step diagonals since it would
      then be a button event.
    */
    if (ev.type == ALLEGRO_EVENT_JOYSTICK_AXIS)
    {
      y_move = 0;
      x_move = 0;
    }

    return ret;
  }

  return 0;
} /* get_keypress */



/*
  Figures out what ratio to scale the display at.
*/
void configure_scaler()
{
  float ratio_x;
  float ratio_y;

  int new_w;
  int new_h;

  new_w = al_get_display_width(realdisp);
  new_h = al_get_display_height(realdisp);
    
  ratio_x = (float)new_w / SCREEN_W;
  ratio_y = (float)new_h / SCREEN_H;
    
  if (ratio_x < ratio_y)
    scaler_ratio = (int)ratio_x;
  else
    scaler_ratio = (int)ratio_y;

  scaler_x_offset = (new_w - (scaler_ratio * SCREEN_W)) / 2;
  scaler_y_offset = (new_h - (scaler_ratio * SCREEN_H)) / 2;
    
  al_set_target_bitmap(al_get_backbuffer(realdisp));
  al_clear_to_color(al_map_rgb(0, 0, 0));

  return;
} /* configure_scaler */



void game_over(const char * msg, const blean_t won)
{
  scr_erase();

  scr_addstr(msg);
  scr_addstr("\n(Press any key to exit)");

  scr_flush();

  get_keypress();

  return;
} /* game_over */




/*
  Stores in RECT the coordinates of GENT in the tileset bitmap.
*/
void gent_rect(const gent_t gent, rect_t * rect)
{
  int tile_y;
  int tile_x;

  tile_y = gent / 16;
  tile_x = gent % 16;

  rect->w = TILE_W;
  rect->h = TILE_H;

  rect->t = 160 + 1 + tile_y * (TILE_H + 1);
  rect->l = 1 + tile_x * (TILE_W + 1);

  return;
} /* gent_rect */



/*
  Prints an extended character C in the status window.
*/
void st_special(const unsigned int c)
{
  terminal[TERM_MAP(st_y, 40 + st_x)] = c;
  terminal_flags[TERM_MAP(st_y, 40 + st_x)] = TERM_EXTENDED;
  st_x++;

  if (st_x >= 40)
  {
    st_x = 0;
    st_y++;
  }

  return;
} /* st_special */


/*
  Clears the status area.
*/
void st_erase(void)
{
  for (st_y = 0; st_y < 24; st_y++)
  {
    for (st_x = 0; st_x < 40; st_x++)
    {
      terminal[TERM_MAP(st_y, 40 + st_x)] = ' ';
      terminal_flags[TERM_MAP(st_y, 40 + st_x)] = 0;
    }
  }

  st_y = 0;
  st_x = 0;

  return;
} /* st_erase */


/*
  Print C in the status window.
*/
void st_addch(const char c)
{
  char temp[2];

  /* Wrap it in a string and pass it on. */
  temp[0] = c;
  temp[1] = '\0';

  st_addstr(temp);

  return;
} /* st_addch */



/*
  Prints S in the status window.
*/
void st_addstr(const char * s)
{
  unsigned int i;
  
  i = 0;

  while (s[i] != '\0')
  {
    if (s[i] == '\n')
    {
      st_x = 0;
      st_y++;
      i++;
      continue;
    }

    terminal[TERM_MAP(st_y, 40 + st_x)] = s[i];

    if (st_reverse)
      terminal_flags[TERM_MAP(st_y, 40 + st_x)] = TERM_REVERSE;
    else if (st_d)
      terminal_flags[TERM_MAP(st_y, 40 + st_x)] = TERM_DIM;
    else
      terminal_flags[TERM_MAP(st_y, 40 + st_x)] = 0;

    st_x++;

    if (st_x >= 40)
    {
      st_x = 0;
      st_y++;
    }

    i++;
  }

  return;
} /* st_addstr */



/*
  Moves the status window pointer to Y, X.
*/
void st_move(const unsigned int y, const unsigned int x)
{
  st_y = y;
  st_x = x;

  return;
} /* st_move */



/*
  Sets the reverse flag for the status window to NEW_STATE.
*/
void st_rev(const blean_t new_state)
{
  st_reverse = new_state;

  return;
} /* st_rev */



/*
  Sets the dim flag for the status window to NEW_STATE.
*/
void st_dim(const blean_t new_state)
{
  st_d = new_state;

  return;
} /* st_dim */



/*
  Stores at Y and X the current coordinates of the status window cursor.
*/
void st_getyx(unsigned int * y, unsigned int * x)
{
  *y = st_y;
  *x = st_x;

  return;
}


/*
  Not used for GUI.
*/
void st_flush(void)
{
  return;
} /* st_flush */



/*
  Puts GENT in the status window.
*/
void st_gent(const gent_t gent)
{
  terminal_flags[TERM_MAP(st_y, (40 + st_x))] = TERM_GENT;
  terminal[TERM_MAP(st_y, (40 + st_x))] = gent;
  terminal[TERM_MAP(st_y, (40 + st_x + 1))] = -1;

  st_x += 2;

  return;
} /* st_gent */



/*
  Clears the map window.
*/
void map_erase(void)
{
  int y;
  int x;

  if (options.glyph_mode)
  {
    for (y = 0; y < board_size_y; y++)
      for (x = 0; x < board_size_x; x++)
	terminal[TERM_MAP(y, x)] = ' ';
  }
  else
  {
    for (y = 0; y < board_size_y; y++)
    {
      for (x = 0; x < board_size_x; x++)
      {
	/* Make odd rows padding. */
	terminal[TERM_MAP(y, (x * 2))] = gent_blank;
	terminal_flags[TERM_MAP(y, (x * 2))] = TERM_GENT;
	terminal[TERM_MAP(y, (x * 2 + 1))] = -1;
      }
    }
  }

  return;
} /* map_erase */



/*
  Not used for GUI.
*/
void map_flush(void)
{
  return;
} /* map_flush */



/*
  Sets the map window cursor to Y, X.
 */
void map_move(const unsigned int y, const unsigned int x)
{
  map_y = y;
  map_x = x;

  return;
} /* map_move */



/*
  Sets the visibility of the map window cursor to NEW_STATE.
*/
void map_cursor(const blean_t new_state)
{
  map_c = new_state;

  return;
} /* map_cursor */



/*
  Puts GENT with attribute(s) ATTR at Y, X in the map window. Y and X
  are relative to their position on the board and will be spaced
  accordingly (see board_size_y/board_size_x).
*/
void map_put(const unsigned int y, const unsigned int x, const gent_t gent, const unsigned char attr)
{
  gent_t new_gent;
  int glyph;
  int g_attr;

  if (options.glyph_mode)
  {
    glyph = glyph_map[gent];
    
    remap_glyph(gent, &glyph, &g_attr);

    terminal[TERM_MAP(y, x)] = glyph;
    terminal_flags[TERM_MAP(y, x)] = g_attr;

    if (attr & MAP_DIM || attr & MAP_SLEEP)
      terminal_flags[TERM_MAP(y, x)] |= TERM_DIM;

    return;
  }

  if (gent == gent_floor)
    new_gent = gent + game->player->location->floor_type;
  else
    new_gent = gent;

  terminal[TERM_MAP(y, (x * 2))] = new_gent;
  terminal_flags[TERM_MAP(y, (x * 2))] = TERM_GENT | TERM_FLOOR;

  terminal[TERM_MAP(y, (x * 2 + 1))] = -1;

  if (attr & MAP_DIM || attr & MAP_SLEEP)
    terminal_flags[TERM_MAP(y, (x * 2))] |= TERM_DIM;

  return;
} /* map_put */



/*
  Clears the message bar.
*/
void mb_erase()
{
  for (mb_y = 20; mb_y < 24; mb_y++)
  {
    for (mb_x = 0; mb_x < 40; mb_x++)
    {
      terminal[TERM_MAP(mb_y, mb_x)] = ' ';
      terminal_flags[TERM_MAP(mb_y, mb_x)] = 0;
    }
  }

  mb_y = 0;
  mb_x = 0;

  return;
} /* mb_erase */



/*
  Not used for GUI.
*/
void mb_flush(void)
{
  return;
} /* mb_flush */



/*
  Sets the cursor to Y, X for the message bar.
*/
void mb_move(const unsigned int y, const unsigned int x)
{
  mb_y = y;
  mb_x = x;
  return;
} /* mb_move */



/*
  Print S on the message bar.
*/
void mb_addstr(const char * s)
{
  unsigned int i;
  
  i = 0;

  while (s[i] != '\0')
  {
    terminal[TERM_MAP((mb_y + 20), mb_x)] = s[i];
    mb_x++;
    i++;
  }

  return;
}



/*
  Prints a MORE marker on the message bar.
*/
void mb_more(void)
{
  mb_move(3, 34);
  mb_addstr("[MORE]");

  return;
} /* mb_more */



/*
  Sets the visibility of the message bar cursor to NEW_STATE.
*/
void mb_cursor(const blean_t new_state)
{
  mb_c = new_state;
  
  return;
} /* mb_cursor */



/*
  Clears the whole terminal.
*/
void scr_erase(void)
{
  int i;

  for (i = 0; i < TERM_H * TERM_W; i++)
  {
    terminal[i] = ' ';
    terminal_flags[i] = 0;
  }

  scr_y = 0;
  scr_x = 0;

  return;
} /* scr_erase */



/*
  Prints an extended character C in the global window.
*/
void scr_special(const unsigned int c)
{
  terminal[TERM_MAP(scr_y, scr_x)] = c;
  terminal_flags[TERM_MAP(scr_y, scr_x)] = TERM_EXTENDED;
  scr_x++;

  if (scr_x >= 80)
  {
    scr_x = 0;
    scr_y++;
  }

  return;
} /* scr_special */



/*
  Stores the current screen cursor at Y, X.
*/
void scr_getyx(unsigned int * y, unsigned int * x)
{
  *y = scr_y;
  *x = scr_x;
  
  return;
} /* scr_getyx */



/*
  Sets the screen cursor coordinates to Y, X.
*/
void scr_move(const unsigned int y, const unsigned int x)
{
  scr_y = y;
  scr_x = x;
  
  return;
} /* scr_move */



/*
  Puts a GENT with attribute(s) ATTR on the screen.
*/
void scr_addgent(const gent_t gent, const unsigned int attr)
{
  int glyph;
  int g_attr;

  if (options.glyph_mode)
  {
    glyph = glyph_map[gent];
    
    remap_glyph(gent, &glyph, &g_attr);

    terminal[TERM_MAP(scr_y, scr_x)] = glyph;
    terminal_flags[TERM_MAP(scr_y, scr_x)] = g_attr;

    /*if (g_attr & MAP_DIM || g_attr & MAP_SLEEP)
      terminal_flags[TERM_MAP(scr_y, scr_x)] |= TERM_DIM;*/
  }
  else
  {
    /* Remap all floors to the level floor type */
    if (gent == gent_floor)
      terminal[TERM_MAP(scr_y, scr_x)] = gent + game->player->location->floor_type;
    else
      terminal[TERM_MAP(scr_y, scr_x)] = gent;
    
    terminal[TERM_MAP(scr_y,  scr_x + 1)] = -1;
    
    terminal_flags[TERM_MAP(scr_y, scr_x)] = TERM_GENT;

    if (attr & MAP_FLOOR)
      terminal_flags[TERM_MAP(scr_y, scr_x)] |= TERM_FLOOR;
  }

  if (attr & MAP_DIM)
    terminal_flags[TERM_MAP(scr_y, scr_x)] |= TERM_DIM;

  if (options.glyph_mode)
    scr_x++;
  else
    scr_x += 2;

  if (scr_x >= TERM_W)
  {
    scr_x = 0;
    scr_y++;
  }

  return;
} /* scr_addgent */



/*
  Prints C on the screen.
*/
void scr_addch(const char c)
{
  char temp[2];

  /* Wrap it in a string and pass it on. */
  temp[0] = c;
  temp[1] = '\0';

  scr_addstr(temp);

  return;
} /* scr_addch */



/*
  Prints S on the screen.
*/
void scr_addstr(const char * s)
{
  unsigned int i;

  i = 0;

  while (s[i] != '\0')
  {
    if (s[i] == '\n')
    {
      scr_x = 0;
      scr_y++;
      i++;
      continue;
    }

    terminal[TERM_MAP(scr_y, scr_x)] = s[i];
    terminal_flags[TERM_MAP(scr_y, scr_x)] = 0;

    if (scr_reverse)
    {
      terminal_flags[TERM_MAP(scr_y, scr_x)] |= TERM_REVERSE;
    }

    scr_x++;

    if (scr_x >= TERM_W)
    {
      scr_x = 0;
      scr_y++;
    }

    i++;
  }

  return;
} /* scr_addstr */



/*
  Not used for GUI.
*/
void scr_flush(void)
{
  return;
} /* scr_flush */



/*
  Sets the reverse flag for the global screen to NEW_STATE.
*/
void scr_rev(const blean_t new_state)
{
  scr_reverse = new_state;
  return;
} /* scr_rev */



void render_terminal(void)
{
  int y;
  int x;
  int mode;
  int raw;
  rect_t source;
  rect_t dest;
  int flags;
  anim_t * anim;
  ALLEGRO_BITMAP * bmp;

//  al_set_target_bitmap(al_get_backbuffer(realdisp));
  al_set_target_bitmap(disp);

  for (y = 0; y < 24; y++)
  {
    for (x = 0; x < 80; x++)
    {
      flags = 0;

/*      if (rand() % 4 == 0)
	flags = ALLEGRO_FLIP_VERTICAL;

      if (rand() % 4 == 0)
      flags = ALLEGRO_FLIP_HORIZONTAL;*/

      mode = terminal_flags[y * 80 + x];
      raw = terminal[y * 80 + x];

      if (raw < 0)
	continue;

      dest.t = y * FONT_H;
      dest.l = x * FONT_W;

      if (mode & TERM_GENT)
      {
	if (mode & TERM_FLOOR)
	{
	  gent_rect(gent_floor + game->player->location->floor_type, &source);

	  if (mode & TERM_DIM)
	    bmp = tileset_dim;
	  else
	    bmp = tileset;

	  al_draw_bitmap_region(bmp, source.l, source.t, source.w, source.h, dest.l, dest.t, flags);

	  gent_rect(raw, &source);
	}
	else
	{
	  gent_rect(gent_blank, &source);

	  if (mode & TERM_DIM)
	    bmp = tileset_dim;
	  else
	    bmp = tileset;

	  al_draw_bitmap_region(bmp, source.l, source.t, source.w, source.h, dest.l, dest.t, flags);
	  gent_rect(raw, &source);
	}
      }
      else
      {
	if (mode & TERM_EXTENDED)
	  raw += 128;
	
	source.w = FONT_W;
	source.h = FONT_H;
	source.t = (raw / 32) * FONT_H;
	source.l = (raw % 32) * FONT_W;

	if (mode & TERM_REVERSE)
	  bmp = mainfont_rev;
	else if (mode & TERM_DIM)
	  bmp = mainfont_dim;
	else
	  bmp = mainfont;
      }
	
/*      if (mode & TERM_REVERSE)
	al_draw_bitmap_region(tileset_rev, source.l, source.t, source.w, source.h, dest.l, dest.t, flags);
      else if (mode & TERM_DIM)
	al_draw_bitmap_region(tileset_dim, source.l, source.t, source.w, source.h, dest.l, dest.t, flags);
	else*/

      al_draw_bitmap_region(bmp, source.l, source.t, source.w, source.h, dest.l, dest.t, flags);
    }
  }

  if (map_c)
  {
    if (options.glyph_mode)
    {
      source.w = FONT_W;
      source.h = FONT_H;
      source.l = FONT_W * 3;
      source.t = 0;
      dest.l = map_x * FONT_W;
      dest.t = map_y * FONT_H;

      bmp = tileset;
    }
    else
    {
      source.w = TILE_W + 6;
      source.h = TILE_H + 6;
      source.l = 1;
      source.t = 134;
      dest.l = map_x * TILE_W - 3;
      dest.t = map_y * TILE_H - 3;

      bmp = mainfont;
    }

    al_draw_bitmap_region(tileset, source.l, source.t, source.w, source.h, dest.l, dest.t, 0);
  }

  if (mb_c /*&& mb_t > MB_CURSOR_BLINK*/)
  {
    source.w = FONT_W;
    source.h = FONT_H;
    source.t = 0;
    source.l = FONT_W * 3;

    dest.t = (mb_y + 20) * FONT_H;
    dest.l = (mb_x) * FONT_W;

    al_draw_bitmap_region(mainfont, source.l, source.t, source.w, source.h, dest.l, dest.t, 0);
  }

  if (term_sel.active)
  {
    source.w = FONT_W;
    source.h = FONT_H;
    source.t = 0;
    source.l = FONT_W;
    
    for (y = term_sel.t; y <= term_sel.b ; y++)
    {
      for (x = term_sel.l; x <= term_sel.r; x++)
      {
	dest.t = y * FONT_H;
	dest.l = x * FONT_W;
	al_draw_tinted_bitmap_region(mainfont, al_map_rgba_f(0.4, 0.4, 0.4, 0.4),
				     source.l, source.t, source.w, source.h, dest.l, dest.t, 0);
      }
    }
  }

  char line[10];
  int real_y;
  int real_x;

  for (anim = first_anim; anim != NULL; anim = anim->next)
  {
    switch (anim->type)
    {
      case anim_type_attention:
      case anim_type_alert:
	if (anim_coords(anim, &real_y, &real_x) == false)
	  continue;

	real_y -= view_top;
	real_x -= view_left;

	if (real_y < 0 || real_y >= board_size_y ||
	    real_x < 0 || real_x >= board_size_x)
	  continue;

	source.w = 32;
	source.h = 32;
	
	source.l = 64;
	source.t = 128;
	
	dest.l = real_x * TILE_W - 6 - 1 + rand() % 3;
	dest.t = real_y * TILE_H - 6 - 1 + rand() % 3;
	
	al_draw_bitmap_region(tileset, source.l, source.t, source.w, source.h, dest.l, dest.t, 0);

	break;

      case anim_type_damage:
	if (anim_coords(anim, &real_y, &real_x) == false)
	  continue;

	real_y -= view_top;
	real_x -= view_left;

	if (real_y < 0 || real_y >= board_size_y ||
	    real_x < 0 || real_x >= board_size_x)
	  continue;

	dest.l = real_x * TILE_W + 10;
	dest.t = real_y * TILE_H + 5 - anim->param[ANIM_POPUP_OFFSET];
	
	sprintf(line, "%d", anim->param[ANIM_POPUP_VALUE]);
	al_draw_text(small_font, al_map_rgb(255, 255, 255), dest.l, dest.t, ALLEGRO_ALIGN_CENTRE, line);
	break;

      case anim_type_deathspell:
	if (anim_coords(anim, &real_y, &real_x) == false)
	  continue;

	real_y -= view_top;
	real_x -= view_left;

	if (real_y < 0 || real_y >= board_size_y ||
	    real_x < 0 || real_x >= board_size_x)
	  continue;

	source.w = TILE_W;
	source.h = TILE_H;
	
	source.l = 163;
	source.t = 128;

	dest.l = real_x * TILE_W - 1 + rand() % 3;
	dest.t = real_y * TILE_H - 1 + rand() % 3;
	
	al_draw_bitmap_region(tileset, source.l, source.t, source.w, source.h, dest.l, dest.t, 0);
	break;

      default:
	break;
    }
  }

  al_set_target_bitmap(al_get_backbuffer(realdisp));

  al_draw_scaled_bitmap(disp,
			0, 0,
			SCREEN_W, SCREEN_H,
			scaler_x_offset, scaler_y_offset,
			SCREEN_W * scaler_ratio, SCREEN_H * scaler_ratio, 0);

  al_flip_display();


  return;
}



void remap_glyph(const gent_t gent, int * glyph, int * attr)
{
  *attr = 0;
  
  switch (gent)
  {
    case gent_wall_v:
      *attr = TERM_EXTENDED | TERM_REVERSE;
      *glyph = ST_VLINE;
      break;

    case gent_wall_h:
      *attr = TERM_EXTENDED | TERM_REVERSE;
      *glyph = ST_HLINE;
      break;

    case gent_wall_es:
      *attr = TERM_EXTENDED | TERM_REVERSE;
      *glyph = ST_SE;
      break;

    case gent_wall_sw:
      *attr = TERM_EXTENDED | TERM_REVERSE;
      *glyph = ST_SW;
      break;

    case gent_wall_ne:
      *attr = TERM_EXTENDED | TERM_REVERSE;
      *glyph = ST_NE;
      break;

    case gent_wall_nw:
      *attr = TERM_EXTENDED | TERM_REVERSE;
      *glyph = ST_NW;
      break;

    case gent_wall_nes:
      *attr = TERM_EXTENDED | TERM_REVERSE;
      *glyph = ST_LTEE;
      break;

    case gent_wall_nsw:
      *attr = TERM_EXTENDED | TERM_REVERSE;
      *glyph = ST_RTEE;
      break;

    case gent_wall_new:
      *attr = TERM_EXTENDED | TERM_REVERSE;
      *glyph = ST_BTEE;
      break;

    case gent_wall_esw:
      *attr = TERM_EXTENDED | TERM_REVERSE;
      *glyph = ST_TTEE;
      break;

    case gent_wall_cross:
      *attr = TERM_EXTENDED | TERM_REVERSE;
      *glyph = ST_CROSS;
      break;

    case gent_floor:
      *attr = TERM_EXTENDED;
      *glyph = ST_FLOOR;
      break;

    case gent_door_closed:
    case gent_terminal:
    case gent_capsule:
      *attr = TERM_REVERSE;
      *glyph = glyph_map[gent];
      break;

    default:
      *glyph = glyph_map[gent];
      break;
  }

  return;
}
