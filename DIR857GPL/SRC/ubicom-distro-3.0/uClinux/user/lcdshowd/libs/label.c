#include <directfb.h>
#include <lite/window.h>
#include <leck/label.h>

DFBResult set_label_color(LiteLabel *label, DFBColor color)
{
	return lite_set_label_color(label, &color);
}

DFBResult set_label_text(LiteLabel *label, const char *text)
{
	return lite_set_label_text(label, text);
}

LiteLabel *label_create(
	LiteWindow *win,
	DFBRectangle rect,
	LiteLabelTheme *theme,
	const char *string,
	int font_size,
	DFBColor color)
{
	DFBResult res;
	LiteLabel *label;

	if ((res = lite_new_label(LITE_BOX(win), &rect, theme, font_size, &label)) != DFB_OK)
		return NULL;

	set_label_text(label, string);
	set_label_color(label, color);
	return label;
}
