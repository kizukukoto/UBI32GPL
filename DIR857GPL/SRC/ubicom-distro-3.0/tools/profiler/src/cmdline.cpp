// cmdline.cpp: implementation of the cmdline class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "ip3kprof.h"
#include "cmdline.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

unsigned int xtoi(char *p)
{
	unsigned int result = 0;
	for (int i = 0; i < 8; ++i) {
		if (*p == 0)
			return result;
		unsigned int digit = 0;
		if (*p >= '0' && *p <= '9') {
			digit = *p - '0';
		} else if (*p >= 'a' && *p <= 'f') {
			digit = 10 + *p - 'a';
		} else if (*p >= 'A' && *p <= 'F') {
			digit = 10 + *p - 'A';
		}
		result = (result << 4) + digit;
		p++;
	}
	return result;
}

cmdline::cmdline()
{
	timelimit = 0;
	stop_on_idle = false;
	fix_mispredicts = false;
	have_address = false;
	log_data = false;
	autoload = false;
	no_gui = false;
	log_seconds = 6;
	stop_seconds = 10;
	btb = false;
	next_name = 0;
	next_lib_path = 0;
	distro_path = ".\\";
}

cmdline::~cmdline()
{

}
