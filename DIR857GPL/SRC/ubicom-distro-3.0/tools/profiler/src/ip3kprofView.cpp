// ip3kprofView.cpp : implementation of the CIp3kprofView class
//

#include "stdafx.h"
#include "ip3kprof.h"
#include "mysocket.h"

#include "ip3kprofDoc.h"
#include "ip3kprofView.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// minimum number of values in each column
#define MIN_GRCOL 4

/////////////////////////////////////////////////////////////////////////////
// CIp3kprofView

IMPLEMENT_DYNCREATE(CIp3kprofView, CScrollView)

BEGIN_MESSAGE_MAP(CIp3kprofView, CScrollView)
	//{{AFX_MSG_MAP(CIp3kprofView)
	ON_WM_TIMER()
	ON_WM_SIZE()
	ON_WM_ERASEBKGND()
	ON_WM_VSCROLL()
	ON_COMMAND(ID_VIEW_SMALLFONT, OnViewSmallfont)
	ON_UPDATE_COMMAND_UI(ID_VIEW_SMALLFONT, OnUpdateViewSmallfont)
	ON_WM_LBUTTONDOWN()
	ON_COMMAND(ID_ACTIONS_1MINUTEGRAPH, OnActions1minutegraph)
	ON_UPDATE_COMMAND_UI(ID_ACTIONS_1MINUTEGRAPH, OnUpdateActions1minutegraph)
	ON_COMMAND(ID_ACTIONS_20MINUTEGRAPH, OnActions20minutegraph)
	ON_UPDATE_COMMAND_UI(ID_ACTIONS_20MINUTEGRAPH, OnUpdateActions20minutegraph)
	ON_COMMAND(ID_ACTIONS_5MINUTEGRAPH, OnActions5minutegraph)
	ON_UPDATE_COMMAND_UI(ID_ACTIONS_5MINUTEGRAPH, OnUpdateActions5minutegraph)
	ON_COMMAND(ID_ACTIONS_60MINUTEGRAPH, OnActions60minutegraph)
	ON_UPDATE_COMMAND_UI(ID_ACTIONS_60MINUTEGRAPH, OnUpdateActions60minutegraph)
	ON_WM_MOUSEMOVE()
	ON_COMMAND(ID_VIEW_GRAPHDURATION_1MINUTE, OnViewGraphduration1minute)
	ON_UPDATE_COMMAND_UI(ID_VIEW_GRAPHDURATION_1MINUTE, OnUpdateViewGraphduration1minute)
	ON_COMMAND(ID_VIEW_GRAPHDURATION_20MINUTES, OnViewGraphduration20minutes)
	ON_UPDATE_COMMAND_UI(ID_VIEW_GRAPHDURATION_20MINUTES, OnUpdateViewGraphduration20minutes)
	ON_COMMAND(ID_VIEW_GRAPHDURATION_5MINUTES, OnViewGraphduration5minutes)
	ON_UPDATE_COMMAND_UI(ID_VIEW_GRAPHDURATION_5MINUTES, OnUpdateViewGraphduration5minutes)
	ON_COMMAND(ID_VIEW_GRAPHDURATION_60MINUTES, OnViewGraphduration60minutes)
	ON_UPDATE_COMMAND_UI(ID_VIEW_GRAPHDURATION_60MINUTES, OnUpdateViewGraphduration60minutes)
//	ON_COMMAND(ID_VIEW_SORTFUNCTIONSBY_LATENCY, OnViewSortfunctionsbyLatency)
//	ON_UPDATE_COMMAND_UI(ID_VIEW_SORTFUNCTIONSBY_LATENCY, OnUpdateViewSortfunctionsbyLatency)
	ON_COMMAND(ID_VIEW_SORTFUNCTIONSBY_TIME, OnViewSortfunctionsbyTime)
	ON_UPDATE_COMMAND_UI(ID_VIEW_SORTFUNCTIONSBY_TIME, OnUpdateViewSortfunctionsbyTime)
	//}}AFX_MSG_MAP
	// Standard printing commands
	ON_COMMAND(ID_FILE_PRINT, CScrollView::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_DIRECT, CScrollView::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_PREVIEW, CScrollView::OnFilePrintPreview)
	ON_COMMAND(ID_VIEW_SORTFUNCTIONSBY_NAME, &CIp3kprofView::OnViewSortfunctionsbyName)
	ON_UPDATE_COMMAND_UI(ID_VIEW_SORTFUNCTIONSBY_NAME, &CIp3kprofView::OnUpdateViewSortfunctionsbyName)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CIp3kprofView construction/destruction

CPen *pens[MAX_STAT_DISPLAY][2];
CPen blackpen(PS_SOLID, 1, RGB(0,0,0));
CPen greypen(PS_SOLID, 1, RGB(190,190,170));


CIp3kprofView::CIp3kprofView()
{
	first_sec = 0;
	numsecs = DEFAULTSECS;
	sortby = SORT_TIME;
	created = false;
	smallfont = 0;
	timerid = 0;
	timeused = 0;
	lastpacket = 0;
	stopcount = 0;
	grheight = 400;
	grwidth = 1000;
	dragx = 0;
	grcol = MIN_GRCOL;

	LOGFONT lf;
	if (!viewfont[0].CreateFont(FHEIGHT0, 0, 0, 0, FW_MEDIUM, 0, 0, 0, ANSI_CHARSET, 
		OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DRAFT_QUALITY, FIXED_PITCH + FF_MODERN, NULL)) {
		AfxMessageBox("Can't create main font");
	}
	viewfont[0].GetLogFont(&lf);
	viewht[0] = lf.lfHeight;
	viewwd[0] = lf.lfWidth;

	if (!viewfont[1].CreateFont(FHEIGHT1, 0, 0, 0, FW_BOLD, 0, 0, 0, ANSI_CHARSET, 
		OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, PROOF_QUALITY, FIXED_PITCH + FF_DONTCARE, NULL)) {
		AfxMessageBox("Can't create small font");
	}
	viewfont[1].GetLogFont(&lf);
	viewht[1] = lf.lfHeight;
	viewwd[1] = lf.lfWidth;

	int i = 0;
	for (int g0 = 0; g0 <= 6; g0++) {
		for (int r0 = 0; r0 <= 6; r0++){
			for (int b0 = 0; b0 <= 6; b0++) {

				int r = ((2 + r0 + b0) * 3) % 7;
				int g = ((4 + r0 + b0 + g0) * 3) % 7;
				int b = (b0 * 3) % 7;

				if (r + b + g < 4) {
					continue;
				}
				if (r + b + g > 12) {
					continue;
				}

				colors[i] = RGB(r * 42, g * 42, b * 42);
				pens[i][0] = new CPen(PS_SOLID, 2, colors[i]);
				pens[i][1] = new CPen(PS_SOLID, 1, colors[i]);
				i++;
				if (i >= MAX_STAT_DISPLAY){
					return;
				}
			}
		}
	}
}

CIp3kprofView::~CIp3kprofView()
{
	for (int i = 0; i < MAX_STAT_DISPLAY; ++i) {
		delete pens[i][0];
		delete pens[i][1];
	}
}

void CIp3kprofView::starttimer()
{
	timerid = SetTimer(12345, UPDATETIME, NULL);
	if (timerid == 0) {
		AfxMessageBox("can't get timer to update view wwindow");
	}
}

BOOL CIp3kprofView::PreCreateWindow(CREATESTRUCT& cs)
{
	cs.style |= WS_VSCROLL;
	return CScrollView::PreCreateWindow(cs);
}

/////////////////////////////////////////////////////////////////////////////
// CIp3kprofView drawing

#define BGCOLOR RGB(255, 250, 245)

#define GRINDENT 40
#define GRRIGHTEDGE 10
#define MENUINDENT 30
#define GRMENUGAP 35
// size of axes
#define GRLEFT 100
#define GRRIGHT 300

#define GRINCR 20
#define GRTXINDENT 5

#define GRTOP (TOPMARGIN + TABHEIGHT + viewht[smallfont])
// pixels in a menu row
#define GRROW 256

#define GRCOLINDENT (GRINDENT*2)

int tcolor[3] = { 175, 0, 100 };

int drawn[MAX_STAT_DISPLAY];	// which element was drawn in the menu position?

// row height
static int pageh = 12;

// pixels per page
#define PAGEW 2
#define PAGEPERROW 512

#define MAX_PAGES (256 * 1024 / 4)

// pixels per row
#define PWSIZE (PAGEW * PAGEPERROW)
#define PHSIZE (pageh * rowperpage)
#define PVINDENT (20 + TABHEIGHT)
#define PHINDENT 50

// line height for legend
#define LEGENDH 20

unsigned int roundpow2(unsigned int x)
{
	x--;
	x |= x >> 1;  // handle  2 bit numbers
	x |= x >> 2;  // handle  4 bit numbers
	x |= x >> 4;  // handle  8 bit numbers
	x |= x >> 8;  // handle 16 bit numbers
	x |= x >> 16; // handle 32 bit numbers
	x++;
 
	return x;
}

#define BLOCK_WASTE RGB(255, 0, 0)


#define PROFILE_MAP_TYPE_UKNOWN PROFILE_MAP_NUM_TYPES
#define PROFILE_MAP_TYPE_ULTRA (PROFILE_MAP_NUM_TYPES + 1)
#define PROFILE_MAP_TYPE_ULTRA_RSV (PROFILE_MAP_NUM_TYPES + 2)
#define PROFILE_MAP_TYPE_ULTRA_HEAP (PROFILE_MAP_NUM_TYPES + 3)
#define PROFILE_MAP_TYPE_LINUX (PROFILE_MAP_NUM_TYPES + 4)

int block_colors[PROFILE_MAP_NUM_TYPES + 5] = {
	RGB(255, 255, 255),	// free
	RGB(150, 0, 150),	// small kernel
	RGB(50, 50, 50),	// file system
	RGB(0, 0, 0),

	RGB(150, 150, 150),	// unknown used
	RGB(0, 150, 0),		// text
	RGB(0, 0, 150),		// stack
	RGB(0, 150, 150),	// app_data
	RGB(150, 100, 0),	// ashmem
	RGB(100, 100, 255),	// read-shared
	RGB(120, 0, 70),	// buffer cache
	RGB(200, 0, 0),		// vma waste
	RGB(0, 0, 0),
	RGB(0, 0, 0),
	RGB(0, 0, 0),
	RGB(0, 0, 0),

	RGB(150, 150, 150),	// unknown
	RGB(0, 200, 0),		// ultra code
	RGB(220, 220, 0),	// ultra reserve
	RGB(150, 80, 0),	// ultra heap
	RGB(0, 200, 0),		// linux
};

char *block_names[PROFILE_MAP_NUM_TYPES + 5] = {
	"Free",
	"0.5K - 8K kernel",
	"File system",
	"unused",

	"Unknown used",
	"App Text    (r-xs)",
	"App Stack   (rwxp)",
	"App Data    (rw-p)",
	"Shared Data (rw-S)",
	"Read-shared (r--s)",
	"Buffer cache (file data)",
	"VMA Waste",
	"unused",
	"unused",
	"unused",
	"unused",

	"Unknown",
	"Ultra code",
	"Ultra reserved",
	"Ultra Heap",
	"Linux Kernel",
};

// TODO: only works for 256 MB memory!
bool drawn_page[MAX_PAGES + 1];

static bool draw_block(CDC* pDC, int start, int size, int type) 
{
	int color = block_colors[type];

	if (start + size > MAX_PAGES) {
		return false;
	}

	for(int i = start; i < start + size; ++i) {
		if (drawn_page[i]) {
			return false;
		}
		drawn_page[i] = true;
	}

	if (type == PROFILE_MAP_TYPE_TEXT)
		color = RGB(0, 150, 0);
	else if (type == PROFILE_MAP_TYPE_STACK)
		color = RGB(0, 0, 150);
	else if (type == PROFILE_MAP_TYPE_APP_DATA)
		color = RGB(0, 150, 150);
	else if (type == PROFILE_MAP_TYPE_FREE)
		color = RGB(255, 255, 255);
	else if (type == PROFILE_MAP_TYPE_ULTRA)
		color = RGB(0, 200, 000);
	else if (type == PROFILE_MAP_TYPE_ULTRA_RSV)
		color = RGB(220, 220, 0);
	else if (type == PROFILE_MAP_TYPE_ULTRA_HEAP)
		color = RGB(150, 80, 0);
	else if (type == PROFILE_MAP_TYPE_LINUX)
		color = RGB(0, 200, 0);
	int xpage = start % (PWSIZE / PAGEW);
	int x = PAGEW * xpage;
	int y = pageh * (start / (PWSIZE / PAGEW));
	if (size != 0) {
		if (xpage + size <= PAGEPERROW) {
			pDC->MoveTo(x, y);
			pDC->LineTo(x, y + pageh);
			pDC->FillSolidRect(x + 1, y + 1, size * PAGEW - 1, pageh - 1, color);
		} else {
			pDC->FillSolidRect(x, y + 1, (PAGEPERROW - xpage) * PAGEW, pageh - 1, color);
			size -= (PAGEPERROW - xpage);
			while (size >= PAGEPERROW) {
				y += pageh;
				pDC->FillSolidRect(0, y + 1, PAGEPERROW * PAGEW, pageh - 1, color);
				size -= PAGEPERROW;
			}
			if (size > 0) {
				y += pageh;
				pDC->FillSolidRect(0, y + 1, size * PAGEW - 1, pageh - 1, color);
			}
		}
	}
	return true;
}

#define MAX_ORDER 13
char *order_label[MAX_ORDER] = {
	"  4 KB",
	"  8 KB",
	" 16 KB",
	" 32 KB",
	" 64 KB",
	"128 KB",
	"256 KB",
	"512 KB",
	"  1 MB",
	"  2 MB",
	"  4 MB",
	"  8 MB",
	" 16 MB",
};

static bool draw_error = false;
void CIp3kprofView::draw_frags(CDC* pDC, int ht)
{
	CIp3kprofDoc* pDoc = GetDocument();
	RECT r;
	int page_shift = pDoc->pr.page_shift;
	assert(page_shift == 12 || page_shift == 14);
	unsigned int sdram_start = pDoc->pr.sdram_begin;
	int total[PROFILE_MAP_NUM_TYPES + 5];
	int free_count[MAX_ORDER];
	int max_free = 0;

	/* partial data, wait for more */
	if (pDoc->pr.getting_map) {
		return;
	}

	if (pDoc->pr.num_pm == 0) {
		pDC->TextOutA(PHINDENT, PVINDENT, "No memory map data received");
		return;
	}

	for (int i = 0; i < MAX_ORDER; ++i) {
		free_count[i] = 0;
	}

	for (int i = 0; i < PROFILE_MAP_NUM_TYPES; ++i) {
		total[i] = 0;
	}

	for (int i = 0; i < MAX_PAGES + 1; ++i)
		drawn_page[i] = false;

	int maxpage = 0;
	int maxstart = 0;
	int linux_start = (pDoc->pr.linux_start - sdram_start) >> page_shift;
	int linux_size = (pDoc->pr.linux_end - pDoc->pr.linux_start) >> page_shift;
	for (int i = 0; i < pDoc->pr.num_pm; ++i) {
		int size = pDoc->pr.pm[i].type_size & PROFILE_MAP_SIZE_MASK;
		int type = pDoc->pr.pm[i].type_size >> PROFILE_MAP_TYPE_SHIFT;
		int start = pDoc->pr.pm[i].start;
		if (size == 0) {
			size = 1 << PROFILE_MAP_TYPE_SHIFT;
		}
		if (start > maxstart) {
			maxstart = start;
		}
		if (start < linux_start) {
			continue;	// OCM block
		}
		if (type == PROFILE_MAP_TYPE_FREE) {
			int order;
			for (order = 0; order < MAX_ORDER; ++order) {
				if ((1 << order) > size) {
					break;
				}
			}
			order--;
			free_count[order]++;
			if (size > max_free) {
				max_free = size;
			}
		} 

		if (size + start > maxpage && size + start <= MAX_PAGES) {
			maxpage = size + start;
		}
	}
	maxpage = roundpow2(maxpage);
	total[PROFILE_MAP_TYPE_UKNOWN] = maxpage;

	int rowperpage = maxpage / PAGEPERROW;
	GetClientRect(&r);
	pageh = (r.bottom - r.top - PVINDENT - 10) / rowperpage;

	// legend on right side
	int vpix = PVINDENT;
	for (int i = 0; i < sizeof(block_colors) / sizeof(int); ++i) {
		if (i > 0 && block_colors[i] == 0)
			continue;
		pDC->FillSolidRect(PHINDENT + PWSIZE + 10, vpix, 15, 15, block_colors[i]);
		vpix += LEGENDH;
	}

	// box around the display
	pDC->MoveTo(PHINDENT - 1, PVINDENT);
	pDC->LineTo(PHINDENT - 1, PVINDENT + PHSIZE);
	pDC->MoveTo(PHINDENT + PWSIZE, PVINDENT);
	pDC->LineTo(PHINDENT + PWSIZE, PVINDENT + PHSIZE);
	pDC->MoveTo(PHINDENT - 1, PVINDENT + PHSIZE);
	pDC->LineTo(PHINDENT + PWSIZE + 1, PVINDENT + PHSIZE);


	CDC outDC;
	CDC *mDC = &outDC;
	CBitmap bm, *oldbm;
	if (!outDC.CreateCompatibleDC(pDC) || 
	   !bm.CreateCompatibleBitmap(pDC, PWSIZE + 1, PHSIZE + 1)) {
		AfxMessageBox("Can't create DC or bitmap!");
		return;
	}
	oldbm = mDC->SelectObject(&bm);
	if (!oldbm) {
		AfxMessageBox("Can't select bitmap");
		return;
	}

	// init to all unknown
	mDC->FillSolidRect(0, 0, PWSIZE - 1, PHSIZE - 1, block_colors[PROFILE_MAP_TYPE_UKNOWN]);

	// row lines
	for (int i = 0; i <= PHSIZE; i += pageh) {
		mDC->MoveTo(0, i);
		mDC->LineTo(PWSIZE - 1, i);
	}
		
	/* ipOS */
	int start = (pDoc->pr.ultra_start - sdram_start) >> page_shift;
	int ipos_size = (pDoc->pr.ultra_size + 4095) >> page_shift;
	draw_block(mDC, start, ipos_size, PROFILE_MAP_TYPE_ULTRA);
	total[PROFILE_MAP_TYPE_ULTRA] = ipos_size;
	start = start + ipos_size;
	int rsv_size = (((pDoc->pr.heap_run_begin - sdram_start) >> page_shift)) - start;
	draw_block(mDC, start, rsv_size, PROFILE_MAP_TYPE_ULTRA_RSV);
	total[PROFILE_MAP_TYPE_ULTRA_RSV] = rsv_size;
	draw_block(mDC, linux_start, linux_size, PROFILE_MAP_TYPE_LINUX);
	total[PROFILE_MAP_TYPE_LINUX] = linux_size;
	start = (pDoc->pr.heap_run_begin - sdram_start) >> page_shift;
	int heap_size = ((pDoc->pr.heap_run_end - sdram_start) >> page_shift) - start;
	draw_block(mDC, start, heap_size, PROFILE_MAP_TYPE_ULTRA_HEAP);
	total[PROFILE_MAP_TYPE_ULTRA_HEAP] = heap_size;

	int last_start = 0;
	int last_size = 0;
	int block_overlaps = 0;
	int overlap_type;
	int overlap_size;
	int overlap_start;
	int max_used = 0;
	int max_start;
	int max_type;
	for (int i = 0; i < pDoc->pr.num_pm; ++i) {
		int size = pDoc->pr.pm[i].type_size & PROFILE_MAP_SIZE_MASK;
		int type = pDoc->pr.pm[i].type_size >> PROFILE_MAP_TYPE_SHIFT;
		int start = pDoc->pr.pm[i].start;
		if (size == 0) {
			size = 1 << PROFILE_MAP_TYPE_SHIFT;
		}
		if (start < linux_start) {
			continue;	// OCM block
		}
		if(!draw_error && start + size > MAX_PAGES) {
			draw_error = true;
			CString tmp;
			tmp.Format("size error: type %d, start %x, size %x", type, start, size);
			AfxMessageBox(tmp);
		}
		if (draw_block(mDC, start, size, type)) {
			if (type != PROFILE_MAP_TYPE_FREE && size > max_used && size <= MAX_PAGES) {
				max_used = size;
				max_type = type;
				max_start = start;
			}
			total[type] += size;
		} else {
			block_overlaps++;
			overlap_type = type;
			overlap_start = start;
			overlap_size = size;
		}
		last_start = start;
		last_size = size;
	}

	// draw the results
	pDC->BitBlt(PHINDENT, PVINDENT, PWSIZE, PHSIZE, mDC, 0, 0, SRCCOPY);

	pDC->SetBkColor(BGCOLOR);
	pDC->TextOutA(PHINDENT, PVINDENT + PHSIZE + 10, "Click on graphic for details");

	for (int i = 0; i < PROFILE_MAP_NUM_TYPES + 5; ++i) {
		if (i != PROFILE_MAP_TYPE_UKNOWN)
			total[PROFILE_MAP_TYPE_UKNOWN] -= total[i];
	}

	for (int i = 0; i <= PHSIZE; i += pageh * 4) {
		char buf[20];
		sprintf(buf, "%d MB", (i / pageh) * (PAGEPERROW * (1 << page_shift)) / (1024 * 1024));
		pDC->TextOutA(0, PVINDENT + i - 8, buf);
	}

	vpix = PVINDENT;
	CString tmp;
	for (int i = 0; i < sizeof(block_colors) / sizeof(int); ++i) {
		if (i > 0 && block_colors[i] == 0) 
			continue;
		tmp.Format("%5.1f MB %s   ", total[i] * (1 << page_shift) / (1024. * 1024.), block_names[i]);
		pDC->TextOutA(PHINDENT + PWSIZE + 10 + 20, vpix, tmp);
		vpix += LEGENDH;
	}
	tmp.Format("%d memory regions", pDoc->pr.num_pm);
	pDC->TextOutA(PHINDENT + PWSIZE + 10 + 20, vpix, tmp);
	vpix += LEGENDH * 2;
	pDC->TextOutA(PHINDENT + PWSIZE + 10, vpix, "Free memory blocks");
	vpix += LEGENDH;
	for (int i = 0; i < MAX_ORDER; ++i) {
		tmp.Format("%4d * %s = %5.1f MB ", free_count[i], order_label[i], free_count[i] * (1 << page_shift) * (1 << i) / (1024. * 1024.));
		pDC->TextOutA(PHINDENT + PWSIZE + 10, vpix, tmp);
		vpix += LEGENDH;
	}
	tmp.Format("Max free block = %5.1f MB ", max_free * (1 << page_shift) / (1024. * 1024.));
	pDC->TextOutA(PHINDENT + PWSIZE + 10 + 20, vpix, tmp);
	vpix += LEGENDH;
	tmp.Format("Max used block = %5.1f MB, type %s, at %5.1f MB ", 
		max_used * (1 << page_shift) / (1024. * 1024.),
		block_names[max_type],
		max_start * (1 << page_shift) / (1024. * 1024.)
		);
	pDC->TextOutA(PHINDENT + PWSIZE + 10 + 20, vpix, tmp);
	vpix += LEGENDH;
	if (block_overlaps) {
		tmp.Format("Warning: %d overlapping blocks not drawn", block_overlaps);
		pDC->TextOutA(PHINDENT + PWSIZE + 10 + 20, vpix, tmp);
		vpix += LEGENDH;
		tmp.Format("     One is %5.1f MB, type %s, at %5.1f MB   ",
			overlap_size * (1 << page_shift) / (1024. * 1024.),
			block_names[overlap_type],
			overlap_start * (1 << page_shift) / (1024. * 1024.));
		pDC->TextOutA(PHINDENT + PWSIZE + 10 + 20, vpix, tmp);
		vpix += LEGENDH;
	}

	// free resources
	mDC->SelectObject(oldbm);
}

void CIp3kprofView::draw_graph(CDC* pDC, int ht)
{
	CIp3kprofDoc* pDoc = GetDocument();
	RECT r;
	GetClientRect(&r);
	grwidth = (r.right-r.left) - GRINDENT * 2 - GRRIGHTEDGE;
	int menu_rows = grwidth /GRROW;		// number of full rows
	int menu_count = 0;	// how many menu items are there?
	for (int g = 0; g < MAX_STAT_DISPLAY; ++g) {
		if (stat_data[g].found) {
			menu_count++;
		}
	}
	int new_grcol = (menu_count + menu_rows - 1) / menu_rows;
	if (new_grcol < MIN_GRCOL) {
		new_grcol = MIN_GRCOL;
	}
	if (grcol != new_grcol) {
		grcol = new_grcol;
		pDC->FillSolidRect(0, 0, r.right, r.bottom, BGCOLOR); 
	}

	grheight = (r.bottom-r.top) - GRTOP - GRMENUGAP - grcol * ht;
	CDC mDC;
	CBitmap bm, *oldbm;
	if (!mDC.CreateCompatibleDC(pDC) || 
	   !bm.CreateCompatibleBitmap(pDC, grwidth, grheight)) {
		AfxMessageBox("Can't create DC or bitmap!");
		return;
	}
	oldbm = mDC.SelectObject(&bm);

	// axes
	CString tmp;
	pDC->TextOut(GRINDENT, TOPMARGIN + TABHEIGHT, tmp);
	pDC->MoveTo(GRINDENT - 1, GRTOP - 1);
	pDC->LineTo(GRINDENT - 1, GRTOP + grheight + 1);
	pDC->LineTo(GRINDENT + grwidth + 1, GRTOP + grheight + 1);
	pDC->LineTo(GRINDENT + grwidth + 1, GRTOP - 1);
	pDC->LineTo(GRINDENT - 1, GRTOP - 1);
	int grincr = GRINCR;
	if (grheight < grincr * ht) {
		grincr /= 2;
	}

	// left and right Y axes
	for (int i = 0; i <= grincr; ++i) {
		CString tmp1, tmp2;
		tmp1.Format("%3d", i * GRLEFT / grincr);
		tmp2.Format("%3d", i * GRRIGHT / grincr);
		pDC->TextOut(GRTXINDENT, GRTOP + grheight - ht / 2 - i * grheight / grincr, tmp1); 
		pDC->TextOut(GRINDENT + grwidth + GRTXINDENT, GRTOP + grheight - ht / 2 - i * grheight / grincr, tmp2); 
	}

	//X axis
	for (int i = 0; i < 10; i ++) {
		CString tmp1;
		int time = first_sec + i * numsecs / 10;
		tmp1.Format("%2d:%02d", time / 60, time % 60);
		pDC->TextOut(GRINDENT + grwidth - 20 - grwidth * i / 10, GRTOP + grheight + 2, tmp1);
	}

	mDC.FillSolidRect(0, 0, grwidth + 1, grheight + 1, RGB(255, 255, 255)); 
	mDC.SetBkColor(BGCOLOR);

	// grid
	mDC.SelectObject(greypen);
	for (int i = 1; i < 10; ++i) {
		mDC.MoveTo(0, grheight * i / 10);
		mDC.LineTo(grwidth, grheight * i / 10);
	}
	// data
	for (int g = 0; g < MAX_STAT_DISPLAY; ++g) {
		if (!stat_data[g].enabled) {
			continue;
		}
		mDC.SelectObject(pens[g][stat_data[g].enabled - 1]);
		int ind = pDoc->pr.gnow - first_sec - numsecs;
		if (ind < 0) {
			ind += MAXSECS;
		}
		mDC.MoveTo(0, grheight - (int)(grheight * pDoc->pr.gval_hist[g][ind]));
		for (int i = 1; i <= numsecs; ++i) {
			ind = pDoc->pr.gnow - first_sec - numsecs + i; 
			if (ind < 0) {
				ind += MAXSECS;
			}
			double val= pDoc->pr.gval_hist[g][ind];
			if (val > 1.0) {
				val = 1.0;
			}
			if (val < 0.0) {
				val = 0.0;
			}
			mDC.LineTo(i * grwidth / numsecs, grheight - (int)(grheight * val));
		}
	}
	pDC->BitBlt(GRINDENT, GRTOP, grwidth, grheight, &mDC, 0, 0, SRCCOPY);

	// menu
	int pos = 0;
	for (int g = 0; g < MAX_STAT_DISPLAY; ++g) {
		if (!stat_data[g].found) {
			continue;
		}
		drawn[pos] = g;
		int x = MENUINDENT + GRROW * (pos / grcol);
		int y = GRTOP + grheight + GRMENUGAP + (pos % grcol) * ht;
		int c = tcolor[stat_data[g].enabled];
		pDC->SetTextColor(RGB(c, c, c));
		tmp.Format("%6.0f %-22s", stat_data[g].range, stat_data[g].s); 
		pDC->TextOut(x, y, tmp);
		pDC->FillSolidRect(x - MENUINDENT + 5, y + 2, MENUINDENT - 10, ht - 4, colors[g]);
		pDC->SetBkColor(BGCOLOR);
		pos++;
	}

	// free resources
	mDC.SelectObject(oldbm);
}

void CIp3kprofView::OnDraw(CDC* pDC)
{
	pDC->SetBkColor(BGCOLOR);
	CIp3kprofDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	RECT client;
	GetClientRect(&client);

	CFont *oldfont = NULL;
	int ht;
	int line_per_page;
	if (pDC->IsPrinting()) {
		oldfont = (CFont *)pDC->SelectObject(&printfont);
		ht = printht * 11 / 10;
	} else {
		oldfont = (CFont *)pDC->SelectObject(&viewfont[smallfont]);
		ht = viewht[smallfont];
		GetCharWidth32(pDC->GetSafeHdc(), (int)'m', (int)'m', &viewwd[smallfont]);
	}
	line_per_page = (client.bottom - client.top) / ht;

	if (pDoc->current_tab == P_GRAPH) {
		draw_graph(pDC, ht);
	} else if (pDoc->current_tab == P_MEM_FRAG) {
		draw_frags(pDC, ht);
	} else if (pDoc->current_tab != -1) {
		CPoint org = pDC->GetViewportOrg();
		int i;
		for (i = 0; i < pDoc->pr.lastline[pDoc->current_tab]; ++i) {
			int top = TOPMARGIN + TABHEIGHT + i * ht;
			if (top < TABHEIGHT - org.y) {
				continue;	// don't draw over the tabs
			}
			CString tmp = pDoc->pr.lines[pDoc->current_tab][i];
			tmp += "                                                                                                                                                                          ";
			if (strncmp(tmp, "Warning", 7) == 0 || strncmp(tmp, "ERROR", 5) == 0) {
				pDC->SetTextColor(RGB(220, 50, 50));
			} else {
				pDC->SetTextColor(RGB(0, 0, 0));
			}
			pDC->TextOut(INDENT, top, tmp);
			if (pDoc->pr.bars[pDoc->current_tab][i]) {
				int start = pDoc->pr.bar_indent[pDoc->current_tab] * viewwd[smallfont];
				int len = (int)((client.right * 3 / 4 - start) * pDoc->pr.bars[pDoc->current_tab][i] / pDoc->pr.bar_range[pDoc->current_tab]);
				if (len > 0) {
					pDC->FillSolidRect(start, top + 1, len, ht - 2, RGB(0, 0, 128));
					pDC->SetBkColor(BGCOLOR);
				}
			}
		}
		for (int j = i; j <  line_per_page; ++j) {
			int top = TOPMARGIN + TABHEIGHT + j * ht;
			if (top < TABHEIGHT - org.y) {
				continue;	// don't draw over the tabs
			}
			CString tmp = "                                                                                                                                                                          ";
			pDC->TextOut(INDENT, top, tmp);
		}
	}
	pDC->SelectObject(oldfont);
}

void CIp3kprofView::OnLButtonDown(UINT nFlags, CPoint point) 
{
	CIp3kprofDoc* pDoc = GetDocument();
	if (pDoc->current_tab == P_GRAPH) {
		if (point.y < GRTOP || point.y > GRTOP + grheight) {
			dragx = 0;
		} else { 
			dragx = point.x;
		}
		if (point.y < GRTOP + grheight) {
			return;
		}
		int x = (point.x - MENUINDENT)/GRROW;
		int y =	(point.y - GRTOP - grheight - GRMENUGAP)/viewht[smallfont];
		int i = x * grcol + y;
		if (i < 0 || i >= MAX_STAT_DISPLAY) {
			return;
		}
		i = drawn[i];
		stat_data[i].enabled++;
		if (stat_data[i].enabled >= 3) {
			stat_data[i].enabled = 0;
		}
		pDoc->UpdateAllViews(NULL);	
	} else if (pDoc->current_tab == P_MEM_FRAG) {
		int x = point.x - PHINDENT;
		if (x < 0 || x > PWSIZE)
			return;
		int y = point.y - PVINDENT;
		if (y < 0)
			return;
		int page = x / PAGEW + (y / pageh) * PAGEPERROW;
		for (int i = 0; i < pDoc->pr.num_pm; ++i) {
			int size = pDoc->pr.pm[i].type_size & PROFILE_MAP_SIZE_MASK;
			int type = pDoc->pr.pm[i].type_size >> PROFILE_MAP_TYPE_SHIFT;
			int start = pDoc->pr.pm[i].start;
			if (size == 0) {
				size = 1 << PROFILE_MAP_TYPE_SHIFT;
			}
			if (page >= start && page < start + size) {
				CString tmp;
				tmp.Format("address 0x%x, type %s, size %5.1f  MB", 
					pDoc->pr.sdram_begin + start * (1 << pDoc->pr.page_shift),
					block_names[type],
					size * (1 << pDoc->pr.page_shift) / (1024. * 1024.)
					);
				AfxMessageBox(tmp);
				break;
			}
		}
	} else {
		CScrollView::OnLButtonDown(nFlags, point);
	}
}


/////////////////////////////////////////////////////////////////////////////
// CIp3kprofView printing

BOOL CIp3kprofView::OnPreparePrinting(CPrintInfo* pInfo)
{
	// default preparation
	pInfo->SetMaxPage(1);
	return DoPreparePrinting(pInfo);
}

void CIp3kprofView::OnBeginPrinting(CDC* pDC, CPrintInfo* pInfo)
{
	int width = pDC->GetDeviceCaps (HORZRES);
	int wt = width/100;
	printht = (int)(wt*2.2);
	printfont.CreateFont(0, wt, 0, 0, FW_NORMAL, 0, 0, 0, ANSI_CHARSET, 
		OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, PROOF_QUALITY, FIXED_PITCH, NULL);
}

void CIp3kprofView::OnEndPrinting(CDC* pDC, CPrintInfo* pInfo)
{
	printfont.DeleteObject();
}

/////////////////////////////////////////////////////////////////////////////
// CIp3kprofView diagnostics

#ifdef _DEBUG
void CIp3kprofView::AssertValid() const
{
	CScrollView::AssertValid();
}

void CIp3kprofView::Dump(CDumpContext& dc) const
{
	CScrollView::Dump(dc);
}

CIp3kprofDoc* CIp3kprofView::GetDocument() // non-debug version is inline
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CIp3kprofDoc)));
	return (CIp3kprofDoc*)m_pDocument;
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CIp3kprofView message handlers

void CIp3kprofView::OnTimer(UINT nIDEvent) 
{
	CIp3kprofDoc* pDoc = GetDocument();
	timeused++;
	int timelimit = ((CIp3kprofApp *)AfxGetApp())->cmdInfo.timelimit;
	// exit on idle
	if (((CIp3kprofApp *)AfxGetApp())->cmdInfo.stop_on_idle &&
		pDoc->pr.packet_count > 10 && pDoc->pr.packet_count == lastpacket) {
		stopcount++;
		if (stopcount >= ((CIp3kprofApp *)AfxGetApp())->cmdInfo.stop_seconds) {
			pDoc->closing = true;
			pDoc->OnSaveDocument(NULL);
			AfxGetMainWnd()->PostMessage(WM_CLOSE, 0, 0);
		}
	} else {
		stopcount = 0;
		if (timelimit != 0 && timeused * UPDATETIME / 1000. > timelimit) {	// exit program if time up
			pDoc->closing = true;
			pDoc->OnSaveDocument(NULL);
			AfxGetMainWnd()->PostMessage(WM_CLOSE, 0, 0);
		}
	}
	lastpacket = pDoc->pr.packet_count;

	// update the display
	if (pDoc->current_tab != tabs.GetCurSel()) {
		dragx = 0;
		Invalidate();
	}
	pDoc->current_tab = tabs.GetCurSel();

	// update the data for the outputs once a second
	pDoc->pr.create_display(((CIp3kprofApp *)AfxGetApp())->cmdInfo.btb,
		pDoc->current_tab, sortby);

	if (pDoc->current_tab != -1) {
		pDoc->UpdateAllViews(NULL);
	}
}

void CIp3kprofView::OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint) 
{	
	int height = 40*viewht[smallfont];
	unsigned int width = 80*viewwd[smallfont];
	CIp3kprofDoc* pDoc = GetDocument();
	if (created) {
		int sel = tabs.GetCurSel();

		height = TOPMARGIN + TABHEIGHT + pDoc->pr.lastline[sel]*viewht[smallfont];		// height of this display
		width = 0;		// width of this display
		for (int i = 0; i < pDoc->pr.lastline[sel]; ++i) {
			if (strlen(pDoc->pr.lines[sel][i]) > width) {
				width = strlen(pDoc->pr.lines[sel][i]);
			}
		}
	}
	SetScrollSizes(MM_TEXT, CSize(INDENT + width * viewwd[smallfont], height),
		CSize(20*viewht[smallfont], 20*viewwd[smallfont]),
		CSize(5*viewht[smallfont], viewwd[smallfont]));
	Invalidate(FALSE);	
}

extern char *pagenames[];

void CIp3kprofView::OnInitialUpdate() 
{
	CScrollView::OnInitialUpdate();

	if (!created) {
		CRect rect;
		GetClientRect(&rect);
		rect.bottom = TABHEIGHT;
		tabs.Create(TCS_FOCUSNEVER | WS_CHILD | WS_VISIBLE, rect, this, 0x1111);	
		TCITEM ti;
		ti.mask = TCIF_TEXT;
		for (int i = 0; i < MAXPAGES; ++i) {
			ti.pszText = pagenames[i];
			tabs.InsertItem(i, &ti);
		}
	}
	created = true;
	GetDocument()->title();
}

void CIp3kprofView::OnSize(UINT nType, int cx, int cy) 
{
	CScrollView::OnSize(nType, cx, cy);
	if (created) {
		tabs.MoveWindow(0, 0, cx, TABHEIGHT);
	}
}

BOOL CIp3kprofView::OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult) 
{
	NMHDR *nm = (NMHDR *)lParam;
	if (nm->code == TCN_SELCHANGE) {
		Invalidate();
		CIp3kprofDoc* pDoc = GetDocument();
		pDoc->UpdateAllViews(NULL);
	}
	return CScrollView::OnNotify(wParam, lParam, pResult);
}


BOOL CIp3kprofView::OnEraseBkgnd(CDC* pDC) 
{	
	RECT rect;
	GetClientRect(&rect);
	CPoint org = pDC->GetViewportOrg();
	pDC->FillSolidRect(0, TABHEIGHT-org.x, rect.right, rect.bottom, BGCOLOR);
	return TRUE;
}

BOOL CIp3kprofView::OnScrollBy(CSize sizeScroll, BOOL bDoScroll) 
{
	int ret = CScrollView::OnScrollBy(sizeScroll, bDoScroll);
	if (created) {
		RECT rect;
		GetClientRect(&rect);
		tabs.MoveWindow(0, 0, rect.right, TABHEIGHT);
	}
	return ret;
}

void CIp3kprofView::OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar) 
{
	// TODO: Add your message handler code here and/or call default
	
	CScrollView::OnVScroll(nSBCode, nPos, pScrollBar);
	Invalidate();
}

void CIp3kprofView::OnViewSmallfont() 
{
	smallfont = 1-smallfont;
	Invalidate();
}

void CIp3kprofView::OnUpdateViewSmallfont(CCmdUI* pCmdUI) 
{
	pCmdUI->SetCheck(smallfont);	
}

void CIp3kprofView::OnActions1minutegraph() 
{
	numsecs = 60;	
}

void CIp3kprofView::OnUpdateActions1minutegraph(CCmdUI* pCmdUI) 
{
	pCmdUI->SetCheck(numsecs == 60);	
}

void CIp3kprofView::OnActions20minutegraph() 
{
	numsecs = 20*60;	
}

void CIp3kprofView::OnUpdateActions20minutegraph(CCmdUI* pCmdUI) 
{
	pCmdUI->SetCheck(numsecs == 20*60);	
}

void CIp3kprofView::OnActions5minutegraph() 
{
	numsecs = 5*60;	
}

void CIp3kprofView::OnUpdateActions5minutegraph(CCmdUI* pCmdUI) 
{
	pCmdUI->SetCheck(numsecs == 5*60);	
}

void CIp3kprofView::OnActions60minutegraph() 
{
	numsecs = 60*60;	
}

void CIp3kprofView::OnUpdateActions60minutegraph(CCmdUI* pCmdUI) 
{
	pCmdUI->SetCheck(numsecs == 60*60);	
}

void CIp3kprofView::OnMouseMove(UINT nFlags, CPoint point) 
{
	if (nFlags & MK_LBUTTON) {
		CIp3kprofDoc* pDoc = GetDocument();
		if (pDoc->current_tab == P_GRAPH && point.y > GRTOP && point.y < GRTOP + grheight && dragx != 0) {
			double dsecs = (point.x - dragx) * (double)numsecs/grwidth;
			if (dsecs > 1 || dsecs < -1) {
				first_sec += (int)dsecs;
				if (first_sec < 0)
					first_sec = 0;
				dragx = point.x;
				pDoc->UpdateAllViews(NULL);
			}
		}
	}
	CScrollView::OnMouseMove(nFlags, point);
}

void CIp3kprofView::OnViewGraphduration1minute() 
{
	numsecs = 1*60;	
	
}

void CIp3kprofView::OnUpdateViewGraphduration1minute(CCmdUI* pCmdUI) 
{
	pCmdUI->SetCheck(numsecs == 1*60);	
	
}

void CIp3kprofView::OnViewGraphduration20minutes() 
{
	numsecs = 20*60;	
	
}

void CIp3kprofView::OnUpdateViewGraphduration20minutes(CCmdUI* pCmdUI) 
{
	pCmdUI->SetCheck(numsecs == 20*60);	
	
}

void CIp3kprofView::OnViewGraphduration5minutes() 
{
	numsecs = 5*60;	
	
}

void CIp3kprofView::OnUpdateViewGraphduration5minutes(CCmdUI* pCmdUI) 
{
	pCmdUI->SetCheck(numsecs == 5*60);	
	
}

void CIp3kprofView::OnViewGraphduration60minutes() 
{
	numsecs = 60*60;	
	
}

void CIp3kprofView::OnUpdateViewGraphduration60minutes(CCmdUI* pCmdUI) 
{
	pCmdUI->SetCheck(numsecs == 60*60);	
	
}

#if 0
void CIp3kprofView::OnViewSortfunctionsbyLatency() 
{
	sortby = SORT_LATENCY;	
}

void CIp3kprofView::OnUpdateViewSortfunctionsbyLatency(CCmdUI* pCmdUI) 
{
	pCmdUI->SetCheck(sortby == SORT_LATENCY);	
}
#endif



void CIp3kprofView::OnViewSortfunctionsbyTime() 
{
	sortby = SORT_TIME;	
}

void CIp3kprofView::OnUpdateViewSortfunctionsbyTime(CCmdUI* pCmdUI) 
{
	pCmdUI->SetCheck(sortby == SORT_TIME);	
	
}

void CIp3kprofView::OnViewSortfunctionsbyName()
{
	sortby = SORT_NAME;	
}

void CIp3kprofView::OnUpdateViewSortfunctionsbyName(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck(sortby == SORT_NAME);	
}
