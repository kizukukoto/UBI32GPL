// ip3kprof.h : main header file for the IP3KPROF application
//

#if !defined(AFX_IP3KPROF_H__500B2A39_8FB4_4D7B_8C36_6A8FEAF46EFF__INCLUDED_)
#define AFX_IP3KPROF_H__500B2A39_8FB4_4D7B_8C36_6A8FEAF46EFF__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"       // main symbols
#include "cmdline.h"

/////////////////////////////////////////////////////////////////////////////
// CIp3kprofApp:
// See ip3kprof.cpp for the implementation of this class
//

class CIp3kprofApp : public CWinApp
{
public:
	CIp3kprofApp();
	cmdline cmdInfo;
	bool view_all_latency;

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CIp3kprofApp)
	public:
	virtual BOOL InitInstance();
	//}}AFX_VIRTUAL

// Implementation
	//{{AFX_MSG(CIp3kprofApp)
	afx_msg void OnAppAbout();
	afx_msg void OnViewAllLatency();
	afx_msg void OnUpdateViewAllLatency(CCmdUI* pCmdUI);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

extern CIp3kprofApp theApp;
/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_IP3KPROF_H__500B2A39_8FB4_4D7B_8C36_6A8FEAF46EFF__INCLUDED_)
