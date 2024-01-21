
// RemoteServerDlg.h: 头文件
//

#pragma once
#include <thread_pool.h>
#include <CPacket.h>

// CRemoteServerDlg 对话框
class CRemoteServerDlg : public CDialogEx
{
// 构造
public:
	CRemoteServerDlg(CWnd* pParent = nullptr);	// 标准构造函数

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_REMOTESERVER_DIALOG };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 支持

	
// 实现
protected:
	HICON m_hIcon;
	thread_pool thread_pool_;

	void frame_thread_routine();
	void cmd_thread_routine();
	void handle_cmd(CMD_Flag flag, std::shared_ptr<BYTE[]> data);

	// 生成的消息映射函数
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
};
