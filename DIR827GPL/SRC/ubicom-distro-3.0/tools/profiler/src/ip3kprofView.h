// ip3kprofView.h : interface of the CIp3kprofView class
//
/////////////////////////////////////////////////////////////////////////////

#if !defined(AFX_IP3KPROFVIEW_H__FE10EF4E_E211_4389_8E9D_3A31A71D4F94__INCLUDED_)
#define AFX_IP3KPROFVIEW_H__FE10EF4E_E211_4389_8E9D_3A31A71D4F94__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#define FHEIGHT0 15
#define FHEIGHT1 12
// msec between updates - must be one second for accurate rate calculations
#define UPDATETIME 1000

#define MAXVIEWLINES 40

#define TABHEIGHT 30
#define TOPMARGIN 10
#define INDENT 15

#define DEFAULTSECS 5*60

class CIp3kprofView : public CScrollView
{
protected: // create from serialization only
	CIp3kprofView();
	DECLARE_DYNCREATE(CIp3kprofView)

// Attributes
public:
	CIp3kprofDoc* GetDocument();
	int timeused;	// number of timer ticks used
	int lastpacket;
	int stopcount;// number of seconds with no packets arriving
	COLORREF colors[MAX_STAT_DISPLAY];	// which color to display
	int numsecs;	// how many seconds to display
	int sortby;	// how to sort functions display
	int grheight;
	int grwidth;
	CTabCtrl tabs;
	int dragx;	// last x pixel value during drag operation

// Operations
public:
	void starttimer(void);

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CIp3kprofView)
	public:
	virtual void OnDraw(CDC* pDC);  // overridden to draw this view
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	virtual void OnInitialUpdate();
	protected:
	virtual BOOL OnPreparePrinting(CPrintInfo* pInfo);
	virtual void OnBeginPrinting(CDC* pDC, CPrintInfo* pInfo);
	virtual void OnEndPrinting(CDC* pDC, CPrintInfo* pInfo);
	virtual void OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint);
	virtual BOOL OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult);
	virtual BOOL OnScrollBy(CSize sizeScroll, BOOL bDoScroll = TRUE);
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CIp3kprofView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:
	bool created;
	int timerid;
	CFont printfont;
	CFont viewfont[2];
	int viewht[2];	// height of the font
	int viewwd[2];	// width of the font
	int printht;
	int smallfont;	// use a smaller font
	int first_sec;
	int grcol;	// the number of menu items in each column
	void draw_graph(CDC* pDC, int ht);
	void draw_frags(CDC* pDC, int ht);
// Generated message map functions
protected:
	//{{AFX_MSG(CIp3kprofView)
	afx_msg void OnTimer(UINT nIDEvent);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnViewSmallfont();
	afx_msg void OnUpdateViewSmallfont(CCmdUI* pCmdUI);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnActions1minutegraph();
	afx_msg void OnUpdateActions1minutegraph(CCmdUI* pCmdUI);
	afx_msg void OnActions20minutegraph();
	afx_msg void OnUpdateActions20minutegraph(CCmdUI* pCmdUI);
	afx_msg void OnActions5minutegraph();
	afx_msg void OnUpdateActions5minutegraph(CCmdUI* pCmdUI);
	afx_msg void OnActions60minutegraph();
	afx_msg void OnUpdateActions60minutegraph(CCmdUI* pCmdUI);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnViewGraphduration1minute();
	afx_msg void OnUpdateViewGraphduration1minute(CCmdUI* pCmdUI);
	afx_msg void OnViewGraphduration20minutes();
	afx_msg void OnUpdateViewGraphduration20minutes(CCmdUI* pCmdUI);
	afx_msg void OnViewGraphduration5minutes();
	afx_msg void OnUpdateViewGraphduration5minutes(CCmdUI* pCmdUI);
	afx_msg void OnViewGraphduration60minutes();
	afx_msg void OnUpdateViewGraphduration60minutes(CCmdUI* pCmdUI);
	afx_msg void OnViewSortfunctionsbyLatency();
	afx_msg void OnUpdateViewSortfunctionsbyLatency(CCmdUI* pCmdUI);
	afx_msg void OnViewSortfunctionsbyTime();
	afx_msg void OnUpdateViewSortfunctionsbyTime(CCmdUI* pCmdUI);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnViewSortfunctionsbyName();
	afx_msg void OnUpdateViewSortfunctionsbyName(CCmdUI *pCmdUI);
};

#ifndef _DEBUG  // debug version in ip3kprofView.cpp
inline CIp3kprofDoc* CIp3kprofView::GetDocument()
   { return (CIp3kprofDoc*)m_pDocument; }
#endif

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_IP3KPROFVIEW_H__FE10EF4E_E211_4389_8E9D_3A31A71D4F94__INCLUDED_)
