// CDownloadDialog.cpp: 实现文件
//

#include "pch.h"
#include "RemoteClient.h"
#include "afxdialogex.h"
#include "CDownloadDialog.h"


// CDownloadDialog 对话框

IMPLEMENT_DYNAMIC(CDownloadDialog, CDialogEx)

CDownloadDialog::CDownloadDialog(CWnd* pParent)
	: CDialogEx(IDD_DOWNLOAD_DIALOG, pParent), pParent(pParent), file_size(-1), data_received(0) {
}

CDownloadDialog::~CDownloadDialog() {
}

void CDownloadDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_DOWNLOAD_PROGRESS, downloadProgress);
	DDX_Control(pDX, ID_DOWNLOAD_CANCEL, btnCancel);
}

BEGIN_MESSAGE_MAP(CDownloadDialog, CDialogEx)
	ON_BN_CLICKED(ID_DOWNLOAD_CANCEL, CDownloadDialog::OnBnClickedDownloadCancel)
	ON_MESSAGE(WM_DOWNLOAD_INVALID_FILE, CDownloadDialog::OnInvalidFile)
	ON_MESSAGE(WM_DOWNLOAD_FILE_SIZE, CDownloadDialog::OnFileSize)
	ON_MESSAGE(WM_DOWNLOAD_DATA_RECEIVED, CDownloadDialog::OnDataReceived)
	ON_MESSAGE(WM_DOWNLOAD_COMPLETE, CDownloadDialog::OnDownloadComplete)
END_MESSAGE_MAP()


LRESULT CDownloadDialog::OnInvalidFile(WPARAM wParam, LPARAM lParam) {
	AfxMessageBox("无法打开文件或者目标文件不存在", MB_OK);
	OnCancel();

	return 0;
}

LRESULT CDownloadDialog::OnFileSize(WPARAM wParam, LPARAM lParam) {
	assert(file_size == -1);
	auto pFileSize = reinterpret_cast<long long*>(lParam);
	TRACE("download file size: %lld\n", file_size);
	if (*pFileSize == 0) {
		// 文件为空
		TRACE("target file is empty\n");
		delete pFileSize;
		OnCancel();
	}

	delete pFileSize;
	return 0;
}

LRESULT CDownloadDialog::OnDataReceived(WPARAM wParam, LPARAM lParam) {
	auto pDataReceived = reinterpret_cast<size_t*>(lParam);
	TRACE("file data received: %d\n", *pDataReceived);
	data_received += *pDataReceived;
	assert(file_size);
	downloadProgress.SetPos(1.0 * data_received / file_size);

	delete pDataReceived;
	return 0;
}

LRESULT CDownloadDialog::OnDownloadComplete(WPARAM wParar, LPARAM lParam) {
	AfxMessageBox("文件下载完成", MB_OK);
	pParent->PostMessage(WM_DOWNLOAD_DLG_READY_TO_DESTROY);

	return 0;
}

// CDownloadDialog 消息处理程序


BOOL CDownloadDialog::OnInitDialog() {
	CDialogEx::OnInitDialog();

	downloadProgress.SetRange(0, 100);
	downloadProgress.SetStep(1);

	return TRUE;  // return TRUE unless you set the focus to a control
	// 异常: OCX 属性页应返回 FALSE
}


void CDownloadDialog::OnCancel() {
	pParent->PostMessage(WM_DOWNLOAD_ABORT);
	pParent->PostMessage(WM_DOWNLOAD_DLG_READY_TO_DESTROY);

	CDialogEx::OnCancel();
}


void CDownloadDialog::OnBnClickedDownloadCancel() {
	OnCancel();
}
