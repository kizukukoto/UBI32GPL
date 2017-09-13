// console.cpp : implementation file
//

#include "stdafx.h"
#include "ip3kprof.h"
#include "console.h"
#include "ip3kprofdoc.h"

// Cconsole

console::console(CIp3kprofDoc * doc)
{
	owner = doc;
}

console::~console()
{
	owner = NULL;
}

bool console::connect(CString ipaddress)
{
	ipaddr = ipaddress;
	if (!Create()) {
		return false;
	}
	if (!Connect(ipaddress, PROFILE_CONTROL_PORT)) {	//connection to board under test
		int error = GetLastError();
		if (error != WSAEWOULDBLOCK) {
			CString tmp;
			tmp.Format("Can't connect to the board at %s.  Check the IP address and firewalls.\nCheck that profilerd is built and installed and running on the board.", ipaddr);
			AfxMessageBox(tmp);
		}
	}
	return true;
}


void console::OnConnect(int nErrorCode)
{
	if (nErrorCode != 0) {
		CString tmp;
		tmp.Format("Can't connect to the board at %s.  Check the IP address and firewalls.\nCheck that profilerd is built and installed and running on the board.", ipaddr);
		AfxMessageBox(tmp);
	} else {
		owner->OnConsoleConnect();
	}
	CAsyncSocket::OnConnect(nErrorCode);
}

void console::OnReceive(int nErrorCode)
{
	if (nErrorCode == 0) {
		owner->OnConsoleReceive();
	}
	CAsyncSocket::OnReceive(nErrorCode);
}
