// ip3kprof.cpp : Defines the class behaviors for the application.
//

#include "stdafx.h"
#include "ip3kprof.h"
#include "mysocket.h"
#include "console.h"

#include "MainFrm.h"
#include "ip3kprofDoc.h"
#include "ip3kprofView.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CIp3kprofApp

BEGIN_MESSAGE_MAP(CIp3kprofApp, CWinApp)
	//{{AFX_MSG_MAP(CIp3kprofApp)
	ON_COMMAND(ID_APP_ABOUT, OnAppAbout)
	ON_COMMAND(ID_VIEW_ALL_LATENCY, OnViewAllLatency)
	ON_UPDATE_COMMAND_UI(ID_VIEW_ALL_LATENCY, OnUpdateViewAllLatency)
	//}}AFX_MSG_MAP
	// Standard file based document commands
	ON_COMMAND(ID_FILE_NEW, CWinApp::OnFileNew)
	ON_COMMAND(ID_FILE_OPEN, CWinApp::OnFileOpen)
	// Standard print setup command
	ON_COMMAND(ID_FILE_PRINT_SETUP, CWinApp::OnFilePrintSetup)
END_MESSAGE_MAP()

bool view_all_latency(void){
	return ((CIp3kprofApp *)AfxGetApp())->view_all_latency;
}

/////////////////////////////////////////////////////////////////////////////
// CIp3kprofApp construction

CIp3kprofApp::CIp3kprofApp()
{
	view_all_latency = false;
	// TODO: add construction code here,
	// Place all significant initialization in InitInstance
}

/////////////////////////////////////////////////////////////////////////////
// The one and only CIp3kprofApp object

CIp3kprofApp theApp;

/////////////////////////////////////////////////////////////////////////////
// CIp3kprofApp initialization

BOOL CIp3kprofApp::InitInstance()
{
	if (!AfxSocketInit())
	{
		AfxMessageBox("Socket init failed");
		return FALSE;
	}

	AfxEnableControlContainer();

	// Standard initialization
	// If you are not using these features and wish to reduce the size
	//  of your final executable, you should remove from the following
	//  the specific initialization routines you do not need.

#ifdef _AFXDLL
	Enable3dControls();			// Call this when using MFC in a shared DLL
#else
	Enable3dControlsStatic();	// Call this when linking to MFC statically
#endif

	// Change the registry key under which our settings are stored.
	SetRegistryKey(_T("Ubicom"));

	LoadStdProfileSettings();  // Load standard INI file options (including MRU)

	// Register the application's document templates.  Document templates
	//  serve as the connection between documents, frame windows and views.

	CSingleDocTemplate* pDocTemplate;
	pDocTemplate = new CSingleDocTemplate(
		IDR_MAINFRAME,
		RUNTIME_CLASS(CIp3kprofDoc),
		RUNTIME_CLASS(CMainFrame),       // main SDI frame window
		RUNTIME_CLASS(CIp3kprofView));
	AddDocTemplate(pDocTemplate);

	// Parse command line for standard shell commands, DDE, file open
	ParseCommandLine(cmdInfo);

	if (cmdInfo.m_nShellCommand != CCommandLineInfo::FileOpen) {
		if (!cmdInfo.autoload) {
			cmdInfo.usage();
			return FALSE;
		}
		cmdInfo.m_strFileName = "autoload";
		cmdInfo.m_nShellCommand = CCommandLineInfo::FileOpen;
	}

	// must be before ProcessShellCommand, whcih creates the frame window
	if (cmdInfo.no_gui && cmdInfo.timelimit != 0) {
		m_nCmdShow = SW_HIDE;
	} else {
		m_nCmdShow = SW_SHOWMAXIMIZED;
	}

	// Dispatch commands specified on the command line
	if (!ProcessShellCommand(cmdInfo))
		return FALSE;

	// The one and only window has been initialized, so show and update it.
	if (cmdInfo.no_gui && cmdInfo.timelimit == 0) {
		AfxMessageBox("Must use -t to set a time limit when turning off the gui with -no_gui");
	}
	m_pMainWnd->ShowWindow(m_nCmdShow);
	m_pMainWnd->UpdateWindow();

	return TRUE;
}


/////////////////////////////////////////////////////////////////////////////
// CAboutDlg dialog used for App About

class CAboutDlg : public CDialog
{
public:
	CAboutDlg();

// Dialog Data
	//{{AFX_DATA(CAboutDlg)
	enum { IDD = IDD_ABOUTBOX };
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAboutDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	//{{AFX_MSG(CAboutDlg)
		// No message handlers
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
	//{{AFX_DATA_INIT(CAboutDlg)
	//}}AFX_DATA_INIT
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAboutDlg)
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
	//{{AFX_MSG_MAP(CAboutDlg)
		// No message handlers
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

// App command to run the dialog
void CIp3kprofApp::OnAppAbout()
{
	CAboutDlg aboutDlg;
	aboutDlg.DoModal();
}

/////////////////////////////////////////////////////////////////////////////
// CIp3kprofApp message handlers


void CIp3kprofApp::OnViewAllLatency() 
{
	view_all_latency = !view_all_latency;	
}

void CIp3kprofApp::OnUpdateViewAllLatency(CCmdUI* pCmdUI) 
{
	pCmdUI->SetCheck(view_all_latency);
}
