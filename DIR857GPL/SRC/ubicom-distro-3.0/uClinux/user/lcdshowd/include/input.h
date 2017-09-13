#include <leck/label.h>
#include <leck/textline.h>
#include "global.h"

#define EDIT_ENABLE	0
#define EDIT_DISABLE	1

typedef struct _key_map {
	DFBPoint p1;
	DFBPoint p2;
	const char *context;
	int (*func)(const char *, LiteTextLine *);
} key_map;

typedef struct _key_combination {
	const char *png;
	key_map *registed_map;
} key_combination;

extern void keyboard_input(const char *lb_title, LiteTextLine *orig_text);
extern void select_input(char *options[], const char *, LiteTextLine *, int editable);
