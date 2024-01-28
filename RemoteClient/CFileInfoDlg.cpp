// CFileInfoDlg.cpp: 实现文件
//

#include "pch.h"
#include "CFileInfoDlg.h"


// CFileInfoDlg 对话框

IMPLEMENT_DYNAMIC(CFileInfoDlg, CDialogEx)

CFileInfoDlg::CFileInfoDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_FILE_INFO_DIALOG, pParent) {

}

CFileInfoDlg::~CFileInfoDlg() {
}

void CFileInfoDlg::DoDataExchange(CDataExchange* pDX) {
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_TREE, dirTree);
	DDX_Control(pDX, IDC_LIST, fileList);
}

void CFileInfoDlg::OnCancel() {
	ShowWindow(SW_HIDE);
}

LRESULT CFileInfoDlg::OnInvalidDir(WPARAM wParam, LPARAM lParam) {
	TRACE("Invalid Dir\n");
	fileList.DeleteAllItems();
	//CClientController::requestDriveInfo();

	return 0;
}

LRESULT CFileInfoDlg::OnFileListUpdated(WPARAM wParam, LPARAM lParam) {
	return LRESULT();
}

BEGIN_MESSAGE_MAP(CFileInfoDlg, CDialogEx)
	ON_NOTIFY(NM_DBLCLK, IDC_TREE, &CFileInfoDlg::OnNMDblclkTree)
	ON_MESSAGE(WM_DIR_TREE_INVALID_DIR, CFileInfoDlg::OnInvalidDir)
	ON_NOTIFY(NM_CLICK, IDC_TREE, &CFileInfoDlg::OnNMClickTree)
	ON_NOTIFY(NM_RCLICK, IDC_LIST, &CFileInfoDlg::OnNMRClickList)
	ON_COMMAND(ID_DOWNLOAD, &CFileInfoDlg::OnDownload)
END_MESSAGE_MAP()


CString CFileInfoDlg::GetPath(HTREEITEM hTree) {
	CString strRet, strTmp;
	do {
		strTmp = dirTree.GetItemText(hTree);
		strRet = strTmp + '\\' + strRet;
		hTree = dirTree.GetParentItem(hTree);
	} while (hTree != NULL);

	if (!strRet.IsEmpty()) {
		assert(strRet[strRet.GetLength() - 1] == '\\');
		strRet.Delete(strRet.GetLength() - 1, 1);
	}

	return strRet;
}

void CFileInfoDlg::UpdateFileInfo() {
	CPoint ptMouse;
	GetCursorPos(&ptMouse);
	dirTree.ScreenToClient(&ptMouse);
	HTREEITEM hTreeSelected = dirTree.HitTest(ptMouse, 0);
	if (hTreeSelected == NULL)
		return;
	lastItemSelected = hTreeSelected;
	CString strPath = GetPath(hTreeSelected);
	TRACE("%s\n", strPath);
	CClientController::requestDirInfo(strPath);
}

// CFileInfoDlg 消息处理程序

void CFileInfoDlg::OnNMDblclkTree(NMHDR* pNMHDR, LRESULT* pResult) {
	*pResult = 0;
	UpdateFileInfo();
}


void CFileInfoDlg::OnNMClickTree(NMHDR* pNMHDR, LRESULT* pResult) {
	*pResult = 0;
	UpdateFileInfo();
}


void CFileInfoDlg::OnNMRClickList(NMHDR* pNMHDR, LRESULT* pResult) {
	LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
	// TODO: 在此添加控件通知处理程序代码
	*pResult = 0;
	CPoint ptMouse, ptList;
	GetCursorPos(&ptMouse);
	ptList = ptMouse;
	fileList.ScreenToClient(&ptList);
	if (fileList.HitTest(ptList) < 0)
		return;
	CMenu menu;
	menu.LoadMenu(IDR_MENU_FILE);
	CMenu *pPopup = menu.GetSubMenu(0);
	if (pPopup == NULL)
		return;
	pPopup->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, ptMouse.x, ptMouse.y, this);
}


void CFileInfoDlg::OnDownload() {
	int nListSelected = fileList.GetSelectionMark();
	CString fileName = fileList.GetItemText(nListSelected, 0);
	HTREEITEM ItemSelected = dirTree.GetSelectedItem();
	assert(ItemSelected);
	CString path = GetPath(ItemSelected);
	path += '\\';
	path += fileName;
	TRACE("%s\n", path);
	// TODO: send cmd
}
