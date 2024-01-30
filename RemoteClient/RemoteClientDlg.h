
// RemoteClientDlg.h: 头文件
//

#pragma once
#include <memory>
#include "CFileInfoDlg.h"

#define WM_CLIENT_RESTART (WM_USER + 400)

// CRemoteClientDlg 对话框
class CRemoteClientDlg : public CDialogEx
{
// 构造
public:
	CRemoteClientDlg(CWnd* pParent, CString strIP, UINT nPort);	// 标准构造函数

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_REMOTECLIENT_DIALOG };
#endif

protected:
virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 支持
//virtual void PostNcDestroy() override;


// 实现
//protected:
public:
	CString strIP;
	UINT nPort;

	HICON m_hIcon;
	CFileInfoDlg m_fileInfoDlg;
	CStatic m_videoFrame;
	CWinThread* pMouseEventThread;
	DWORD MouseEventThreadID;
	CPoint m_lastDownPoint;
	DWORD m_lastDownTime;

	std::pair<double, double> GetNormalizedMousePosition();

	// 生成的消息映射函数
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg LRESULT DisplayFrame(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnDirTreeUpdated(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnInvalidDir(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnInvalidFile(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnFileSize(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnDownloadComplete(WPARAM wParam, LPARAM lParam);
	DECLARE_MESSAGE_MAP()


public:
	afx_msg void OnContextMenu(CWnd* /*pWnd*/, CPoint /*point*/);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnFile();
	virtual void OnCancel();
	afx_msg void OnReconnect();
};
