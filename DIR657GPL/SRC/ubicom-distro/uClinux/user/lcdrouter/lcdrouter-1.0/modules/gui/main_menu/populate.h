#ifndef POPULATE_H
#define POPULATE_H
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>

//#include <lite/lite.h>
//#include <lite/box.h>
//#include <lite/window.h>
//#include <lite/util.h>

#include <string.h>
#include <gui/mm/menu_data.h>

struct menu_data_instance* create_and_populate_menu_data();

#endif
