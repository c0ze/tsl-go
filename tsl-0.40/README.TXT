The Slimy Lichmummy Manual 0.40
(it's the Lichmummy that's slimy, not the manual)

by Ulf Åström
happyponyland.net
ulf.astrom@gmail.com


Introduction
============
TSL is a "rogue-like" adventure game. It is turn-based, keyboard
controlled and uses ASCII characters to represent the game world.


Building and installing
=======================
Building TSL should work something like this, assuming you are using a
UNIX(-like) environment with GCC and ncurses/SDL:

gzip -d tsl-version.tar.gz
tar xvpf tsl-version.tar
cd tsl-version

Now you can now either 

./build_console.sh
 - to link with curses and play in a terminal, or

./build_gui.sh
 - to link with Allegro and play in a graphical mode.

The game binary is called "tsl".


Quick-start Tutorial
====================
Start the game. Bring up the in-game help system by pressing ? and it
should tell you all you need to now.

Note: That's a literal question mark. If your keyboard layout requires
you to hold shift to type a ?, you must also do so within TSL. Also
note that TSL distinguishes between upper-and lowercase. For example,
'A' will not invoke the same command as 'a'.

TSL is designed for "vi-keys" movement with the hjkl keys. If you are
unwilling or unable to use them for any reason, it has built-in Dvorak
support and even allows full key remapping (see "Customizing").

If you have problems playing with the numpad, it is recommended you
turn NumLock off.


Customizing
===========
There are several directives you can use to set your personal
preferences and customize your TSL experience. Create a file called
tsl_conf (in the same directory as the game binary) or .tsl_conf (in
your home directory, UNIX only). Enter any of these options, on
separate lines:

* name NAME
  Sets the default player character name to NAME. No spaces are
  allowed. If no name directive is present, the username of the
  current user will be used.

* nomorgue
  Prevents the game from dumping information about your character
  (morgue.txt in the current working directory) at the end of the
  game.

* forcegetname
  If enabled, the user must always enter a name for the player
  character. Overrides name option. This can also be set when the game
  is started from the shell with the --force-get-name option.

* dvorak
  Loads a built-in Dvorak keymap. If you want additional customization
  (bind and bindn) these must be added _after_ the dvorak command.

* fullscreen
  (only applies to graphical version)
  Tries to use the whole screen.

* noautoequip
  Disables autoequipping items when picked up.

* autocenter
  Automatically recenters view around player each step.

* dotfloors
  (only applies to terminal)
  Displays floors as periods (.) instead of checkerboards.

* largecursor
  (only applies to terminal version)
  Changes the cursor used in-game (for selecting locations, etc) to
  "very visible" instead of just "visible". This doesn't work on all
  terminals (e.g. PuTTY), so only enable it if you are certain your
  system can handle it properly.

* bind ACTION KEY
  bindn ACTION NUM
  Binds a key to an action. bind binds a character literal, while
  bindn binds a numerical value. Example: bind quit Q - binds
  uppercase Q to action quit bindn cancel 32 - binds ASCII code 32
  (space, since we cannot enter a literal space character) to action
  cancel. Note that you can't bind multiple keys to the same action,
  the last key bound will take precedence.
  Example: "bindn cancel 27" will let you use Escape to leave menus.

The End.
