#pragma once
#include "afxdialogex.h"
#include "ClientController.h"

#define WM_DOWNLOAD_DLG_READY_TO_DESTROY (WM_USER + 120)

// CDownloadDialog 对话框

class CDownloadDialog : public CDialogEx {
	DECLARE_DYNAMIC(CDownloadDialog)

public:
	CDownloadDialog(CWnd* pParent);   // 标准构造函数
	virtual ~CDownloadDialog();

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DOWNLOAD_DIALOG };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持
	afx_msg LRESULT OnInvalidFile(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnFileSize(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnDataReceived(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnDownloadComplete(WPARAM wParar, LPARAM lParam);

	DECLARE_MESSAGE_MAP()

	CWnd* pParent;
	long long file_size;
	long long data_received;

public:
	CProgressCtrl downloadProgress;
	virtual BOOL OnInitDialog();
	CButton btnCancel;
	virtual void OnCancel();
	afx_msg void OnBnClickedDownloadCancel();
};
