// CStartupDlg.cpp: 实现文件
//

#include "pch.h"
#include "RemoteClient.h"
#include "afxdialogex.h"
#include "CStartupDlg.h"


// CStartupDlg 对话框

IMPLEMENT_DYNAMIC(CStartupDlg, CDialogEx)

CStartupDlg::CStartupDlg(CWnd* pParent, CString& strIP, UINT& nPort, bool& ok)
	: CDialogEx(IDD_STARTUP_DIALOG, pParent), strIP_(strIP), nPort_(nPort), ok_(ok) {
}

CStartupDlg::~CStartupDlg() {
}

void CStartupDlg::DoDataExchange(CDataExchange* pDX) {
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_IPADDRESS1, m_addrEdit);
	DDX_Control(pDX, IDC_EDIT1, m_portEdit);
	DDX_Control(pDX, IDC_BUTTON1, m_btnOK);
}


BEGIN_MESSAGE_MAP(CStartupDlg, CDialogEx)
	ON_BN_CLICKED(IDC_BUTTON1, &CStartupDlg::OnBnClickedButton1)
END_MESSAGE_MAP()


// CStartupDlg 消息处理程序


void CStartupDlg::OnCancel() {
	CDialogEx::OnCancel();
}


BOOL CStartupDlg::OnInitDialog() {
	CDialogEx::OnInitDialog();

	CString strIP = _T("192.168.56.102");
	int b1, b2, b3, b4;
	_stscanf_s(strIP, _T("%d.%d.%d.%d"), &b1, &b2, &b3, &b4);
	m_addrEdit.SetAddress(b1, b2, b3, b4);

	m_portEdit.SetWindowText("9527");

	return TRUE;  // return TRUE unless you set the focus to a control
	// 异常: OCX 属性页应返回 FALSE
}


void CStartupDlg::OnBnClickedButton1() {
	DWORD dIP;
	m_addrEdit.GetAddress(dIP);
	unsigned char bytes[4];
	for (int i = 0; i < 4; i++)
		bytes[i] = (dIP >> (i * 8)) & 0xFF;
	strIP_.Format(("%d.%d.%d.%d"), bytes[3], bytes[2], bytes[1], bytes[0]);

	CString strPort;
	m_portEdit.GetWindowText(strPort);
	nPort_ = _ttoi(strPort);

	ok_ = true;

	OnCancel();
}
