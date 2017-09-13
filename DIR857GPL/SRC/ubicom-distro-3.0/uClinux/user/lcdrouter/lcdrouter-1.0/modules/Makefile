include ../Makefile.inc

DESKTOP_FILES = gui/desktop/desktop.c

MAIN_MENU_FILES = gui/main_menu/main_menu.c gui/main_menu/populate.c status/populate_status.c $(CALIBRATION_FILES)

TEMPLATE_FILES = gui/template/template.c 

CALIBRATION_FILES = status/calibration.c status/calibrate.c

DUMMY_RETURN_FILES = dummy_return/dummy_return.c

NETWORK_MAP_FILES = network_map/network_map.c

ifeq ($(CC),cc)
NETWORK_STATUS_FILES = status/network_status.c 
else
NETWORK_STATUS_FILES = status/network_status.c status/network_utils.c
endif

ifeq ($(CC),cc)
GUI_CFLAGS += -I../include
else
GUI_CFLAGS += -I../include -I$(ROOTDIR)/user/nvram -I$(ROOTDIR)/user/libplatform
endif

ifeq ($(CC),cc)
GUI_LDFLAGS += -L/lib
else
GUI_LDFLAGS += -L/lib -L$(ROOTDIR)/user/nvram -lnvram -L$(ROOTDIR)/user/libplatform -lvct
endif

all: main_menu.so desktop.so network_status.so dummy_return.so network_map.so

desktop.so:
	$(CC) $(GUI_CFLAGS) -o $@ $(DESKTOP_FILES) $(GUI_LDFLAGS) ../src/liblcdrouter.so -shared

main_menu.so:
	$(CC) $(GUI_CFLAGS) -o $@ $(MAIN_MENU_FILES) $(GUI_LDFLAGS) ../src/liblcdrouter.so -shared

template.so:
	$(CC) $(GUI_CFLAGS) -o $@ $(TEMPLATE_FILES) $(GUI_LDFLAGS) ../src/liblcdrouter.so -shared

network_status.so:
	$(CC) $(GUI_CFLAGS) -o $@ $(NETWORK_STATUS_FILES) $(GUI_LDFLAGS) ../src/liblcdrouter.so -shared

dummy_return.so:
	$(CC) $(GUI_CFLAGS) -o $@ $(DUMMY_RETURN_FILES) $(GUI_LDFLAGS) ../src/liblcdrouter.so -shared

network_map.so:
	$(CC) $(GUI_CFLAGS) -o $@ $(NETWORK_MAP_FILES) $(GUI_LDFLAGS) ../src/liblcdrouter.so -shared

clean:
	rm -f *.so

install:
	test -z "../build/lib/lcdrouter" || mkdir -p ../build/lib/lcdrouter
	cp *.so ../build/lib/lcdrouter
