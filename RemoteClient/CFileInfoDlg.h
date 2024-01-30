#pragma once
#include "afxdialogex.h"
#include "CDownloadDialog.h"



// CFileInfoDlg 对话框

class CFileInfoDlg : public CDialogEx {
	DECLARE_DYNAMIC(CFileInfoDlg)

public:
	CFileInfoDlg(CWnd* pParent = nullptr);   // 标准构造函数
	virtual ~CFileInfoDlg();

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_FILE_INFO_DIALOG };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持
	virtual void OnCancel();
	afx_msg LRESULT OnInvalidDir(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnFileListUpdated(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnDownloadAbort(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnDownloadDlgDestroy(WPARAM wParam, LPARAM lParam);
	DECLARE_MESSAGE_MAP()
	virtual LRESULT WindowProc(UINT message, WPARAM wParam, LPARAM lParam);


	CString GetPath(HTREEITEM hTree);
	void UpdateFileInfo();

	CWnd* pParent;
public:
	CTreeCtrl dirTree;
	CListCtrl fileList;
	HTREEITEM lastItemSelected;
	std::shared_ptr<CDownloadDialog> pDownloadDlg;
	afx_msg void OnNMDblclkTree(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnNMClickTree(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnNMRClickList(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnDownload();
};
