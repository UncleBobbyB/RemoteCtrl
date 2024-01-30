
// RemoteClientDlg.cpp: 实现文件
//
#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#include "pch.h"
#include "framework.h"
#include "RemoteClientDlg.h"
#include "ClientController.h"
#include "afxdialogex.h"
#include "resource.h"
#include "RemoteClient.h"


// 用于应用程序“关于”菜单项的 CAboutDlg 对话框

class CAboutDlg : public CDialogEx {
public:
	CAboutDlg();

	// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_ABOUTBOX };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	// 实现
protected:
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnFile();
};

CAboutDlg::CAboutDlg() : CDialogEx(IDD_ABOUTBOX) {
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX) {
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// CRemoteClientDlg 对话框

CRemoteClientDlg::CRemoteClientDlg(CWnd* pParent, CString strIP, UINT nPort)
	: CDialogEx(IDD_REMOTECLIENT_DIALOG, pParent), strIP(strIP), nPort(nPort) {
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CRemoteClientDlg::DoDataExchange(CDataExchange* pDX) {
	CDialogEx::DoDataExchange(pDX);
}

std::pair<double, double> CRemoteClientDlg::GetNormalizedMousePosition() {
	POINT pt;
	GetCursorPos(&pt); // Get global cursor position
	m_videoFrame.ScreenToClient(&pt);

	RECT rect;
	m_videoFrame.GetClientRect(&rect);
	
	double normalizedX = double(pt.x) / double(rect.right - rect.left);
	double normalizedY = double(pt.y) / double(rect.bottom - rect.top);

	return { normalizedX, normalizedY };
}

BEGIN_MESSAGE_MAP(CRemoteClientDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_WM_TIMER()
	ON_WM_MOUSEMOVE()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_LBUTTONDBLCLK()
	ON_WM_RBUTTONDOWN()
	ON_WM_RBUTTONUP()
	// ON_WM_CONTEXTMENU()
	ON_MESSAGE(WM_FRAME_DATA_AVAILABLE, CRemoteClientDlg::DisplayFrame)
	ON_COMMAND(ID_FILE, &CRemoteClientDlg::OnFile)
	ON_MESSAGE(WM_DIR_TREE_UPDATED, CRemoteClientDlg::OnDirTreeUpdated)
	ON_MESSAGE(WM_DIR_TREE_INVALID_DIR, CRemoteClientDlg::OnInvalidDir)
	ON_MESSAGE(WM_DOWNLOAD_INVALID_FILE, CRemoteClientDlg::OnInvalidFile)
	ON_MESSAGE(WM_DOWNLOAD_FILE_SIZE, CRemoteClientDlg::OnFileSize)
	ON_MESSAGE(WM_DOWNLOAD_COMPLETE, CRemoteClientDlg::OnDownloadComplete)
	ON_COMMAND(ID_RECONNECT, &CRemoteClientDlg::OnReconnect)
END_MESSAGE_MAP()


// CRemoteClientDlg 消息处理程序

UINT MouseEventWorkerThread(LPVOID pParam) {
	CRemoteClientDlg *thiz = static_cast<CRemoteClientDlg*>(pParam);
	MSG msg;
	while (GetMessage(&msg, nullptr, 0, 0)) {
		POINT point;
		point.x = GET_X_LPARAM(msg.lParam);
		point.y = GET_Y_LPARAM(msg.lParam);
		CRect rect;
		thiz->m_videoFrame.GetClientRect(&rect);
		thiz->m_videoFrame.ClientToScreen(&rect);
		thiz->ScreenToClient(&rect);
		if (!rect.PtInRect(point)) {
			continue;
		}

		switch (msg.message) {
		case WM_MOUSEMOVE:
			CClientController::SendMouseEvent(mouse_move, thiz->GetNormalizedMousePosition());
			break;
		case WM_LBUTTONDOWN:
			CClientController::SendMouseEvent(lb_down, thiz->GetNormalizedMousePosition());
			break;
		case WM_LBUTTONUP:
			CClientController::SendMouseEvent(lb_up, thiz->GetNormalizedMousePosition());
			break;
		case WM_LBUTTONDBLCLK:
			CClientController::SendMouseEvent(lb_2click, thiz->GetNormalizedMousePosition());
			break;
		case WM_RBUTTONDOWN:
			CClientController::SendMouseEvent(rb_down, thiz->GetNormalizedMousePosition());
			break;
		case WM_RBUTTONUP:
			CClientController::SendMouseEvent(rb_up, thiz->GetNormalizedMousePosition());
			break;
		default:
			TRACE("Unknown mouse event\n");
			break;
		}
			TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	return 0;
}

BOOL CRemoteClientDlg::OnInitDialog() {
	CDialogEx::OnInitDialog();

	// 将“关于...”菜单项添加到系统菜单中。

	// IDM_ABOUTBOX 必须在系统命令范围内。
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != nullptr) {
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty()) {
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// 设置此对话框的图标。  当应用程序主窗口不是对话框时，框架将自动
	//  执行此操作
	SetIcon(m_hIcon, TRUE);			// 设置大图标
	SetIcon(m_hIcon, FALSE);		// 设置小图标


	m_videoFrame.SubclassDlgItem(IDC_VIDEO_FRAME, this);

	pMouseEventThread = AfxBeginThread(MouseEventWorkerThread, this);
	MouseEventThreadID = pMouseEventThread->m_nThreadID;

	// 文件查看对话框初始化
	//m_fileInfoDlg.dirTree.SubclassDlgItem(IDC_TREE, this);
	//m_fileInfoDlg.fileList.SubclassDlgItem(IDC_LIST, this);
	assert(m_fileInfoDlg.Create(IDD_FILE_INFO_DIALOG, this));
	CNetController::set_pDirTree(&(m_fileInfoDlg.dirTree));
	CNetController::set_pFileTree(&(m_fileInfoDlg.fileList));
	CNetController::set_pLastItem(&(m_fileInfoDlg.lastItemSelected));

	// 控制器初始化
	CClientController::init(strIP, nPort);

	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

void CRemoteClientDlg::OnSysCommand(UINT nID, LPARAM lParam) {
	if ((nID & 0xFFF0) == IDM_ABOUTBOX) {
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	} else {
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// 如果向对话框添加最小化按钮，则需要下面的代码
//  来绘制该图标。  对于使用文档/视图模型的 MFC 应用程序，
//  这将由框架自动完成。

void CRemoteClientDlg::OnPaint() {
	if (IsIconic()) {
		CPaintDC dc(this); // 用于绘制的设备上下文

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 使图标在工作区矩形中居中
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// 绘制图标
		dc.DrawIcon(x, y, m_hIcon);
	} else {
		CDialogEx::OnPaint();
	}
}

//当用户拖动最小化窗口时系统调用此函数取得光标
//显示。
HCURSOR CRemoteClientDlg::OnQueryDragIcon() {
	return static_cast<HCURSOR>(m_hIcon);
}

LRESULT CRemoteClientDlg::DisplayFrame(WPARAM wParam, LPARAM lParam) {
	std::shared_ptr<CImage> pImage = CClientController::getFrame();
	CDC* pDC = m_videoFrame.GetDC();
	if (pDC == nullptr)
		throw std::runtime_error("video frame dc is null");

	CRect rect;
	m_videoFrame.GetClientRect(&rect);
	if (pImage != nullptr) // 拖拽窗口时有可能会使CImage丢失DC，因此需要额外判断
		pImage->Draw(pDC->m_hDC, rect);

	m_videoFrame.ReleaseDC(pDC);
	return 0;
}

LRESULT CRemoteClientDlg::OnDirTreeUpdated(WPARAM wParam, LPARAM lParam) {
	m_fileInfoDlg.Invalidate();
	m_fileInfoDlg.UpdateWindow();

	return 0;
}

LRESULT CRemoteClientDlg::OnInvalidDir(WPARAM wParam, LPARAM lParam) {
	if (m_fileInfoDlg.GetSafeHwnd())
		m_fileInfoDlg.PostMessage(WM_DIR_TREE_INVALID_DIR, wParam, lParam);

	return 0;
}

LRESULT CRemoteClientDlg::OnInvalidFile(WPARAM wParam, LPARAM lParam) {
	if (m_fileInfoDlg.GetSafeHwnd() && m_fileInfoDlg.pDownloadDlg != nullptr)
		m_fileInfoDlg.pDownloadDlg->PostMessage(WM_DOWNLOAD_INVALID_FILE, wParam, lParam);

	return 0;
}

LRESULT CRemoteClientDlg::OnFileSize(WPARAM wParam, LPARAM lParam) {
	if (m_fileInfoDlg.GetSafeHwnd() && m_fileInfoDlg.pDownloadDlg != nullptr)
		m_fileInfoDlg.pDownloadDlg->PostMessage(WM_DOWNLOAD_FILE_SIZE, wParam, lParam);

	return 0;
}

LRESULT CRemoteClientDlg::OnDownloadComplete(WPARAM wParam, LPARAM lParam) {
	if (m_fileInfoDlg.GetSafeHwnd() && m_fileInfoDlg.pDownloadDlg != nullptr)
		m_fileInfoDlg.pDownloadDlg->PostMessage(WM_DOWNLOAD_COMPLETE, wParam, lParam);

	return 0;
}

void CRemoteClientDlg::OnTimer(UINT_PTR nIDEvent) {
	switch (nIDEvent) {
	default:
		break;
	}

	CDialogEx::OnTimer(nIDEvent);
}


void CRemoteClientDlg::OnContextMenu(CWnd* pWnd, CPoint point) {
}

void CRemoteClientDlg::OnMouseMove(UINT nFlags, CPoint point) {
	// Pack the mouse coordinates into a WPARAM and LPARAM
	WPARAM wParam = 0;
	LPARAM lParam = MAKELPARAM(point.x, point.y);

	// Post the message to the worker thread
	PostThreadMessage(MouseEventThreadID, WM_MOUSEMOVE, wParam, lParam);

	CDialogEx::OnMouseMove(nFlags, point);
}

void CRemoteClientDlg::OnLButtonDown(UINT nFlags, CPoint point) {
	WPARAM wParam = 0;
	LPARAM lParam = MAKELPARAM(point.x, point.y);

	PostThreadMessage(MouseEventThreadID, WM_LBUTTONDOWN, wParam, lParam);

	CDialogEx::OnLButtonDown(nFlags, point); // Call default handler
}

void CRemoteClientDlg::OnLButtonUp(UINT nFlags, CPoint point) {
	WPARAM wParam = 0;
	LPARAM lParam = MAKELPARAM(point.x, point.y);

	PostThreadMessage(MouseEventThreadID, WM_LBUTTONUP, wParam, lParam);

	CDialogEx::OnLButtonUp(nFlags, point); // Call default handler
}

void CRemoteClientDlg::OnLButtonDblClk(UINT nFlags, CPoint point) {
	WPARAM wParam = 0;
	LPARAM lParam = MAKELPARAM(point.x, point.y);

	PostThreadMessage(MouseEventThreadID, WM_LBUTTONDBLCLK, wParam, lParam);

	CDialogEx::OnLButtonDblClk(nFlags, point);
}

void CRemoteClientDlg::OnRButtonDown(UINT nFlags, CPoint point) {
	WPARAM wParam = 0;
	LPARAM lParam = MAKELPARAM(point.x, point.y);

	PostThreadMessage(MouseEventThreadID, WM_RBUTTONDOWN, wParam, lParam);

	CDialogEx::OnRButtonDblClk(nFlags, point);
}

void CRemoteClientDlg::OnRButtonUp(UINT nFlags, CPoint point) {
	WPARAM wParam = 0;
	LPARAM lParam = MAKELPARAM(point.x, point.y);

	PostThreadMessage(MouseEventThreadID, WM_RBUTTONUP, wParam, lParam);

	CDialogEx::OnRButtonUp(nFlags, point);
}


void CRemoteClientDlg::OnFile() {
	// TODO: 在此添加命令处理程序代码

	m_fileInfoDlg.ShowWindow(SW_SHOW);
}


//void CRemoteClientDlg::PostNcDestroy() {
//	CDialog::PostNcDestroy();
//	delete this;
//	static_cast<CRemoteClientApp*>(AfxGetApp())->m_pClientDlg = nullptr;
//}


void CRemoteClientDlg::OnCancel() {
	DestroyWindow();
	CClientController::destroy();
	::PostQuitMessage(0);
}


void CRemoteClientDlg::OnReconnect() {
	AfxGetApp()->PostThreadMessage(WM_CLIENT_RESTART, 0, 0);
}
