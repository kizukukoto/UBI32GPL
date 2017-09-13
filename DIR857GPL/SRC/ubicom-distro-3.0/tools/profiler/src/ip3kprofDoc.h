// ip3kprofDoc.h : interface of the CIp3kprofDoc class
//
/////////////////////////////////////////////////////////////////////////////
//
// TODO list:
//
// Autodetect reload or reset of ip3k, and save data and restart

#if !defined(AFX_IP3KPROFDOC_H__B8BA9EE2_2C96_4EE3_BD79_CB339B46A248__INCLUDED_)
#define AFX_IP3KPROFDOC_H__B8BA9EE2_2C96_4EE3_BD79_CB339B46A248__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "profile.h"

class console;
class mysocket;


class CIp3kprofDoc : public CDocument
{
protected: // create from serialization only
	CIp3kprofDoc();
	DECLARE_DYNCREATE(CIp3kprofDoc)

// Attributes
public:
	bool havedoc;
	bool havedata;		// have some data to save
	profile pr;		// the profiler that takes the data 
	int log_rate;
	int current_tab;
	bool closing;		// shutting down, so ignore input

// Operations
public:
	void OnPacket(char *buf, unsigned int size);	// a udp data packet has arrived from the board
	void OnConsoleConnect();			// connection established with profilerd on the board
	void OnConsoleReceive();			// a tcp control packet arrived from profilerd on the board
	void title(){
		if(pr.get_elf_file_count() == 0)
			return;
		CString name = pr.get_elf_file(0);
		name.Replace(".elf", ".txt");
		SetTitle(name);
		m_strPathName = name;
	}

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CIp3kprofDoc)
	public:
	virtual BOOL OnNewDocument();
	virtual void Serialize(CArchive& ar);
	virtual BOOL OnOpenDocument(LPCTSTR lpszPathName);
	virtual BOOL OnSaveDocument(LPCTSTR lpszPathName);
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CIp3kprofDoc();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:
	mysocket *sock;		// socket to receive UDP packets from the device
	CString distro_path;
	BOOL find_elf_file(char *target, char *path, LPCTSTR name);
	int connect_board(void);
// Generated message map functions
protected:
	//{{AFX_MSG(CIp3kprofDoc)
	afx_msg void OnActionsRestart();
	afx_msg void OnViewThreadAll();


	afx_msg void OnUpdateViewThreadAll(CCmdUI* pCmdUI);
	afx_msg void OnViewThreadThread0();
	afx_msg void OnUpdateViewThreadThread0(CCmdUI* pCmdUI);
	afx_msg void OnViewThreadThread1();
	afx_msg void OnUpdateViewThreadThread1(CCmdUI* pCmdUI);
	afx_msg void OnViewThreadThread2();
	afx_msg void OnUpdateViewThreadThread2(CCmdUI* pCmdUI);
	afx_msg void OnViewThreadThread3();
	afx_msg void OnUpdateViewThreadThread3(CCmdUI* pCmdUI);
	afx_msg void OnViewThreadThread4();
	afx_msg void OnUpdateViewThreadThread4(CCmdUI* pCmdUI);
	afx_msg void OnViewThreadThread5();
	afx_msg void OnUpdateViewThreadThread5(CCmdUI* pCmdUI);
	afx_msg void OnViewThreadThread6();
	afx_msg void OnUpdateViewThreadThread6(CCmdUI* pCmdUI);
	afx_msg void OnViewThreadThread7();
	afx_msg void OnUpdateViewThreadThread7(CCmdUI* pCmdUI);
	afx_msg void OnActionsSetclockfrequency();
	afx_msg void OnActionsLogdata();
	afx_msg void OnUpdateActionsLogdata(CCmdUI* pCmdUI);
	afx_msg void OnActionsUpdategraph();
	afx_msg void OnUpdateActionsUpdategraph(CCmdUI* pCmdUI);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnCollectdataforThread8();
	afx_msg void OnUpdateCollectdataforThread8(CCmdUI *pCmdUI);
	afx_msg void OnCollectdataforThread9();
	afx_msg void OnUpdateCollectdataforThread9(CCmdUI *pCmdUI);
	afx_msg void OnCollectdataforThread10();
	afx_msg void OnUpdateCollectdataforThread10(CCmdUI *pCmdUI);
	afx_msg void OnCollectdataforThread11();
	afx_msg void OnUpdateCollectdataforThread11(CCmdUI *pCmdUI);

protected:
	virtual BOOL SaveModified();
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_IP3KPROFDOC_H__B8BA9EE2_2C96_4EE3_BD79_CB339B46A248__INCLUDED_)
