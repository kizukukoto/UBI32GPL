// cmdline.h: interface for the cmdline class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_CMDLINE_H__636AF19B_7FB4_4C63_8B74_9C2E7FC0C440__INCLUDED_)
#define AFX_CMDLINE_H__636AF19B_7FB4_4C63_8B74_9C2E7FC0C440__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#define MAX_NAMES 100
#define MAX_NAME_LENGTH 200
#define MAX_LIB_PATHS 100

unsigned int xtoi(char *p);

class cmdline : public CCommandLineInfo  
{
public:
	cmdline();
	virtual ~cmdline();
	int timelimit;
	bool stop_on_idle;
	bool fix_mispredicts;
	bool btb;
	bool autoload;
	bool log_data;
	int log_seconds;
	int stop_seconds;
	bool have_address;
	CString ipaddress;
	CString distro_path;
//	unsigned int offset[MAX_NAMES];	// the address offset to use for this file
	char names[MAX_NAMES][MAX_NAME_LENGTH];
	int next_name;
	CString lib_path[MAX_LIB_PATHS];
	int next_lib_path;
	void ParseParam( LPCTSTR lpszParam, BOOL bFlag, BOOL bLast ){
		CString param = lpszParam;
		if (bFlag && *lpszParam == 't' && strlen(lpszParam) > 2)
			timelimit = atoi(lpszParam + 2);
		else if(bFlag && *lpszParam == 'S') {
			stop_on_idle = true;
			stop_seconds = 10;
			if(param.GetLength() > 2) {
				param = param.Mid(2);
				stop_seconds = atoi(param);
				if(stop_seconds == 0)
					stop_seconds = 6;
			}
		}
		else if (bFlag && param[0] == 'a') {
			autoload = true;
		} 
		else if (bFlag && param == "btb") {
			btb = true;
		}
		else if (bFlag && param.Find("D:") == 0) {
			distro_path = &(lpszParam[2]);
			distro_path += "\\";
		}
		else if (bFlag && param.Find("lib:") == 0) {
			if (next_lib_path < MAX_LIB_PATHS) {
				lib_path[next_lib_path] = &(lpszParam[4]);
				lib_path[next_lib_path] += "\\";
				next_lib_path++;
			}
		}
		else if (bFlag && param.Find("log") == 0) {
			log_data = true;
			log_seconds = 6;
			if (param.GetLength() > 4) {
				param = param.Mid(4);
				log_seconds = atoi(param);
				if (log_seconds == 0)
					log_seconds = 6;
			}
		}
		else if (bFlag && *lpszParam == 'h' || bFlag && *lpszParam == '?') {
			AfxMessageBox("Usage: ip3kprof [-t:seconds] [-S[:seconds]] [-log[:seconds]] [-ip:board-ip-address] [-D:distro_path] [-a] [-lib:library_path] [ultra-elf-file linux-elf-file] [application-or-library-elf-file] ...");
		}
		else if (bFlag && *lpszParam == 'i' && *(lpszParam + 1) == 'p') {
			ipaddress = lpszParam + 3;
			have_address = true;
		}
		else {
			/* get all the file name parameters */
			if (!bFlag && next_name < MAX_NAMES - 1) {
				strncpy(names[next_name], lpszParam, MAX_NAME_LENGTH);
				names[next_name][MAX_NAME_LENGTH - 1] = 0;
				next_name++;
			}
			CCommandLineInfo::ParseParam(lpszParam, bFlag, bLast);
		}
	}
};

#endif // !defined(AFX_CMDLINE_H__636AF19B_7FB4_4C63_8B74_9C2E7FC0C440__INCLUDED_)
