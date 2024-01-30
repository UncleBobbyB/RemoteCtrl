#pragma once
#include "afxdialogex.h"


// CStartupDlg 对话框

class CStartupDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CStartupDlg)

public:
	CStartupDlg(CWnd* pParent, CString& strIP, UINT& nPort, bool& ok);   // 标准构造函数
	virtual ~CStartupDlg();

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_STARTUP_DIALOG };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()


	CIPAddressCtrl m_addrEdit;
	CEdit m_portEdit;
	CButton m_btnOK;

	CString& strIP_;
	UINT& nPort_;
	bool& ok_;
public:
	virtual void OnCancel();
	virtual BOOL OnInitDialog();
	afx_msg void OnBnClickedButton1();
};
