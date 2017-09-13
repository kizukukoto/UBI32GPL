/*
 * Advanced Mouse Handler
 *
 * Copyright c 2009 Ubicom Inc. <www.ubicom.com>.  All rights reserved.
 *
 * This file contains confidential information of Ubicom, Inc. and your use of
 * this file is subject to the Ubicom Software License Agreement distributed with
 * this file. If you are uncertain whether you are an authorized user or to report
 * any unauthorized use, please contact Ubicom, Inc. at +1-408-789-2200.
 * Unauthorized reproduction or distribution of this file is subject to civil and
 * criminal penalties.
 */

#include <stdlib.h>
#include <limits.h>
#include <string.h>
#include <directfb.h>

#define DEBUG 1
#define TRACE 1

#if DEBUG
#define DEBUG_PRINTF(...) printf(__VA_ARGS__)
#else
#define DEBUG_PRINTF(...)
#endif

#if TRACE
#define DEBUG_TRACE(...) DEBUG_PRINTF(__VA_ARGS__)
#else
#define DEBUG_TRACE(...)
#endif

#define SCR_X 320
#define SCR_Y 240

#define NDS_LEN	 32 //Normal DiStribution table LENgth 
#define HST_LEN	 NDS_LEN //HiSTory LENgth 
#define QUE_LEN	(HST_LEN/2) //QUEue LENgth
#define VAR_RAD	(SCR_X/8/2) //VARiance RADius (finger width/2)
#define VAR_SQR	(VAR_RAD*VAR_RAD) //VARiance SQuaRe
#define GST_PER	 35 //Gesture Percentage
#define RIP_RAD  VAR_RAD //RIPple RADius
#define SAM_MIN  3 //SAMple MINimum
#define MAR_PER  2.5 //MARgin PERcentage

#define MAR_X (SCR_X*(MAR_PER*10/1000))
#define MAR_Y (SCR_Y*(MAR_PER*10/1000))

/*
 * Normal Distribution Table
 * http://www.math.csusb.edu/faculty/stanton/probstat/normal_distribution.html
 *
 *	0.0743657, 0.0718041, 0.0669425, 0.0602603,  0.0523766,  0.0439562,  0.0356188,  0.0278685,
 *	0.0210536, 0.0153573, 0.0108163, 0.00735561, 0.00482987, 0.00306216, 0.00187455, 0.001108
 *
 *	for integer process and integer max value limit, multiply each item by 10000000/6 => 0.0743657*10000000/6=123943
 *	consider the case of max 32 samples and max coordinate value 1024: 123943*32*1024=4061364224 < ULONG_MAX
 */
static unsigned long NorDistTab[NDS_LEN] = {
	  1847,   3124,   5104,   8050,  12259,  18027,  25596,  35089,
	 46448,  59365,  73260,  87294, 100434, 111571, 119674, 123943,
	123943, 119674, 111571, 100434,  87294,  73260,  59365,  46448,
	 35089,  25596,  18027,  12259,   8050,   5104,   3124,   1847
};

static int HstC = 0, QueC = 0; //History Counter and Queue Counter
static DFBPoint Hst[HST_LEN]; //History
static DFBPoint Que[QUE_LEN]; //Queue
static unsigned long Var[HST_LEN]; //Variance
int MarEnabled = 1; //margin check flag

/*
 * Update history from queue
 */
static void qu_update(void)
{
	memcpy(&Hst[HST_LEN-QueC], &Que[0], sizeof(DFBPoint)*QueC);
	QueC = 0;
}

/*
 * Variance based on the reference point
 */
static void ms_variance(DFBPoint *pt)
{
	int i, x, y;

	for (i = 0; i < HstC; i++) {

		x = Hst[i].x - pt->x; //x offset
		y = Hst[i].y - pt->y; //y offset

		Var[i] = x * x + y * y; //distance square base
	}
}

/*
 * Average by normal distribution weighted 
 */
static void ms_average(DFBPoint *pt)
{
	int i, j, n, m;
	unsigned long x, y, w;

	j = 0; //distribution array index
	m = HstC - 1; //denominator
	n = m / 2; //init numerator of distribution array index for rounding
	x = y = w = 0;

	for (i = 0; i < HstC; i++) {

		if (Hst[i].x < 0) continue; //skip if jittered mark

		x += Hst[i].x * NorDistTab[j]; //weighted x
		y += Hst[i].y * NorDistTab[j]; //weighted y
		w += NorDistTab[j]; //weights

		/*
		 * Distribution Array Index = round((DST_LEN-1)*i/(HstC-1))
		 */
		n += (NDS_LEN - 1);
		while (n  >= m) {
			n -= m;
			j++;
		}
	}

	pt->x = x / w;
	pt->y = y / w;

	DEBUG_PRINTF("Average: s=%d x=%d y=%d\n", HstC, pt->x, pt->y);
}

/*
 * Find and mark jittered samples
 */
static int ms_dejitter(void)
{
	int i, c = 0;

	for (i = 0; i < HstC; i++) {

		if (Var[i] >= VAR_SQR) {

			DEBUG_PRINTF("Dejitter: %d,%d\n", Hst[i].x, Hst[i].y);
			Hst[i].x = -1; //jittered mark
			c++; //jittered counter
		}
	}

	HstC -= c; //reset number of valid samples

	return c;
}

/*
 * Convert all samples to a vector
 */
static void ms_vector(DFBPoint *pt)
{
	int i;

	pt->x = pt->y = 0;

	for (i = HstC/2; i < HstC; i++) {
		pt->x += Hst[i].x;
		pt->y += Hst[i].y;
	}

	for (i = 0; i < (HstC+1)/2; i++) {
		pt->x -= Hst[i].x;
		pt->y -= Hst[i].y;
	}
}

/*
 * Gesture determination by percentage of variance
 */
static DFBWindowEventType ms_gesture(DFBPoint *pt)
{
	int i, c = 0;

	for (i = 0; i < HstC; i++) {
		if (Var[i] >= VAR_SQR) c++; //variance counter
	}

	DEBUG_PRINTF("Variance: c=%d %%=%d\n", c, c*100/HstC);

	if (c > HstC * GST_PER / 100) { //determine motion if variance is greater than GST_PER%

		ms_vector(pt);
		return DWET_MOTION;
	}

	if (ms_dejitter()) {

		//avoid mis-behavior, necessary to check min samples again
		if (HstC < SAM_MIN) return DWET_NONE;

		ms_average(pt);
	}

	return DWET_BUTTONUP;
}

/*
 * Touch press and gesture motion handler
 */
void ms_advanced_handler(DFBWindowEvent *evt)
{
	DFBWindowEventType evtype = evt->type;
	evt->type = DWET_NONE; //assume return DWER_NONE

	if (evtype == DWET_BUTTONDOWN || evtype == DWET_MOTION) {

		DEBUG_TRACE("D %d %d\n", evt->cx, evt->cy);

		/*
		 * skip margin cases, most of them are jitters
		 */
		if (MarEnabled) {
			if (evt->cx < MAR_X || evt->cx >= SCR_X-MAR_X || evt->cy < MAR_Y || evt->cy >= SCR_Y-MAR_Y) {
				return;
			}
		} 

		if (HstC < HST_LEN) {

			Hst[HstC  ].x = evt->cx;
			Hst[HstC++].y = evt->cy;
		} else {

			Que[QueC  ].x = evt->cx;
			Que[QueC++].y = evt->cy;
			if (QueC >= QUE_LEN) qu_update();
		}

	} else if (evtype == DWET_BUTTONUP) {

		DEBUG_TRACE("U %d %d\n", evt->cx, evt->cy);

		/*
		 * Do not record pen-up point
		 */
		if (HstC >= HST_LEN && QueC != 0) qu_update();

		if (HstC >= SAM_MIN) {

			DFBPoint pt;

			ms_average(&pt);
			ms_variance(&pt);

			evt->type = ms_gesture(&pt);
			evt->cx   = pt.x;
			evt->cy   = pt.y;
		}

		HstC = 0;

	} else {

		DEBUG_TRACE("?\n");
	}
}

/*
 * Touch scroll tracking handler
 */
#if 0
DFBWindowEventType ms_scroll_handler(void)
{
}
#endif

/*
 * Button ripple region handler
 *
 *	Consider circle (x - cx)^2 + (y - cy)^2 = r^2, r = 0..RIP_RAD
 *	when r is getting longer and if the circle is overlaid with the button, it's a press.
 *
 *	    x1  x2
 *	  1 | 2 | 3
 *	----+---+---- y1
 *	  4 | 5 | 6       button area (5)=(x1,y1)-(x2,y2)
 *	----+---+---- y2
 *	  7 | 8 | 9
 *
 *	a valid press:
 *	(5)    y1 <= y <= y2 and x1 <= x <= x2
 *	(4)(6) y1 <= y <= y2 and (4) x <= x1 and x + r >= x1 (6) x >= x2 and x - r <= x2
 *	(2)(8) x1 <= x <= x2 and (2) y <= y1 and y + r >= y1 (8) y >= y2 and y - r <= y2
 *	(1)(7) x < x1 and (1) y < y1 and (x1-x)^2+(y1-y)^2 <= r^2 (7) y > y2 and (x1-x)^2+(y2-y)^2 <= r^2
 *	(3)(9) x > x2 and (3) y < y1 and (x2-x)^2+(y1-y)^2 <= r^2 (9) y > y2 and (x2-x)^2+(y2-y)^2 <= r^2
 *
 *	NOTE: Expanding button's area can achieve similar result, simpler but not accurate for corner cases
 */
int bt_ripple_handler(int x, int y, DFBRegion *rg, int nr)
{
	int i, r;

	printf("%s(%d) (%d, %d)\n", __func__, __LINE__, x, y);
	//(5)
	for (i = 0; i < nr; i++) {
		printf("target (%d, %d) (%d, %d)\n", rg[i].x1, rg[i].y1, rg[i].x2, rg[i].y2);
		if (rg[i].x1 <= x && x <= rg[i].x2 && rg[i].y1 <= y && y <= rg[i].y2) {
			return i;
		}
	}

	for (r = 0; r < RIP_RAD; r++) {

		long rr = r * r;
		for (i = 0; i < nr; i++) {

			int yy = rg[i].y1; //y1
			int y2 = rg[i].y2;

			//(1)(4)(7)
			int xx = rg[i].x1;
			if (x < xx) {
				if (y < yy) goto corner;    //(1)
				if (y > y2) goto corner_y2; //(7)
				if (x + r > xx) return i;   //(4)
				continue;
			}

			//(3)(6)(9)
			xx = rg[i].x2;
			if (x > xx) {
				if (y < yy) goto corner;    //(3)
				if (y > y2) goto corner_y2; //(9)
				if (x - r < xx) return i;   //(6)
				continue;
			}

			//(2)
			if (y < yy) {
				if (y + r >= yy) return i;
				continue;
			}

			//(8)
//			if (y > y2) {
				if (y - r <= y2) return i;
				continue;
//			}
corner_y2:
			yy = y2;
corner:
			//(1)(7)(3)(9)
			xx -= x;
			yy -= y;
			if ((long)xx*xx + (long)yy*yy <= rr) return i;
		}
	}

	return -1;
}
