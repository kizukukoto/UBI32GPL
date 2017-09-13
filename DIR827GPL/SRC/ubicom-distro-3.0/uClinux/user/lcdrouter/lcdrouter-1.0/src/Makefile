include ../Makefile.inc

FILES = gui/engine/animation.c \
	gui/engine/app_loader.c \
	gui/mm/menu_data.c \
	debug.c \

GUI_CFLAGS += -I../include

ifdef HEADER_WIDGET
FILES += gui/widgets/header.c 
endif

ifdef MENUV2_WIDGET
FILES += gui/widgets/menuv2.c 
endif

ifdef STATUSBAR_WIDGET
FILES += gui/widgets/statusbar.c 
endif   

ifdef GRID_WIDGET
FILES += gui/widgets/grid.c 
endif

ifdef LISTBOX_WIDGET
FILES += gui/widgets/listbox.c
endif

ifdef SCROLL_WIDGET
FILES += gui/widgets/scroll.c 
endif

ifdef KEYBOARD_WIDGET
FILES += gui/widgets/keyboard.c
endif

ifdef KEYBOARD_WINDOW_WIDGET
FILES += gui/widgets/keyboard_window.c
endif

ifdef CALIBRATION_WINDOW_WIDGET
FILES += gui/widgets/calibration_window.c
endif

all: liblcdrouter.so

liblcdrouter.so:
	$(CC) $(GUI_CFLAGS) -o $@ $(FILES) $(GUI_LDFLAGS) -shared

install:
	test -z "../build/lib" || mkdir -p ../build/lib
	cp liblcdrouter.so ../build/lib/
clean:
	rm -f *.so
