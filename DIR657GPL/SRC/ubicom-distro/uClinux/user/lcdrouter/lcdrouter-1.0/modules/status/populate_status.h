#ifndef POPULATE_STATUS_H
#define POPULATE_STATUS_H
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
#include <calibration.h>

struct menu_data_instance* populate_and_create_status();

#endif
