#pragma once
#include "afxdialogex.h"


// CDirInfoDlg 对话框

class CDirInfoDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CDirInfoDlg)

public:
	CDirInfoDlg(CWnd* pParent = nullptr);   // 标准构造函数
	virtual ~CDirInfoDlg();

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DIRONFIDIALOG };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()
public:
	CTreeCtrl file_info;
	virtual BOOL OnInitDialog();
};
