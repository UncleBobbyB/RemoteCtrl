// CDirInfoDlg.cpp: 实现文件
//

#include "pch.h"
#include "RemoteClient.h"
#include "afxdialogex.h"
#include "CDirInfoDlg.h"


// CDirInfoDlg 对话框

IMPLEMENT_DYNAMIC(CDirInfoDlg, CDialogEx)

CDirInfoDlg::CDirInfoDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_DIRONFIDIALOG, pParent) {

}

CDirInfoDlg::~CDirInfoDlg() {
}

void CDirInfoDlg::DoDataExchange(CDataExchange* pDX) {
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_TREE, file_info);
}


BEGIN_MESSAGE_MAP(CDirInfoDlg, CDialogEx)
END_MESSAGE_MAP()


// CDirInfoDlg 消息处理程序


BOOL CDirInfoDlg::OnInitDialog() {
	CDialogEx::OnInitDialog();

	// TODO:  在此添加额外的初始化
	file_info.SubclassDlgItem(IDC_FILE_INFO, this);

	return TRUE;  // return TRUE unless you set the focus to a control
}
