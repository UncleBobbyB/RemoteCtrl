
// RemoteClient.h: PROJECT_NAME 应用程序的主头文件
//

#pragma once

#ifndef __AFXWIN_H__
	#error "在包含此文件之前包含 'pch.h' 以生成 PCH"
#endif

#include "resource.h"		// 主符号
#include "RemoteClientDlg.h"

// CRemoteClientApp:
// 有关此类的实现，请参阅 RemoteClient.cpp
//

class CRemoteClientApp : public CWinApp {

public:
	CRemoteClientApp();

// 重写
public:
	virtual BOOL InitInstance();
	virtual int ExitInstance();

// 实现
	afx_msg void OnClientRestart(WPARAM wParam, LPARAM lParam);
	DECLARE_MESSAGE_MAP()

public:
	CRemoteClientDlg* m_pClientDlg;

};

extern CRemoteClientApp theApp;
