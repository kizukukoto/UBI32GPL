#if !defined(AFX_MYSOCKET_H__1FFE720D_6804_4702_9DF3_F9F8C1515960__INCLUDED_)
#define AFX_MYSOCKET_H__1FFE720D_6804_4702_9DF3_F9F8C1515960__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// mysocket.h : header file
//

class CIp3kprofDoc;

/////////////////////////////////////////////////////////////////////////////
// mysocket command target

class mysocket : public CAsyncSocket
{
// Attributes
public:

// Operations
public:
	mysocket(CIp3kprofDoc *);
	virtual ~mysocket();

// Overrides
public:
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(mysocket)
	public:
	virtual void OnReceive(int nErrorCode);
	//}}AFX_VIRTUAL

	// Generated message map functions
	//{{AFX_MSG(mysocket)
		// NOTE - the ClassWizard will add and remove member functions here.
	//}}AFX_MSG

// Implementation
protected:
	CIp3kprofDoc *owner;
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_MYSOCKET_H__1FFE720D_6804_4702_9DF3_F9F8C1515960__INCLUDED_)
