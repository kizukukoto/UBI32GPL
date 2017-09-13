// ip3kprofDoc.cpp : implementation of the CIp3kprofDoc class
//

#include "stdafx.h"
#include "mysocket.h"
#include "console.h"
#include "ip3kprof.h"
#include <sys/types.h>
#include <sys/stat.h>
#include "ip3kprofDoc.h"
#include "ip3kprofview.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

void fatal_error(const char *err) {
	CString tmp = "Error: ";
	tmp += err;
	AfxMessageBox(tmp);
}

static bool send_error = false;

console *linux_console;
/*
 * send a message to the board on the control TCP channel
 */
bool linux_console_send(const char *msg)
{
	char buf[500];
	if (!linux_console) {
		return false;
	}
	int err = linux_console->Send(msg, strlen(msg));
	if (err == SOCKET_ERROR) { 
		if (!send_error) {
			sprintf(buf, "Send to board failed, sending: %s", msg);
			AfxMessageBox(buf);
			send_error = true;
		}
	}
	return true;
}

/////////////////////////////////////////////////////////////////////////////
// CIp3kprofDoc

IMPLEMENT_DYNCREATE(CIp3kprofDoc, CDocument)

BEGIN_MESSAGE_MAP(CIp3kprofDoc, CDocument)
	//{{AFX_MSG_MAP(CIp3kprofDoc)
	ON_COMMAND(ID_ACTIONS_RESTART, OnActionsRestart)
	ON_COMMAND(ID_VIEW_THREAD_ALL, OnViewThreadAll)
	ON_UPDATE_COMMAND_UI(ID_VIEW_THREAD_ALL, OnUpdateViewThreadAll)
	ON_COMMAND(ID_VIEW_THREAD_THREAD0, OnViewThreadThread0)
	ON_UPDATE_COMMAND_UI(ID_VIEW_THREAD_THREAD0, OnUpdateViewThreadThread0)
	ON_COMMAND(ID_VIEW_THREAD_THREAD1, OnViewThreadThread1)
	ON_UPDATE_COMMAND_UI(ID_VIEW_THREAD_THREAD1, OnUpdateViewThreadThread1)
	ON_COMMAND(ID_VIEW_THREAD_THREAD2, OnViewThreadThread2)
	ON_UPDATE_COMMAND_UI(ID_VIEW_THREAD_THREAD2, OnUpdateViewThreadThread2)
	ON_COMMAND(ID_VIEW_THREAD_THREAD3, OnViewThreadThread3)
	ON_UPDATE_COMMAND_UI(ID_VIEW_THREAD_THREAD3, OnUpdateViewThreadThread3)
	ON_COMMAND(ID_VIEW_THREAD_THREAD4, OnViewThreadThread4)
	ON_UPDATE_COMMAND_UI(ID_VIEW_THREAD_THREAD4, OnUpdateViewThreadThread4)
	ON_COMMAND(ID_VIEW_THREAD_THREAD5, OnViewThreadThread5)
	ON_UPDATE_COMMAND_UI(ID_VIEW_THREAD_THREAD5, OnUpdateViewThreadThread5)
	ON_COMMAND(ID_VIEW_THREAD_THREAD6, OnViewThreadThread6)
	ON_UPDATE_COMMAND_UI(ID_VIEW_THREAD_THREAD6, OnUpdateViewThreadThread6)
	ON_COMMAND(ID_VIEW_THREAD_THREAD7, OnViewThreadThread7)
	ON_UPDATE_COMMAND_UI(ID_VIEW_THREAD_THREAD7, OnUpdateViewThreadThread7)
	ON_COMMAND(ID_ACTIONS_LOGDATA, OnActionsLogdata)
	ON_UPDATE_COMMAND_UI(ID_ACTIONS_LOGDATA, OnUpdateActionsLogdata)
	ON_COMMAND(ID_ACTIONS_UPDATEGRAPH, OnActionsUpdategraph)
	ON_UPDATE_COMMAND_UI(ID_ACTIONS_UPDATEGRAPH, OnUpdateActionsUpdategraph)
	//}}AFX_MSG_MAP
	ON_COMMAND(ID_COLLECTDATAFOR_THREAD8, &CIp3kprofDoc::OnCollectdataforThread8)
	ON_UPDATE_COMMAND_UI(ID_COLLECTDATAFOR_THREAD8, &CIp3kprofDoc::OnUpdateCollectdataforThread8)
	ON_COMMAND(ID_COLLECTDATAFOR_THREAD9, &CIp3kprofDoc::OnCollectdataforThread9)
	ON_UPDATE_COMMAND_UI(ID_COLLECTDATAFOR_THREAD9, &CIp3kprofDoc::OnUpdateCollectdataforThread9)
	ON_COMMAND(ID_COLLECTDATAFOR_THREAD10, &CIp3kprofDoc::OnCollectdataforThread10)
	ON_UPDATE_COMMAND_UI(ID_COLLECTDATAFOR_THREAD10, &CIp3kprofDoc::OnUpdateCollectdataforThread10)
	ON_COMMAND(ID_COLLECTDATAFOR_THREAD11, &CIp3kprofDoc::OnCollectdataforThread11)
	ON_UPDATE_COMMAND_UI(ID_COLLECTDATAFOR_THREAD11, &CIp3kprofDoc::OnUpdateCollectdataforThread11)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CIp3kprofDoc construction/destruction

CIp3kprofDoc::CIp3kprofDoc()
{
	sock = NULL;
	havedata = false;
	havedoc = false;
	log_rate = ((CIp3kprofApp *)AfxGetApp())->cmdInfo.log_seconds;
	pr.set_log(((CIp3kprofApp *)AfxGetApp())->cmdInfo.log_data, log_rate);
	current_tab = -1;
	closing = false;
}

CIp3kprofDoc::~CIp3kprofDoc()
{
	if (sock) {
		delete sock;
		sock = NULL;
	}

	if (linux_console) {
		delete linux_console;
		linux_console = NULL;
	}
}


BOOL CIp3kprofDoc::OnNewDocument()
{
	if (!CDocument::OnNewDocument()) {
		return FALSE;
	}

	pr.init_profiler(true, linux_console_send);

	return TRUE;
}


/////////////////////////////////////////////////////////////////////////////
// CIp3kprofDoc serialization

void CIp3kprofDoc::Serialize(CArchive& ar)
{
	if (ar.IsStoring())
	{
		// TODO: add storing code here
	}
	else
	{
		// TODO: add loading code here
	}
}

/////////////////////////////////////////////////////////////////////////////
// CIp3kprofDoc diagnostics

#ifdef _DEBUG
void CIp3kprofDoc::AssertValid() const
{
	CDocument::AssertValid();
}

void CIp3kprofDoc::Dump(CDumpContext& dc) const
{
	CDocument::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CIp3kprofDoc commands


void CIp3kprofDoc::OnActionsRestart() 
{
	linux_console_send("stop\n");
	delete sock;
	sock = NULL;
	delete linux_console;
	linux_console = new console(this);	
	if (linux_console == NULL) {
		AfxMessageBox("Can't create network console socket to connect to the board");
		return;
	}

	pr.init_profiler(true, linux_console_send);

	connect_board();
	UpdateAllViews(NULL);
}

/*
 * find file name using search path and if found put the full name in target
 *
 * search path is:
 *
 * name
 * path/name
 * distro/name
 * distro/path/name
 * lib/name
 * distro/lib/name
 * distro/uclic/lib/name
 * distro/user/name/name
 * distro/user/name
 *
 * return TRUE if found
 */
#define MAXTRIES 100
BOOL CIp3kprofDoc::find_elf_file(char *target, char *path, LPCTSTR name)
{
	struct _stat buf;
	CString tmp;
	CString tried[MAXTRIES];
	int numtries = 0;

	// name exists?
	tmp = name;
	if (_stat(tmp, &buf) == 0) {
		strcpy(target, tmp);
		return TRUE;
	}
	tried[numtries++] = tmp;

	// path\name exists?
	tmp = path;
	tmp += name;
	if (_stat(tmp, &buf) == 0) {
		strcpy(target, tmp);
		return TRUE;
	}
	tried[numtries++] = tmp;

	// try in distro\name
	tmp = distro_path + name;
	if (_stat(tmp, &buf) == 0) {
		strcpy(target, tmp);
		return TRUE;
	}
	tried[numtries++] = tmp;

	// try in distro\path\name
	tmp = distro_path + path + name;
	if (_stat(tmp, &buf) == 0) {
		strcpy(target, tmp);
		return TRUE;
	}
	tried[numtries++] = tmp;

	// try in (lib path)\name
	for (int i = 0; i < pr.get_lib_path_count(); ++i) {
		tmp = pr.get_lib_path(i);
		tmp += "\\";
		tmp += name;
		if (_stat(tmp, &buf) == 0) {
			strcpy(target, tmp);
			return TRUE;
		}
		tried[numtries++] = tmp;
	}

	// try in user\name
	tmp = distro_path + "user\\";
	tmp += name;
	tmp += "\\";
	tmp += name;
	if (_stat(tmp, &buf) == 0) {
		strcpy(target, tmp);
		return TRUE;
	}
	tried[numtries++] = tmp;

	// try in user
	tmp = distro_path + "user\\";
	tmp += name;
	if (_stat(tmp, &buf) == 0) {
		strcpy(target, tmp);
		return TRUE;
	}
	tried[numtries++] = tmp;

	tmp = "Can't find file: ";
	tmp += name;
	tmp += ".  Looked at:\n";
	for (int i = 0; i < numtries; ++i) {
		tmp += tried[i];
		tmp += "\n";
	}
	AfxMessageBox(tmp);
	return FALSE;
}

#define D_NONE 0
#define D_UCLINUX 1
#define D_OPENWRT 2
#define D_ANDROID 3


BOOL CIp3kprofDoc::OnOpenDocument(LPCTSTR lpszPath)
{
	bool autoload = theApp.cmdInfo.autoload;
	int distro_arch = CHIP_7K;
	char name[MAX_FILE_NAME];
	if (sock != NULL) {
		delete sock;
		sock = NULL;
	}

	POSITION pos;
	pos = GetFirstViewPosition();
	CIp3kprofView * view = (CIp3kprofView *)GetNextView(pos);

	distro_path = theApp.cmdInfo.distro_path;
	int distro = D_NONE;
	if (distro_path.Find("uClinux") != -1)
		distro = D_UCLINUX;
	else if (distro_path.Find("openwrt") != -1)
		distro = D_OPENWRT;
	else if (distro_path.Find("android") != -1)
		distro = D_ANDROID;
	else {
		distro = D_NONE;
	}
	
	pr.clear_elf_files();
	if (strstr(lpszPath, ".pgr") != NULL) {
		pr.init_profiler(false, NULL);
		if (!pr.read_graph(lpszPath)) {
			AfxMessageBox("Can't open graph file (or wrong version)");
			return FALSE;
		}
		SetTitle(lpszPath);

	} else {
		if (!((CIp3kprofApp *)AfxGetApp())->cmdInfo.have_address) {
			AfxMessageBox("Please use -ip:ip-address to specify the ip address of the board to profile.");
			return FALSE;
		}
		// lib path is either absolute or relative to specified distro
		// user specified lib paths come before defaults
		for (int i = 0; i < theApp.cmdInfo.next_lib_path; ++i) {
			if (theApp.cmdInfo.lib_path[i][0] == '\\' ||
				theApp.cmdInfo.lib_path[i][1] == ':') {
				pr.add_lib_path(theApp.cmdInfo.lib_path[i]);
			} else {
				strcpy(name, theApp.cmdInfo.distro_path);
				strcat(name, theApp.cmdInfo.lib_path[i]);
				pr.add_lib_path(name);
			}
		}

		// default paths
		if (distro == D_UCLINUX) {
			strcpy(name, theApp.cmdInfo.distro_path);
			strcat(name, "uClibc\\lib\\");
			pr.add_lib_path(name);
		} else if (distro == D_ANDROID) {
			// figure out our arch
			struct _stat stat_buf;
			strcpy(name, theApp.cmdInfo.distro_path);
			strcat(name, "out\\target\\product\\IP8K\\vmlinux");
			if (_stat(name, &stat_buf) == 0) {
				distro_arch = CHIP_8K;
			}

			strcpy(name, theApp.cmdInfo.distro_path);
			if (distro_arch == CHIP_7K) {
				strcat(name, "out\\target\\product\\IP7K\\symbols\\system\\lib\\");
			} else {
				strcat(name, "out\\target\\product\\IP8K\\symbols\\system\\lib\\");
			}
			pr.add_lib_path(name);
			strcpy(name, theApp.cmdInfo.distro_path);
			if (distro_arch == CHIP_7K) {
				strcat(name, "out\\target\\product\\IP7K\\symbols\\system\\bin\\");
			} else {
				strcat(name, "out\\target\\product\\IP8K\\symbols\\system\\bin\\");
			}
			pr.add_lib_path(name);
		} else if (distro == D_OPENWRT) {
			strcpy(name, theApp.cmdInfo.distro_path);
			strcat(name, "staging_dir\\target-ubicom32-unknown-linux-gnu\\root-ubicom32\\lib\\");
			pr.add_lib_path(name);

			strcpy(name, theApp.cmdInfo.distro_path);
			strcat(name, "staging_dir\\target-ubicom32-unknown-linux-gnu\\root-ubicom32\\usr\\lib\\");
			pr.add_lib_path(name);

			strcpy(name, theApp.cmdInfo.distro_path);
			strcat(name, "staging_dir\\target-ubicom32-unknown-linux-gnu\\root-ubicom32\\bin\\");
			pr.add_lib_path(name);

			strcpy(name, theApp.cmdInfo.distro_path);
			strcat(name, "staging_dir\\target-ubicom32-unknown-linux-gnu\\root-ubicom32\\sbin\\");
			pr.add_lib_path(name);

			strcpy(name, theApp.cmdInfo.distro_path);
			strcat(name, "staging_dir\\target-ubicom32-unknown-linux-gnu\\root-ubicom32\\usr\\bin\\");
			pr.add_lib_path(name);

			strcpy(name, theApp.cmdInfo.distro_path);
			strcat(name, "staging_dir\\target-ubicom32-unknown-linux-gnu\\root-ubicom32\\usr\\sbin\\");
			pr.add_lib_path(name);

			strcpy(name, theApp.cmdInfo.distro_path);
			strcat(name, "staging_dir\\target-ubicom32-unknown-linux-gnu\\root-ubicom32\\lib\\modules\\2.6.28.10\\");
			pr.add_lib_path(name);
			strcpy(name, theApp.cmdInfo.distro_path);

			strcat(name, "build_dir\\target-ubicom32-unknown-linux-gnu\\");
			pr.add_lib_path(name);
		}

		theApp.cmdInfo.m_strFileName;


		if (autoload) {
			// set up ultra file name
			if (distro == D_NONE) {
				if (!find_elf_file(name, "ultra\\projects\\mainexec\\", "ultra.elf")) {
					return FALSE;
				}
				pr.add_elf_file(name);
			} else  {
				if (!find_elf_file(name, "..\\ultra\\projects\\mainexec\\", "ultra.elf")) {
					return FALSE;
				}
				pr.add_elf_file(name);
			}
			if (distro == D_UCLINUX) {
				if (find_elf_file(name, "..\\Linux-2.6.x\\", "vmlinux")) {
					pr.add_elf_file(name);
				}
				if (find_elf_file(name, "user\\busybox\\", "busybox_unstripped")) {
					pr.add_elf_file(name);
				}
				if (find_elf_file(name, "..\\uClibc\\lib\\", "libuClibc-0.9.30.1.so")) {
					pr.add_elf_file(name);
				}
				if (find_elf_file(name, "..\\uClibc\\lib\\", "ld-uClibc-0.9.30.1.so")) {
					pr.add_elf_file(name);
				}
			} else if (distro == D_OPENWRT) {
				if (find_elf_file(name, "..\\Linux-2.6.x\\", "vmlinux")) {
					pr.add_elf_file(name);
				}
				if (find_elf_file(name, "", "busybox")) {
					pr.add_elf_file(name);
				}
				if (find_elf_file(name, "", "libuClibc-0.9.30.1.so")) {
					pr.add_elf_file(name);
				}
				if (find_elf_file(name, "", "ld-uClibc-0.9.30.1.so")) {
					pr.add_elf_file(name);
				}
				if (find_elf_file(name, "", "profilerd")) {
					pr.add_elf_file(name);
				}
			} else if (distro == D_ANDROID) {
				if (distro_arch == CHIP_7K && find_elf_file(name, "out\\target\\product\\IP7K\\", "vmlinux")) {
					pr.add_elf_file(name);
				}
				if (distro_arch == CHIP_8K && find_elf_file(name, "out\\target\\product\\IP8K\\", "vmlinux")) {
					pr.add_elf_file(name);
				}
				if (find_elf_file(name, "", "libdvm.so")) {
					pr.add_elf_file(name);
				}
				if (find_elf_file(name, "", "libc.so")) {
					pr.add_elf_file(name);
				}
				if (find_elf_file(name, "", "libstdc++.so")) {
					pr.add_elf_file(name);
				}
				if (find_elf_file(name, "", "libpixelflinger.so")) {
					pr.add_elf_file(name);
				}
				if (find_elf_file(name, "", "libsurfaceflinger.so")) {
					pr.add_elf_file(name);
				}
				if (find_elf_file(name, "", "zygote_listener")) {
					pr.add_elf_file(name);
				}
				if (find_elf_file(name, "", "servicemanager")) {
					pr.add_elf_file(name);
				}
			} else if (distro == D_NONE) {
				if (find_elf_file(name, "linux-2.6.x\\", "vmlinux")) {
					pr.add_elf_file(name);
				}
				if (find_elf_file(name, "uClinux\\user\\busybox\\", "busybox_unstripped")) {
					pr.add_elf_file(name);
				}
				if (find_elf_file(name, "uClibc\\lib\\", "libuClibc-0.9.30.1.so")) {
					pr.add_elf_file(name);
				}
				if (find_elf_file(name, "uClibc\\lib\\", "ld-uClibc-0.9.30.1.so")) {
					pr.add_elf_file(name);
				}
			}
			for (int i = 0; i < theApp.cmdInfo.next_name; ++i) {
				if (find_elf_file(name, "", theApp.cmdInfo.names[i])) {
					pr.add_elf_file(name);
				}
			}
		} else {
			if (theApp.cmdInfo.next_name < 2) {
				AfxMessageBox("Please specify the mainexec ultra.elf file and vmlinux file, or use -a");
				return FALSE;
			}
			// set up ultra and vmlinux file names
			if (distro == D_NONE) {
				if (!find_elf_file(name, "ultra\\projects\\mainexec\\", theApp.cmdInfo.names[0])) {
					return FALSE;
				}
				pr.add_elf_file(name);
				if (!find_elf_file(name, "linux-2.6.x\\", theApp.cmdInfo.names[1])) {
					return FALSE;
				}
				pr.add_elf_file(name);
			} else  {
				if (!find_elf_file(name, "..\\ultra\\projects\\mainexec\\", theApp.cmdInfo.names[0])) {
					return FALSE;
				}
				pr.add_elf_file(name);
				if (!find_elf_file(name, "..\\linux-2.6.x\\", theApp.cmdInfo.names[1])) {
					return FALSE;
				}
				pr.add_elf_file(name);
			}
			for (int i = 2; i < theApp.cmdInfo.next_name; ++i) {
				if (find_elf_file(name, "", theApp.cmdInfo.names[i])) {
					pr.add_elf_file(name);
				}
			}
		}

		/*
		 * create a socket to communicate with the board
		 */
		if (linux_console == NULL) {
			linux_console = new console(this);	
			if (linux_console == NULL) {
				AfxMessageBox("Can't create network console socket to connect to the board");
				return FALSE;
			}
		}

		pr.init_profiler(true, linux_console_send);

		if(!connect_board()) {
			return FALSE;
		}
	}

	view->starttimer();

	havedoc = true;
	return TRUE;
}

BOOL CIp3kprofDoc::connect_board(void)
{
	/*
	 * create a socket to receive UDP packets
	 */
	sock = new mysocket(this);
	if (sock == NULL) {
		AfxMessageBox("Can't open network socket");
		return FALSE;
	}
	if (!sock->Create(PROFILE_PORT, SOCK_DGRAM)) {
		AfxMessageBox("Can't create network socket.  Is another copy of the profiler tool running?");
		return FALSE;
	}

	/*
	 * need huge receive buffer to avoid losing packets during initialization
	 */
	int size;
	int len = sizeof(size);
	if (sock->GetSockOpt(SO_RCVBUF, &size, &len) < 0) {
		AfxMessageBox("Can't get receive socket size.");
		return FALSE;
	}
	if (size < 32768 * 16) {
		size = 32768 * 16;
	}
	if (sock->SetSockOpt(SO_RCVBUF, &size, sizeof(size)) < 0) {
		AfxMessageBox("Can't set receive socket size.");
		return FALSE;
	}

	/* 
	 * connect to the board now that all is ready
	 */
	if (!linux_console->connect(((CIp3kprofApp *)AfxGetApp())->cmdInfo.ipaddress)) {
		AfxMessageBox("Can't connect to the board");
		return FALSE;
	}
	return TRUE;
}

void CIp3kprofDoc::OnViewThreadAll() 
{
	pr.data_thread = -1;	
	OnActionsRestart();
}

void CIp3kprofDoc::OnUpdateViewThreadAll(CCmdUI* pCmdUI) 
{
	pCmdUI->SetCheck(pr.data_thread == -1);	
}

void CIp3kprofDoc::OnViewThreadThread0() 
{
	pr.data_thread = 0;	
	OnActionsRestart();
}

void CIp3kprofDoc::OnUpdateViewThreadThread0(CCmdUI* pCmdUI) 
{
	pCmdUI->SetCheck(pr.data_thread == 0);	
	pCmdUI->Enable(pr.type[0] == T_NRT || pr.type[0] == T_HIGH);
}

void CIp3kprofDoc::OnViewThreadThread1() 
{
	pr.data_thread = 1;	
	OnActionsRestart();
}

void CIp3kprofDoc::OnUpdateViewThreadThread1(CCmdUI* pCmdUI) 
{
	pCmdUI->SetCheck(pr.data_thread == 1);	
	pCmdUI->Enable(pr.type[1] == T_NRT || pr.type[1] == T_HIGH);
}

void CIp3kprofDoc::OnViewThreadThread2() 
{
	pr.data_thread = 2;	
	OnActionsRestart();
}

void CIp3kprofDoc::OnUpdateViewThreadThread2(CCmdUI* pCmdUI) 
{
	pCmdUI->SetCheck(pr.data_thread == 2);	
	pCmdUI->Enable(pr.type[2] == T_NRT || pr.type[2] == T_HIGH);
}

void CIp3kprofDoc::OnViewThreadThread3() 
{
	pr.data_thread = 3;	
	OnActionsRestart();
}

void CIp3kprofDoc::OnUpdateViewThreadThread3(CCmdUI* pCmdUI) 
{
	pCmdUI->SetCheck(pr.data_thread == 3);	
	pCmdUI->Enable(pr.type[3] == T_NRT || pr.type[3] == T_HIGH);
}

void CIp3kprofDoc::OnViewThreadThread4() 
{
	pr.data_thread = 4;	
	OnActionsRestart();
}

void CIp3kprofDoc::OnUpdateViewThreadThread4(CCmdUI* pCmdUI) 
{
	pCmdUI->SetCheck(pr.data_thread == 4);	
	pCmdUI->Enable(pr.type[4] == T_NRT || pr.type[4] == T_HIGH);
}

void CIp3kprofDoc::OnViewThreadThread5() 
{
	pr.data_thread = 5;	
	OnActionsRestart();
}

void CIp3kprofDoc::OnUpdateViewThreadThread5(CCmdUI* pCmdUI) 
{
	pCmdUI->SetCheck(pr.data_thread == 5);	
	pCmdUI->Enable(pr.type[5] == T_NRT || pr.type[5] == T_HIGH);
}

void CIp3kprofDoc::OnViewThreadThread6() 
{
	pr.data_thread = 6;	
	OnActionsRestart();
}

void CIp3kprofDoc::OnUpdateViewThreadThread6(CCmdUI* pCmdUI) 
{
	pCmdUI->SetCheck(pr.data_thread == 6);	
	pCmdUI->Enable(pr.type[6] == T_NRT || pr.type[6] == T_HIGH);
}

void CIp3kprofDoc::OnViewThreadThread7() 
{
	pr.data_thread = 7;	
	OnActionsRestart();
}

void CIp3kprofDoc::OnUpdateViewThreadThread7(CCmdUI* pCmdUI) 
{
	pCmdUI->SetCheck(pr.data_thread == 7);	
	pCmdUI->Enable(pr.type[7] == T_NRT || pr.type[7] == T_HIGH);
}

extern char *pagenames[MAXPAGES];

BOOL CIp3kprofDoc::OnSaveDocument(LPCTSTR lpszPathName) 
{
	CString name, covname;
	if (lpszPathName == NULL) {
		name = pr.get_elf_file(0);
	} else {
		name = lpszPathName;
	}
	// force .txt extension
	int loc = name.ReverseFind('.');
	int slash = name.ReverseFind('\\');
	if (slash == -1)
		slash = name.ReverseFind('/');
	if (loc == -1 || slash > loc) {
		name += ".txt";
	} else { 
		name = name.Left(loc) + ".txt";
	}
	FILE *out = fopen(name, "w");
	if (out == NULL) {
		CString msg;
		msg.Format("Can't open file %s for writing", name);
		AfxMessageBox(msg);
		return FALSE;
	}
	pr.create_display(((CIp3kprofApp *)AfxGetApp())->cmdInfo.btb,
		-1, SORT_TIME);
	for (int page = 0; page < MAXPAGES; ++page) {
		fprintf(out, "\n\n%s page:\n\n", pagenames[page]);
		for (int i = 0; i < pr.lastline[page]; ++i) {
			fputs(pr.lines[page][i], out);
			fputs("\n", out);
		}
	}
	fclose(out);

	name.Replace(".txt", ".pgr");
	if (!pr.save_graph(name)) {
		CString msg;
		msg.Format("Can't open file %s for writing", name);
		AfxMessageBox(msg);
	}

	name.Replace(".pgr", "-hazards.txt");
	if (!pr.save_hazards(1000, name, 0)) {
		CString msg;
		msg.Format("Can't open file %s for writing", name);
		AfxMessageBox(msg);
	}

	SetModifiedFlag(FALSE);

	return TRUE;
}

void CIp3kprofDoc::OnConsoleConnect()
{
	pr.on_connect();
}

void CIp3kprofDoc::OnConsoleReceive()
{
	char buf[4096];
	int res = linux_console->Receive(buf, 4000);
	if (res == 0) {	// socket was closed
		AfxMessageBox("Control socket was closed");
		return;
	}
	if (res == SOCKET_ERROR) {
		AfxMessageBox("Error on board network receive data");
		return;
	}
	buf[res] = 0;
	CString address;
	unsigned int port;
	linux_console->GetSockName(address, port);
	pr.on_receive(buf, address);
}

void CIp3kprofDoc::OnPacket(char *buf, unsigned int size)  	// a packet has arrived from the board
{
	if (closing)
		return;
	pr.on_packet(buf, size);
	SetModifiedFlag();
}


void CIp3kprofDoc::OnActionsLogdata() 
{
	pr.set_log(!pr.get_log(), log_rate);	
}

void CIp3kprofDoc::OnUpdateActionsLogdata(CCmdUI* pCmdUI) 
{
	pCmdUI->SetCheck(pr.get_log());	
}

void CIp3kprofDoc::OnActionsUpdategraph() 
{
	if (pr.get_elf_file_count() == 0) {
		return;
	}
	if (pr.update) {	// one final update all 
		pr.create_display(((CIp3kprofApp *)AfxGetApp())->cmdInfo.btb,
		-1, SORT_TIME);
	}
	pr.update = !pr.update;	
}

void CIp3kprofDoc::OnUpdateActionsUpdategraph(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(pr.get_elf_file_count() != 0);
	pCmdUI->SetCheck(pr.update);
}

void CIp3kprofDoc::OnCollectdataforThread8()
{
	pr.data_thread = 8;	
	OnActionsRestart();
}

void CIp3kprofDoc::OnUpdateCollectdataforThread8(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck(pr.data_thread == 8);	
	pCmdUI->Enable(pr.type[8] == T_NRT || pr.type[8] == T_HIGH);
}

void CIp3kprofDoc::OnCollectdataforThread9()
{
	pr.data_thread = 9;	
	OnActionsRestart();
}

void CIp3kprofDoc::OnUpdateCollectdataforThread9(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck(pr.data_thread == 9);	
	pCmdUI->Enable(pr.type[9] == T_NRT || pr.type[9] == T_HIGH);
}

void CIp3kprofDoc::OnCollectdataforThread10()
{
	pr.data_thread = 10;	
	OnActionsRestart();
}

void CIp3kprofDoc::OnUpdateCollectdataforThread10(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck(pr.data_thread == 10);	
	pCmdUI->Enable(pr.type[10] == T_NRT || pr.type[10] == T_HIGH);
}

void CIp3kprofDoc::OnCollectdataforThread11()
{
	pr.data_thread = 11;	
	OnActionsRestart();
}

void CIp3kprofDoc::OnUpdateCollectdataforThread11(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck(pr.data_thread == 11);	
	pCmdUI->Enable(pr.type[11] == T_NRT || pr.type[11] == T_HIGH);
}

BOOL CIp3kprofDoc::SaveModified()
{
//	int timelimit = ((CIp3kprofApp *)AfxGetApp())->cmdInfo.timelimit;
//	if (timelimit != 0) {
//		if (IsModified()) {
//			DoFileSave();
//		}
//		return TRUE;		// no dialog to ask if saving is OK
//	}
	return CDocument::SaveModified();
}
