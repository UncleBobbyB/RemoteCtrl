
// RemoteServerDlg.h: 头文件
//

#pragma once
#include <thread_pool.h>
#include <CPacket.h>
#include <blocking_queue.h>
#include <memory>

// CRemoteServerDlg 对话框
class CRemoteServerDlg : public CDialogEx {
	typedef blocking_queue<std::pair<size_t, std::shared_ptr<BYTE[]>>> io_queue;

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
	blocking_queue<std::pair<CPacketHeader, std::shared_ptr<BYTE[]>>> mouse_cmd_to_handle_{ 100 };
	io_queue IOqu_cmd_to_send_{ 100 };

	void frame_thread_routine();
	void cmd_recv_thread_routine();
	void mouse_cmd_thread_routine();
	void handle_cmd(CPacketHeader header, std::shared_ptr<BYTE[]> data);
	void cmd_send_thread_routine();

	// 生成的消息映射函数
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
};
