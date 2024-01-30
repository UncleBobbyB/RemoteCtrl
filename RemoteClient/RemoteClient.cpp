
// RemoteClient.cpp: 定义应用程序的类行为。
//

#include "pch.h"
#include "framework.h"
#include "CStartupDlg.h"
#include "RemoteClient.h"
#include "RemoteClientDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CRemoteClientApp

BEGIN_MESSAGE_MAP(CRemoteClientApp, CWinApp)
	ON_COMMAND(ID_HELP, &CWinApp::OnHelp)
	ON_THREAD_MESSAGE(WM_CLIENT_RESTART, &CRemoteClientApp::OnClientRestart)
END_MESSAGE_MAP()


// CRemoteClientApp 构造

CRemoteClientApp::CRemoteClientApp() {
	// 支持重新启动管理器
	m_dwRestartManagerSupportFlags = AFX_RESTART_MANAGER_SUPPORT_RESTART;

	// TODO: 在此处添加构造代码，
	// 将所有重要的初始化放置在 InitInstance 中
}


// 唯一的 CRemoteClientApp 对象

CRemoteClientApp theApp;


// CRemoteClientApp 初始化

BOOL CRemoteClientApp::InitInstance() {
	// 如果一个运行在 Windows XP 上的应用程序清单指定要
	// 使用 ComCtl32.dll 版本 6 或更高版本来启用可视化方式，
	//则需要 InitCommonControlsEx()。  否则，将无法创建窗口。
	INITCOMMONCONTROLSEX InitCtrls;
	InitCtrls.dwSize = sizeof(InitCtrls);
	// 将它设置为包括所有要在应用程序中使用的
	// 公共控件类。
	InitCtrls.dwICC = ICC_WIN95_CLASSES;
	InitCommonControlsEx(&InitCtrls);

	CWinApp::InitInstance();


	AfxEnableControlContainer();

	// 创建 shell 管理器，以防对话框包含
	// 任何 shell 树视图控件或 shell 列表视图控件。
	CShellManager *pShellManager = new CShellManager;

	// 激活“Windows Native”视觉管理器，以便在 MFC 控件中启用主题
	CMFCVisualManager::SetDefaultManager(RUNTIME_CLASS(CMFCVisualManagerWindows));

	// 标准初始化
	// 如果未使用这些功能并希望减小
	// 最终可执行文件的大小，则应移除下列
	// 不需要的特定初始化例程
	// 更改用于存储设置的注册表项
	// TODO: 应适当修改该字符串，
	// 例如修改为公司或组织名
	SetRegistryKey(_T("应用程序向导生成的本地应用程序"));

	// enable restart
	RegisterApplicationRestart(L"/restart", 0);

	CString strIP;
	UINT nPort;
	bool ok = false;

	CStartupDlg startDlg(nullptr, strIP, nPort, ok);
	startDlg.DoModal();
	if (!ok)
		return TRUE;

	m_pClientDlg = new CRemoteClientDlg(nullptr, strIP, nPort);
	m_pMainWnd = m_pClientDlg;
	m_pClientDlg->Create(IDD_REMOTECLIENT_DIALOG);
	m_pClientDlg->ShowWindow(SW_SHOW);

	// 删除上面创建的 shell 管理器。
	if (pShellManager != nullptr) 
		delete pShellManager;

#if !defined(_AFXDLL) && !defined(_AFX_NO_MFC_CONTROLS_IN_DIALOGS)
	ControlBarCleanUp();
#endif

	// 由于对话框已关闭，所以将返回 FALSE 以便退出应用程序，
	//  而不是启动应用程序的消息泵。
	return TRUE;
}

int CRemoteClientApp::ExitInstance() {
	::ExitProcess(0);

	return CWinApp::ExitInstance();
}

void CRemoteClientApp::OnClientRestart(WPARAM wParam, LPARAM lParam) {
	// I have shiity codes containing static members that're impossible to clean & reset
	// so Imma just restarting the whole fucking app 
	// genius.

	// get the full path of the executable
	TCHAR szFileName[MAX_PATH];
	GetModuleFileName(NULL, szFileName, MAX_PATH);

	// set up the process and startup info
	STARTUPINFO si = { 0 };
	si.cb = sizeof(si);
	PROCESS_INFORMATION pi = { 0 };

	// start a new instance of the application
	if (CreateProcess(szFileName, NULL, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
		// close the current instance
		AfxGetMainWnd()->PostMessage(WM_CLOSE);
	}
}

