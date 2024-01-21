
// RemoteClientDlg.h: 头文件
//

#pragma once
#include <memory>

//#define ip "127.0.0.1"
#define ip "10.0.0.18"
#define port 9527

#define WM_FRAME_DATA_AVAILABLE (WM_USER + 110)

// CRemoteClientDlg 对话框
class CRemoteClientDlg : public CDialogEx
{
// 构造
public:
	CRemoteClientDlg(CWnd* pParent = nullptr);	// 标准构造函数

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_REMOTECLIENT_DIALOG };
#endif

protected:
virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 支持


// 实现
//protected:
public:
	HICON m_hIcon;
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
	DECLARE_MESSAGE_MAP()


	// 用图片渲染视频帧
public:
	afx_msg void OnContextMenu(CWnd* /*pWnd*/, CPoint /*point*/);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnFile();
};
