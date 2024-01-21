#pragma once
#include "blocking_queue.h"
#include "observable.h"
#include <winsock2.h>
#include <windows.h>
#include <afxsock.h>
#include <memory>
#include <vector>


/*
* 负责接受视频流的套接字
* 套接字作为阻塞队列的生产者
*/
class CFrameSocket : public CSocket {

public:
	~CFrameSocket() noexcept override {}

	bool init() {
		if (!this->Create()) {
			// TODO: Handle error
			return false;
		}
		if (AsyncSelect(FD_READ | FD_CLOSE) == false) {
			// Handle error
			return false;
		}
		// reuse address
		int bReuseaddr = 1;
		SetSockOpt(SO_REUSEADDR, &bReuseaddr, sizeof(bReuseaddr), SOL_SOCKET);
	}

	// Connects to a specific server IP and port
	bool ConnectToServer() {
		return Connect(strServerIP_, nPort_);
	}

	void set_ip(const CString& strServerIP) {
		this->strServerIP_ = strServerIP;
	}

	void set_port(UINT nPort) {
		this->nPort_ = nPort;
	}

	template<typename F, typename ...Args>
	void register_OnReceive_callback(F f, Args ...args) {
		OnReceive_callback = std::bind(f, args...);
	}

protected:
	// Overridden from CSocket to handle received data
	// main thread calls this
	void OnReceive(int nErrorCode) override {
		if (!nErrorCode) {
			OnReceive_callback();
		} else {
			// TODO: handle error code
		}

		CSocket::OnReceive(nErrorCode);
	}
	// Buffer for receiving data from TCP buffer
	CString strServerIP_;
	UINT nPort_;
	std::function<void()> OnReceive_callback;
};
