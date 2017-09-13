#ifndef __LABEL_H__
#define __LABEL_H__
#include <leck/label.h>

LiteLabel *label_create(
	LiteWindow *win,
	DFBRectangle rect,
	LiteLabelTheme *theme,
	const char *string,
	int font_size,
	DFBColor color);

extern DFBResult set_label_color(LiteLabel *label, DFBColor color);
extern DFBResult set_label_text(LiteLabel *label, const char *text);

#endif
