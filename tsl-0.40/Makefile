COMMON_CFLAGS = -Wall
COMMON_SRC=$(wildcard common/*.c)

CONSOLE_LIBS   = -lm -lcurses
CONSOLE_CFLAGS = -DTSL_CONSOLE
# Should be equivalent to your list of C files, if you don't build selectively
CONSOLE_SRC=$(wildcard console/*.c)

all: clean tsl_console tsl_gui

clean:
	-rm tsl_gui tsl_console

tsl_console: $(COMMON_SRC) $(CONSOLE_SRC)
	gcc -o $@ $^ $(COMMON_CFLAGS) $(CONSOLE_CFLAGS) $(CONSOLE_LIBS)

GUI_LIBS   = -lm -lallegro -lallegro_image -lallegro_font
GUI_CFLAGS = -DTSL_GUI
GUI_SRC=$(wildcard gui/*.c)

tsl_gui: $(COMMON_SRC) $(CONSOLE_INC) $(GUI_SRC)
	gcc -o $@ $^ $(COMMON_CFLAGS) $(GUI_CFLAGS) $(GUI_LIBS)
