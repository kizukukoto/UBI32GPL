// mysocket.cpp : implementation file
//

#include "stdafx.h"
#include "ip3kprof.h"
#include "mysocket.h"
#include "ip3kprofdoc.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// mysocket

mysocket::mysocket(CIp3kprofDoc *p)
{
	owner = p;
}

mysocket::~mysocket()
{
}


// Do not edit the following lines, which are needed by ClassWizard.
#if 0
BEGIN_MESSAGE_MAP(mysocket, CAsyncSocket)
	//{{AFX_MSG_MAP(mysocket)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()
#endif	// 0

/////////////////////////////////////////////////////////////////////////////
// mysocket member functions

#define MAXBUF 2000

void mysocket::OnReceive(int nErrorCode) 
{
	char buf[MAXBUF];
	unsigned int remoteport;
	CString remoteip;

	int size = ReceiveFrom(buf, MAXBUF-1, remoteip, remoteport); //make it smaller so we can experiment mulitple notifications
	if (size == 0) {
		buf[0] = 0;
	}
	else if (size == SOCKET_ERROR) { 
		if (GetLastError() != WSAEWOULDBLOCK) {
			if (GetLastError() != WSAEMSGSIZE) {
				char msg[256];
				sprintf(msg, "OnReceive error: %d", GetLastError());
				AfxMessageBox(msg);
				buf[0] = 0;
			}
			else {
				AfxMessageBox("The datagram was too large and was truncated");
			}
		}
	}
	else {
		owner->OnPacket(buf, size);
	}

	CAsyncSocket::OnReceive(nErrorCode);
}
