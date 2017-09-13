#include "mouse.h"
#include "wizard.h"

int option_return_click(LiteWindow *win, int opt)
{
	int x, y, opt_idx, ret = PAGE_UNDEFINE;
	struct {
		int opt[5];
		DFBRegion btr[5];
	} btrs = {
		{ (opt & PAGE_PREV)? PAGE_PREV:
		  (opt & PAGE_BACK)? PAGE_BACK: PAGE_NOTHANKS,
		  PAGE_CANCLE,
		  (opt & PAGE_NEXT)? PAGE_NEXT: PAGE_BEGIN,
		  PAGE_YES,
		  PAGE_NO },
		{ {  36, 192, 112, 218 },	/* Prev, Back, No Thanks */
		  { 122, 190, 206, 218 },	/* Cancle */
		  { 216, 190, 292, 218 },	/* Next, Begin */
		  {  30,  80, 163, 163 },	/* Yes */
		  { 170,  80, 303, 163 }}	/* No */
	};

	while (true) {
		DFBWindowEventType type = mouse_handler(win, &x, &y, 0);

		if (type == DWET_BUTTONUP) {
			opt_idx = bt_ripple_handler(x, y, btrs.btr, 5);

			if (opt_idx == -1)
				continue;
			if (opt & btrs.opt[opt_idx])
				ret = btrs.opt[opt_idx];
			if (ret != PAGE_UNDEFINE)
				break;
		}
	}

	return ret;
}
