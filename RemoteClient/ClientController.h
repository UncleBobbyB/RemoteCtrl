#pragma once
#include <mutex>
#include <memory>
#include <atlimage.h>
#include <thread>
#include <afxwin.h>
#include "NetController.h"

#define W 1920 // resolution
#define H 1080 // resolution
#define C 3 // RGB channel

class CClientController {

public:
	CClientController() = delete;

	CClientController(const CClientController&) = delete;
	CClientController& operator=(const CClientController&) = delete;

	// Initializes and starts the network thread, if not already running
	static void init(const char* strServerIP, UINT nPort) {
		strServerIP_ = strServerIP;
		nPort_ = nPort;
		CNetController::init(strServerIP_, nPort_, 
			std::shared_ptr<std::mutex>(&mtx_frame_buffer_), std::shared_ptr<std::shared_ptr<CImage>>(&frame_buffer_));
	}

	// ui�̵߳��øú������û��������л�ȡ��Ƶ֡���ݵ�ַ
	static std::shared_ptr<CImage> getFrame() {
		std::lock_guard<std::mutex> lck(mtx_frame_buffer_);
		return std::move(frame_buffer_);
	}

	// ui�̵߳��øú�������cmd��Ϣ
	static void sendMouseEvent(CMD_Flag flag, const std::pair<double, double>& p) {
		size_t buffer_size = sizeof(flag) + sizeof(p);
		std::shared_ptr<BYTE[]> buffer(new BYTE[buffer_size]);
		memcpy(buffer.get(), &flag, sizeof(flag));
		memcpy(buffer.get() + sizeof(flag), &p, sizeof(p));

		CNetController::send_cmd(buffer_size, buffer);
	}

	static void requestDriveInfo() {
		CNetController::request_drive_info();
	}

	static void requestDirInfo(CString strPath) {
		CNetController::request_dir_info(strPath);
	}


private:
	inline static const char* strServerIP_;
	inline static UINT nPort_;

	inline static std::mutex mtx_frame_buffer_{};
	inline static std::shared_ptr<CImage> frame_buffer_{ nullptr }; // һ��ֻ�洢һ֡
};

