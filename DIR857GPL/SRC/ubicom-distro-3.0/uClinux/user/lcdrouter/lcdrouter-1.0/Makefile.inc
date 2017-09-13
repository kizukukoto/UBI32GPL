PROJECTNAME = lcdrouter
HEADER_WIDGET = 1
STATUSBAR_WIDGET = 1
MENUV2_WIDGET = 1
GRID_WIDGET = 1
LISTBOX_WIDGET = 1
SCROLL_WIDGET = 1
KEYBOARD_WIDGET = 1
KEYBOARD_WINDOW_WIDGET = 1
CALIBRATION_WINDOW_WIDGET = 1

ifeq ($(CC),cc)
GUI_CFLAGS = -DMODULES_PATH=\"../modules\" -DLITEFONTDIR=\"/share/fonts/truetype\" -D_REENTRANT -DPC -I/usr/local/include -I/usr/local/include/directfb
GUI_LDFLAGS = -L/usr/local/lib -llite -lleck -ldirectfb -lfusion -ldirect -lpthread -ldl -lm
else
GUI_CFLAGS = $(CFLAGS)  -DMODULES_PATH=\"/usr/lib/lcdrouter\" -DLITEFONTDIR=\"/share/fonts/truetype\" -D_REENTRANT
GUI_LDFLAGS = $(LDFLAGS) -llite -lleck -ldirectfb -lfusion -ldirect -lpthread -ldl -lm
endif
