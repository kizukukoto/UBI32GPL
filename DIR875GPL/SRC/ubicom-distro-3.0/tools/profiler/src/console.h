#pragma once

// Cconsole command target
class CIp3kprofDoc;

class console : public CAsyncSocket
{
public:
	console(CIp3kprofDoc *);
	virtual ~console();
	bool connect(CString ipaddress);	// connect and get map data
protected:
	CIp3kprofDoc *owner;
	CString ipaddr;
public:
	virtual void OnConnect(int nErrorCode);
	virtual void OnReceive(int nErrorCode);
};


