﻿
// RemoteServerDlg.cpp: 实现文件
//

#include "pch.h"
#include "framework.h"
#include "RemoteServer.h"
#include "RemoteServerDlg.h"
#include "afxdialogex.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif
#include <iostream>
#include <thread>
#include "SimulateMouse.h"

// CRemoteServerDlg 对话框


CRemoteServerDlg::CRemoteServerDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_REMOTESERVER_DIALOG, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CRemoteServerDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CRemoteServerDlg, CDialogEx)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_WM_MOUSEMOVE()
	ON_WM_LBUTTONDOWN()
END_MESSAGE_MAP()


void CaptureScreen(CImage& screen) {
	if (!screen.IsNull())
		screen.Destroy();
	HDC hScreen = GetDC(NULL);
	int nBitPerPixel = GetDeviceCaps(hScreen, BITSPIXEL);
	int nWidth = GetDeviceCaps(hScreen, HORZRES);
	int nHeight = GetDeviceCaps(hScreen, VERTRES);
	screen.Create(nWidth, nHeight, nBitPerPixel);
	BitBlt(screen.GetDC(), 0, 0, nWidth, nHeight, hScreen, 0, 0, SRCCOPY);
	ReleaseDC(NULL, hScreen);
	HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, 0);
	assert(hMem != NULL);
	IStream* pStream = NULL;
	assert(CreateStreamOnHGlobal(hMem, TRUE, &pStream) == S_OK);
	screen.Save(pStream, Gdiplus::ImageFormatPNG);
	pStream->Release();
	screen.ReleaseDC();
}

void send_all(SOCKET sock, const char* buffer, int len) {
	std::cerr << "sending packet, size: " << len << std::endl;
	int total_sent = 0;
	while (total_sent < len) {
		int n = send(sock, buffer + total_sent, len - total_sent, 0);
		if (!n) {
			// peer disconnect
			std::cerr << "peer disconnect" << std::endl;
			return;
		}
		if (n == SOCKET_ERROR) {
			int errCode = WSAGetLastError();
			std::cerr << "socket error: " << errCode << std::endl;
			return;
		}
		total_sent += n;
	}
	std::cerr << "packet send okay" << std::endl;
}

void CRemoteServerDlg::frame_thread_routine() {
	SOCKET ListenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (ListenSocket == INVALID_SOCKET) {
		TRACE("Error at socket(): %ld\n", WSAGetLastError());
		WSACleanup();
		return;
	}

	sockaddr_in service;
	service.sin_family = AF_INET;
	service.sin_addr.s_addr = INADDR_ANY; // or inet_addr("127.0.0.1");
	service.sin_port = htons(9527); // Port number

	if (bind(ListenSocket, (SOCKADDR*)&service, sizeof(service)) == SOCKET_ERROR) {
		TRACE("bind() failed.\n");
		closesocket(ListenSocket);
		WSACleanup();
		return;
	}

	if (listen(ListenSocket, SOMAXCONN) == SOCKET_ERROR) {
		TRACE("Error at listen(): %ld\n", WSAGetLastError());
		closesocket(ListenSocket);
		WSACleanup();
		return;
	}

	SOCKET clnt_sock;
	clnt_sock = accept(ListenSocket, NULL, NULL);
	if (clnt_sock == INVALID_SOCKET) {
		TRACE("accept failed: %d\n", WSAGetLastError());
		closesocket(ListenSocket);
		WSACleanup();
		return;
	}

	TRACE("accepted\n");

	CImage image;
	while (1) {
		// Capture Screen
		//CaptureScreen(image);
		if (!image.IsNull())
			image.Destroy();

		HDC hScreen = ::GetDC(NULL);
		int nBitPerPixel = GetDeviceCaps(hScreen, BITSPIXEL);
		int nWidth = GetDeviceCaps(hScreen, HORZRES);
		int nHeight = GetDeviceCaps(hScreen, VERTRES);
		image.Create(nWidth, nHeight, nBitPerPixel);
		BitBlt(image.GetDC(), 0, 0, nWidth, nHeight, hScreen, 0, 0, SRCCOPY);
		::ReleaseDC(NULL, hScreen);

		HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, 0);
		assert(hMem != NULL);
		IStream* pStream = NULL;
		assert(CreateStreamOnHGlobal(hMem, TRUE, &pStream) == S_OK);
		image.Save(pStream, Gdiplus::ImageFormatPNG);
		LARGE_INTEGER bg{ 0 };
		pStream->Seek(bg, STREAM_SEEK_SET, NULL);
		PBYTE pData = (PBYTE)GlobalLock(hMem);

		// to BYTE*
		// CPacketHeader + raw data
		SIZE_T size_raw_data = GlobalSize(hMem);
		CPacketHeader header(frame, size_raw_data);

		// send data
		send_all(clnt_sock, reinterpret_cast<char*>(&header), sizeof(header));
		send_all(clnt_sock, (char*)pData, size_raw_data);


		pStream->Release();
		image.ReleaseDC();
		GlobalUnlock(hMem);
	}
}

void CRemoteServerDlg::handle_cmd(CMD_Flag flag, std::shared_ptr<BYTE[]> data = nullptr) {
	std::pair<double, double> p;
	int screenWidth = GetSystemMetrics(SM_CXSCREEN);
	int screenHeight = GetSystemMetrics(SM_CYSCREEN);
	switch (flag) {
	case mouse_move:
		assert(data != nullptr);
		p = *reinterpret_cast<std::pair<double, double>*>(data.get());
		SetMousePosition(p.first * screenWidth, p.second * screenHeight);
		break;
	case lb_down:
		assert(data != nullptr);
		p = *reinterpret_cast<std::pair<double, double>*>(data.get());
		SetMousePosition(p.first * screenWidth, p.second * screenHeight);
		SimulateLeftDown();
		break;
	case lb_up:
		assert(data != nullptr);
		p = *reinterpret_cast<std::pair<double, double>*>(data.get());
		SetMousePosition(p.first * screenWidth, p.second * screenHeight);
		SimulateLeftUp();
		break;
	case lb_2click:
		assert(data != nullptr);
		p = *reinterpret_cast<std::pair<double, double>*>(data.get());
		SetMousePosition(p.first * screenWidth, p.second * screenHeight);
		SimulateLeftDoubleClick();
		break;
	case rb_down:
		assert(data != nullptr);
		p = *reinterpret_cast<std::pair<double, double>*>(data.get());
		SetMousePosition(p.first * screenWidth, p.second * screenHeight);
		SimulateRightDown();
		break;
	case rb_up:
		assert(data != nullptr);
		p = *reinterpret_cast<std::pair<double, double>*>(data.get());
		SetMousePosition(p.first * screenWidth, p.second * screenHeight);
		SimulateRightUp();
		break;
	default:
		throw std::runtime_error("Unknown flag");
		break;
	}
}


void CRemoteServerDlg::cmd_thread_routine() {
	SOCKET ListenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (ListenSocket == INVALID_SOCKET) {
		TRACE("Error at socket(): %ld\n", WSAGetLastError());
		WSACleanup();
		return;
	}

	sockaddr_in service;
	service.sin_family = AF_INET;
	service.sin_addr.s_addr = INADDR_ANY; // or inet_addr("127.0.0.1");
	service.sin_port = htons(9528); // Port number

	if (bind(ListenSocket, (SOCKADDR*)&service, sizeof(service)) == SOCKET_ERROR) {
		TRACE("bind() failed.\n");
		closesocket(ListenSocket);
		WSACleanup();
		return;
	}

	if (listen(ListenSocket, SOMAXCONN) == SOCKET_ERROR) {
		TRACE("Error at listen(): %ld\n", WSAGetLastError());
		closesocket(ListenSocket);
		WSACleanup();
		return;
	}

	SOCKET clnt_sock;
	clnt_sock = accept(ListenSocket, NULL, NULL);
	if (clnt_sock == INVALID_SOCKET) {
		TRACE("accept failed: %d\n", WSAGetLastError());
		closesocket(ListenSocket);
		WSACleanup();
		return;
	}

	TRACE("accepted\n");

	std::shared_ptr<BYTE[]> buffer(new BYTE[1024]);
	size_t total_received = 0;
	size_t data_len = 0;
	const size_t header_len = sizeof(CPacketHeader);
	while (1) {
		// settle buffered data
		while (total_received >= header_len + data_len) {
			assert(total_received == header_len + data_len);
			CPacketHeader header = *reinterpret_cast<CPacketHeader*>(buffer.get());
			std::shared_ptr<BYTE[]> data = nullptr;
			if (data_len) {
				data = std::shared_ptr<BYTE[]>(new BYTE[data_len]);
				memcpy(data.get(), buffer.get() + header_len, data_len);
			}
			//thread_pool_.add_task(&CRemoteServerDlg::handle_cmd, this, header.cmd, data);
			handle_cmd(header.cmd, data);
			total_received = 0;
			data_len = 0;
		}

		if (!data_len) {
			// expecting a header
			assert(header_len >= total_received);
			int n = recv(clnt_sock, (char*)buffer.get() + total_received, header_len - total_received, 0);
			if (n == 0) {
				TRACE("peer disconnect\n");
				break;
			}
			if (n < 0) {
				TRACE("socket error\n");
				break;
			}

			total_received += n;
			if (total_received < header_len)
				continue;
			CPacketHeader header = *reinterpret_cast<CPacketHeader*>(buffer.get());
			assert(header.header == HEADER_FLAG);
			data_len = header.len;
		} else {
			assert(total_received >= header_len);
			int n = recv(clnt_sock, (char*)buffer.get() + total_received, data_len - total_received + header_len, 0);
			if (n == 0) {
				TRACE("peer disconnect\n");
				break;
			}
			if (n < 0) {
				TRACE("socket error\n");
				break;
			}

			total_received += n;
			assert(total_received <= header_len + data_len);
		}

	}
}

// CRemoteServerDlg 消息处理程序

BOOL CRemoteServerDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// 设置此对话框的图标。  当应用程序主窗口不是对话框时，框架将自动
	//  执行此操作
	SetIcon(m_hIcon, TRUE);			// 设置大图标
	SetIcon(m_hIcon, FALSE);		// 设置小图标

	// TODO: 在此添加额外的初始化代码
	WSADATA wsaData;
	int iResult;

	// Initialize Winsock
	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0) {
		TRACE("WSAStartup failed: %d\n", iResult);
		return 1;
	}

	std::thread frame_thread(&CRemoteServerDlg::frame_thread_routine, this);
	frame_thread.detach();

	std::thread cmd_thread(&CRemoteServerDlg::cmd_thread_routine, this);
	cmd_thread.detach();

	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

// 如果向对话框添加最小化按钮，则需要下面的代码
//  来绘制该图标。  对于使用文档/视图模型的 MFC 应用程序，
//  这将由框架自动完成。

void CRemoteServerDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // 用于绘制的设备上下文

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 使图标在工作区矩形中居中
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// 绘制图标
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

//当用户拖动最小化窗口时系统调用此函数取得光标
//显示。
HCURSOR CRemoteServerDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}
