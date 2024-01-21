#pragma once
#include "blocking_queue.h"
#include "observable.h"
#include <wtypes.h>
#include <afxsock.h>
#include <memory>
#include "CFrameSocket.h"
#include "CPacket.h"
#include <iostream>
#include "thread_pool.h"

#define W 1920 // resolution
#define H 1080 // resolution
#define C 3 // RGB channel

#define WM_FRAME_DATA_AVAILABLE (WM_USER + 110)

/*
* 网络控制模块
* 拥有两个阻塞队列
* 视频帧：
*	一个线程作为生产者异步从TCP缓冲区读取数据到阻塞队列
*	一个线程作为消费者异步从阻塞队列读取数据到用户缓冲区
* 命令：
*	主线程作为往阻塞队列写入数据的生产者
*	socket线程作为从阻塞队列读取并发送数据的消费者
*/
class CNetController {
	typedef blocking_queue<std::pair<size_t, std::shared_ptr<BYTE[]>>> io_queue;
public:
	/*
	* 网络控制器函数返回类型
	*/
	// constructor
	CNetController() = delete;


	static bool init(const CString& strServerIP, UINT nPort, 
		std::shared_ptr<std::mutex> p_mtx_frame_buffer, std::shared_ptr<std::shared_ptr<CImage>> p_frame_buffer) {
		pMainWnd = AfxGetMainWnd();

		// init frame data socket
		WSADATA wsaData;
		struct sockaddr_in server;
		// Initialize Winsock
		if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
			std::cerr << "Failed. Error Code : " << WSAGetLastError() << std::endl;
			return 1;
		}
		// Create socket
		do {
			frame_sock_ = socket(AF_INET, SOCK_STREAM, 0);
			if (frame_sock_ == INVALID_SOCKET) {
				TRACE("Could not create socket : %d\n", WSAGetLastError());
				Sleep(100);
			}
		} while (frame_sock_ == INVALID_SOCKET);
		do {
			cmd_sock_ = socket(AF_INET, SOCK_STREAM, 0);
			if (cmd_sock_ == INVALID_SOCKET) {
				TRACE("Could not create socket : %d\n", WSAGetLastError());
				Sleep(100);
			}
		} while (cmd_sock_ == INVALID_SOCKET);
		// Setup the server address
		//server.sin_addr.s_addr = inet_addr(strServerIP);
		if (inet_pton(AF_INET, strServerIP, &server.sin_addr) <= 0) {
			TRACE("Invalid address/ Address not supported\n");
			return false;
		}
		server.sin_family = AF_INET;
		server.sin_port = htons(nPort);
		// Connect to server
		int ret;
		do {
			ret = connect(frame_sock_, (struct sockaddr*)&server, sizeof(server));
			if (ret < 0) {
				TRACE("frame socket connect error\n");
				Sleep(100);
			}
		} while (ret < 0);
		TRACE("frame socket conenct succeed\n");
		server.sin_port = htons(nPort + 1);
		do {
			ret = connect(cmd_sock_, (struct sockaddr*)&server, sizeof(server));
			if (ret < 0) {
				TRACE("cmd sockeet connect error\n");
				Sleep(100);
			}
		} while (ret < 0);
		TRACE("cmd socket conenct succeed\n");

		p_mtx_frame_buffer_ = p_mtx_frame_buffer;
		p_frame_buffer_ = p_frame_buffer;

		std::thread frameIO_main_thread(&frameIO_main_thread_routine, std::ref(frameIO_main_thread_terminated));
		frameIO_main_thread.detach();

		std::thread cmd_send_thread(&cmd_send_thread_routine, std::ref(cmd_send_thread_terminated));
		cmd_send_thread.detach();

		return true;
	}

	static void send_cmd(size_t buffer_size, std::shared_ptr<BYTE[]> data) {
		// data = cmd flag + cmd data (if any)
		thread_pool_.add_task(&decltype(IOqu_cmd_)::enqueue, &IOqu_cmd_, std::make_pair(buffer_size, data));
	}

private:
	inline static CWnd* pMainWnd{ nullptr };
	inline static thread_pool thread_pool_{};

	inline static SOCKET frame_sock_{};
	inline static io_queue IOqu_frame_{ 10 }; // 最多存储10帧数据
	inline static std::atomic<bool> frameIO_main_thread_terminated{ false };
	inline static std::shared_ptr<std::mutex> p_mtx_frame_buffer_;
	inline static std::shared_ptr<std::shared_ptr<CImage>> p_frame_buffer_;

	inline static std::atomic<bool> flag_on_recv_{ false };

	inline static SOCKET cmd_sock_{};
	inline static io_queue IOqu_cmd_{ 100 }; // cmd阻塞队列
	inline static std::atomic<bool> cmd_send_thread_terminated{ false };
		
	// converting raw data to CImage
	static std::shared_ptr<CImage> raw_data_to_CImage(BYTE* data, size_t size_data) {
		if (data == nullptr) {
			return nullptr;
		}

		auto pImage = std::make_shared<CImage>();
		IStream* pStream = SHCreateMemStream(data, size_data);
		if (pStream == nullptr) {
			TRACE("not supposed to be here");
			return nullptr;
		}

		if (!SUCCEEDED(pImage->Load(pStream))) {
			TRACE("not supposed to be here");
			return nullptr;
		}

		pStream->Release();
		return pImage;
	}

	// 消费线程函数
	static void frameIO_main_thread_routine(const std::atomic<bool>& terminated) {
		AfxSocketInit();

		std::atomic<bool> frameIO_recv_thread_terminated = false;
		std::thread frameIO_recv_thread(&frameIO_recv_thread_rountine, std::ref(frameIO_recv_thread_terminated)); // frame IO dedicated thread
		
		while (!terminated.load()) {
			auto [buffer_size, buffer] = IOqu_frame_.dequeue(); // 自动阻塞
			auto pImage = raw_data_to_CImage(buffer.get(), buffer_size);
			if (pImage == nullptr) {
				TRACE("raw data to CImage failed");
				continue;
			}

			{
				std::lock_guard<std::mutex> lck(*p_mtx_frame_buffer_);
				*p_frame_buffer_ = pImage;
			}

			//CWnd* pMainWnd = AfxGetMainWnd();
			if (pMainWnd && ::IsWindow(pMainWnd->m_hWnd)) {
				pMainWnd->PostMessage(WM_FRAME_DATA_AVAILABLE);
			} else {
				TRACE("not supposed to be here");
			}
		}

		frameIO_recv_thread_terminated = true;
		if (frameIO_recv_thread.joinable())
			frameIO_recv_thread.join();

		closesocket(frame_sock_);
		WSACleanup();
	}

	static void handle_peer_disconnect() {
		// TODO: peer disconnect
		TRACE("peer disconnect\n");
	}

	static void handle_socket_error() {
		// TODO: handle socket error
		int error_code = WSAGetLastError();
		std::cerr << "recv failed with error: " << error_code << std::endl;

		// Handle specific errors
		switch (error_code) {
		case WSAECONNRESET:
			// Connection reset by peer
			std::cerr << "Connection reset by peer." << std::endl;
			// Handle connection reset logic
			break;
		case WSAETIMEDOUT:
			// Timeout error
			std::cerr << "Operation timed out." << std::endl;
			// Handle timeout logic
			break;
			// Add additional cases as needed
		default:
			// Other errors
			std::cerr << "Some other error occurred." << std::endl;
			break;
		}

		// Decide on the course of action
		// E.g., close socket, retry, etc.
		closesocket(frame_sock_);
		WSACleanup();
	}

	// 负责从TCP缓冲区读取到用户缓冲区的生产线程函数
	static void frameIO_recv_thread_rountine(const std::atomic<bool>& terminated) {
		size_t buffer_size = W * H * C * 2;
		std::shared_ptr<BYTE[]> buffer(new BYTE[buffer_size]);
		size_t total_received = 0;
		size_t data_len = 0;
		size_t header_len = sizeof(CPacketHeader);
		while (!terminated.load()) {
			// settle buffered data
			while (total_received >= header_len +  data_len) {
				assert(total_received == header_len + data_len);
				std::shared_ptr<BYTE[]> item(new BYTE[data_len]);
				memcpy(item.get(), buffer.get() + header_len, data_len);
				IOqu_frame_.enqueue({ data_len, item });
				total_received = 0;
				data_len = 0;
			}

			if (!data_len) {
				// start from header
				assert(total_received <= header_len);
				int n_received = recv(frame_sock_, (char*)buffer.get() + total_received, header_len - total_received, 0); // blocks at here
				if (n_received == 0) {
					handle_peer_disconnect();
					continue;
				}
				if (n_received < 0) {
					handle_socket_error();
					continue;
				}
				total_received += n_received;
				if (total_received < header_len)
					continue;
				assert(total_received == header_len);
				//CPacketHeader header(buffer);
				CPacketHeader header = *reinterpret_cast<CPacketHeader*>(buffer.get());
				assert(header.header == HEADER_FLAG);
				data_len = header.len;
			} else {
				// continue reading
				assert(total_received >= header_len);
				int n_received = recv(frame_sock_, (char*)buffer.get() + total_received, data_len - total_received + header_len, 0); // blocks at here
				if (n_received == 0) {
					handle_peer_disconnect();
					continue;
				}
				if (n_received < 0) {
					handle_socket_error();
				}
				total_received += n_received;
				assert(total_received <= header_len + data_len);
			}
		}
	}

	static void send_all(SOCKET sock, const char* buffer, int len) {
		int total_sent = 0;
		while (total_sent < len) {
			int n = send(sock, buffer + total_sent, len - total_sent, 0);
			if (!n) {
				handle_peer_disconnect();
				return;
			}
			if (n == SOCKET_ERROR) {
				handle_socket_error();
				return;
			}
			total_sent += n;
		}
	}

	// 负责发送cmd的线程
	static void cmd_send_thread_routine(const std::atomic<bool>& terminated) {
		while (!terminated.load()) {
			auto [buffer_size, buffer] = IOqu_cmd_.dequeue(); // will be blocked here
			// buffer = cmd flag + cmd data (if any)
			CMD_Flag cmd = *reinterpret_cast<CMD_Flag*>(buffer.get());
			CPacketHeader header(cmd, buffer_size - sizeof(CMD_Flag));
			send_all(cmd_sock_, reinterpret_cast<char*>(&header), sizeof(CPacketHeader));
			if (header.len) {
				send_all(cmd_sock_, reinterpret_cast<char*>(buffer.get()) + sizeof(CMD_Flag), header.len);
			}
		}
	}
};

