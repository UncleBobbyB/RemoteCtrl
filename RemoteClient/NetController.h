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
#include "file_info.h"

#define W 1920 // resolution
#define H 1080 // resolution
#define C 3 // RGB channel

#define WM_FRAME_DATA_AVAILABLE (WM_USER + 110)
#define WM_DIR_TREE_UPDATED (WM_USER + 111)
#define WM_DIR_TREE_INVALID_DIR (WM_USER + 112)
#define WM_FILE_TREE_UPDATED (WM_USER + 113)
#define WM_DOWNLOAD_INVALID_FILE (WM_USER + 114)
#define WM_DOWNLOAD_FILE_SIZE (WM_USER + 115)
#define WM_DOWNLOAD_DATA_RECEIVED (WM_USER + 116)
#define WM_DOWNLOAD_COMPLETE (WM_USER + 117)
#define WM_DOWNLOAD_ABORT (WM_USER + 118)

/*
* 网络控制模块
* ------------------------------------------------------------------------------------------------------
* 主线程会调用其中的public函数，所有操作均为异步执行，如果调用需要访问阻塞队列的函数，则由线程池代理执行
* 调用该模块的线程（通常为主线程）不会阻塞
* ------------------------------------------------------------------------------------------------------
* 拥有三个阻塞队列
* 视频帧：
*	一个线程作为生产者异步从TCP缓冲区读取数据到阻塞队列
*	一个线程作为消费者异步从阻塞队列读取数据到用户缓冲区
* 命令：
*	主线程作为往阻塞队列写入数据的生产者
*	socket线程作为从阻塞队列读取并发送数据的消费者
* 文件传输：
*	网络传输的数据包与命令共享一个阻塞队列
*	命令模块的消费者（作为文件数据阻塞队列的生产者）将文件数据包放到另一个阻塞队列中
*	由文件数据阻塞队列的消费者负责组装文件信息
* ------------------------------------------------------------------------------------------------------
*/
class CNetController {
	// never should've written a whole lot of static shit
	// it's almost impossible to clean up or reset the static members on quit/restart
	// have to call the main thread to forcely terminate the whole shit
	// genius.


	typedef blocking_queue<std::pair<size_t, std::shared_ptr<BYTE[]>>> io_queue;
public:
	CNetController() = delete;

	static void destroy() {
		frameIO_main_thread_terminated = true;
		cmd_send_thread_terminated = true;
		cmd_recv_thread_terminated = true;
		handle_cmd_thread_terminated = true;

		IOqu_frame_.terminate();
		IOqu_cmd_send_.terminate();
		Bqu_cmd_recv_.terminate();
		IOqu_file_data_.terminate();
	}

	static bool init(const CString& strServerIP, UINT nPort, 
		std::shared_ptr<std::mutex> p_mtx_frame_buffer, std::shared_ptr<std::shared_ptr<CImage>> p_frame_buffer) {
		pMainWnd = AfxGetMainWnd();

		// Init frame data socket
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
			cmd_send_sock_ = socket(AF_INET, SOCK_STREAM, 0);
			if (cmd_send_sock_ == INVALID_SOCKET) {
				TRACE("Could not create socket : %d\n", WSAGetLastError());
				Sleep(100);
			}
		} while (cmd_send_sock_ == INVALID_SOCKET);
		do {
			cmd_recv_sock_ = socket(AF_INET, SOCK_STREAM, 0);
			if (cmd_recv_sock_ == INVALID_SOCKET) {
				TRACE("Could not create sosket: %d\n", WSAGetLastError());
				Sleep(100);
			}
		} while (cmd_recv_sock_ == INVALID_SOCKET);
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
		TRACE("frame socket connect succeed\n");
		server.sin_port = htons(nPort + 1);
		do {
			ret = connect(cmd_send_sock_, (struct sockaddr*)&server, sizeof(server));
			if (ret < 0) {
				TRACE("cmd socket connect error\n");
				Sleep(100);
			}
		} while (ret < 0);
		TRACE("cmd send socket connect succeed\n");
		server.sin_port = htons(nPort + 2);
		do {
			ret = connect(cmd_recv_sock_, (struct sockaddr*)&server, sizeof(server));
			if (ret < 0) {
				TRACE("cmd socket connect error\n");
				Sleep(100);
			}
		} while (ret < 0);
		TRACE("cmd recv socket connect succeed\n");

		p_mtx_frame_buffer_ = p_mtx_frame_buffer;
		p_frame_buffer_ = p_frame_buffer;

		std::thread frameIO_main_thread(&frameIO_main_thread_routine, std::ref(frameIO_main_thread_terminated));
		frameIO_main_thread.detach();

		std::thread cmd_send_thread(&cmd_send_thread_routine, std::ref(cmd_send_thread_terminated));
		cmd_send_thread.detach();

		std::thread cmd_recv_thread(&cmd_recv_thread_routine, std::ref(cmd_recv_thread_terminated));
		cmd_recv_thread.detach();

		std::thread handle_cmd_thread(&handle_cmd_thread_routine, std::ref(handle_cmd_thread_terminated));
		handle_cmd_thread.detach();

		// Request drive info in advance
		request_drive_info();

		return true;
	}

	static void set_pDirTree(CTreeCtrl* pFileTree) {
		pDirTree_ = pFileTree;
	}

	static void set_pFileTree(CListCtrl* pFileList) {
		pFileList_ = pFileList;
	}

	static void set_pLastItem(HTREEITEM* lastItemSelected) {
		pLastItemSelected_ = lastItemSelected;
	}

	static void send_cmd(size_t buffer_size, std::shared_ptr<BYTE[]> data) {
		// data = cmd flag + cmd data (if any)
		thread_pool_.add_task(&decltype(IOqu_cmd_send_)::enqueue, &IOqu_cmd_send_, std::make_pair(buffer_size, data));
	}

	static void request_drive_info() {
		CMD_Flag flag = drive_info;
		size_t buffer_size = sizeof(flag);
		std::shared_ptr<BYTE[]> buffer(new BYTE[buffer_size]);
		memcpy(buffer.get(), &flag, buffer_size);
		send_cmd(buffer_size, buffer);
	}

	static void request_dir_info(const CString& strPath) {
		CMD_Flag flag = dir_info;
		size_t buffer_size = sizeof(flag) + strPath.GetLength() * sizeof(TCHAR);
		std::shared_ptr<BYTE[]> buffer(new BYTE[buffer_size]);
		memcpy(buffer.get(), &flag, sizeof(flag));
		memcpy(buffer.get() + sizeof(flag), reinterpret_cast<const BYTE*>(strPath.GetString()), strPath.GetLength() * sizeof(TCHAR));
		send_cmd(buffer_size, buffer);
	}

	static void request_download(const CString& strPath, FILE* pFile) {
		assert(pDest_ == nullptr);
		pDest_ = pFile;
		CMD_Flag flag = download;
		size_t data_size = strPath.GetLength() * sizeof(TCHAR);
		std::shared_ptr<BYTE[]> buffer(new BYTE[sizeof(flag) + data_size]);
		memcpy(buffer.get(), &flag, sizeof(flag));
		memcpy(buffer.get() + sizeof(flag), strPath.GetString(), data_size);
		send_cmd(sizeof(flag) + data_size, buffer);
	}

	static void abort_download() {
		IOqu_file_data_.terminate();
		CMD_Flag flag = download_abort;
		std::shared_ptr<BYTE[]> buffer(new BYTE[sizeof(flag)]);
		send_cmd(sizeof(flag), buffer);
	}

private:
	inline static CWnd*			pMainWnd{ nullptr };
	inline static CTreeCtrl*	pDirTree_{ nullptr };
	inline static CListCtrl*	pFileList_{ nullptr };
	inline static HTREEITEM*	pLastItemSelected_{ nullptr };
	inline static thread_pool   thread_pool_{};

	inline static SOCKET						frame_sock_{};
	inline static io_queue						IOqu_frame_{ 10 }; // 最多存储10帧数据
	inline static std::atomic<bool>				frameIO_main_thread_terminated{ false };
	inline static std::shared_ptr<std::mutex>	p_mtx_frame_buffer_;
	inline static std::shared_ptr<std::shared_ptr<CImage>>	p_frame_buffer_;

	inline static std::atomic<bool> flag_on_recv_{ false };

	inline static SOCKET				cmd_send_sock_{};
	inline static io_queue				IOqu_cmd_send_{ 100 }; // cmd阻塞队列
	inline static std::atomic<bool>		cmd_send_thread_terminated{ false };

	inline static SOCKET				cmd_recv_sock_{};
	inline static std::atomic<bool>		cmd_recv_thread_terminated{ false };
	inline static std::atomic<bool>		handle_cmd_thread_terminated{ false };
	inline static blocking_queue<std::tuple<CMD_Flag, size_t, std::shared_ptr<BYTE[]>>> Bqu_cmd_recv_{ 100 };

	inline static FILE*		pDest_{ nullptr };
	inline static io_queue	IOqu_file_data_{ 100 };
	

	// Converting raw data to CImage
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
			auto [buffer_size, buffer] = IOqu_frame_.dequeue().value(); // 自动阻塞
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
		closesocket(cmd_send_sock_);
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
			// Settle buffered data
			while (total_received >= header_len +  data_len) {
				assert(total_received == header_len + data_len);
				std::shared_ptr<BYTE[]> item(new BYTE[data_len]);
				memcpy(item.get(), buffer.get() + header_len, data_len);
				IOqu_frame_.enqueue({ data_len, item });
				total_received = 0;
				data_len = 0;
			}

			if (!data_len) {
				// Start from header
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
				// Continue reading
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
			auto [buffer_size, buffer] = IOqu_cmd_send_.dequeue().value(); // will be blocked here
			// buffer = cmd flag + cmd data (if any)
			CMD_Flag cmd = *reinterpret_cast<CMD_Flag*>(buffer.get());
			CPacketHeader header(cmd, buffer_size - sizeof(CMD_Flag));
			send_all(cmd_send_sock_, reinterpret_cast<char*>(&header), sizeof(CPacketHeader));
			if (header.len) {
				send_all(cmd_send_sock_, reinterpret_cast<char*>(buffer.get()) + sizeof(CMD_Flag), header.len);
			}
		}
	}

	static void cmd_recv_thread_routine(const std::atomic<bool>& terminated) {
		size_t buffer_size = 1024 * 1024; // 1KB
		std::shared_ptr<BYTE[]> buffer(new BYTE[buffer_size]);
		size_t total_received = 0;
		size_t data_len = 0;
		size_t header_len = sizeof(CPacketHeader);
		while (!terminated.load()) {
			while (total_received >= header_len + data_len) {
				assert(total_received == header_len + data_len);
				CPacketHeader header = *reinterpret_cast<CPacketHeader*>(buffer.get());
				CMD_Flag flag = reinterpret_cast<CPacketHeader*>(buffer.get())->cmd;
				std::shared_ptr<BYTE[]> item(new BYTE[data_len]);
				memcpy(item.get(), buffer.get() + header_len, data_len);
				Bqu_cmd_recv_.enqueue({ flag, data_len, item });
				total_received = 0;
				data_len = 0;
			}
			if (!data_len) {
				assert(total_received <= header_len);
				int n_received = recv(cmd_recv_sock_, (char*)buffer.get() + total_received, header_len - total_received, 0);
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
				CPacketHeader header = *reinterpret_cast<CPacketHeader*>(buffer.get());
				assert(header.header == HEADER_FLAG);
				data_len = header.len;
			} else {
				assert(total_received >= header_len);
				int n_received = recv(cmd_recv_sock_, (char*)buffer.get() + total_received, data_len - total_received + header_len, 0);
				if (n_received == 0) {
					handle_peer_disconnect();
					continue;
				}
				if (n_received < 0) {
					handle_socket_error();
					continue;
				}
				total_received += n_received;
				assert(total_received <= header_len + data_len);
			}
		}
	}

	static void handle_drive_info(size_t data_size, std::shared_ptr<BYTE[]> data) {
		pDirTree_->DeleteAllItems();
		std::vector<DriveInfo> drives = DeserializeDriveInfo(data_size, data);
		// Populate tree control
		for (const auto& drive : drives) {
			HTREEITEM hDrive = pDirTree_->InsertItem((drive.drive_letter + std::string(":")).c_str());
			for (const auto& file : drive.files) {
				if (file.is_dir)
					pDirTree_->InsertItem(file.name.c_str(), hDrive);
			}
		}
		// Post message to main thread
		pMainWnd->PostMessage(WM_DIR_TREE_UPDATED);
	}

	static void handle_dir_info(size_t data_size, std::shared_ptr<BYTE[]> data) {
		pFileList_->DeleteAllItems();
		std::vector<FileInfo> files = DeserializeDirInfo(data_size, data);
		if (files.empty()) {
			TRACE("Dir empty\n");
			return;
		}		
		assert(*pLastItemSelected_);
		TRACE("%s\n", pDirTree_->GetItemText(*pLastItemSelected_));
		for (HTREEITEM hChildItem = pDirTree_->GetChildItem(*pLastItemSelected_); hChildItem != NULL; ) {
			HTREEITEM hNextItem = pDirTree_->GetNextSiblingItem(hChildItem);
			pDirTree_->DeleteItem(hChildItem);
			hChildItem = hNextItem;
		}
		for (const auto& file : files) {
			if (file.is_dir) {
				pDirTree_->InsertItem(file.name.c_str(), *pLastItemSelected_);
			} else {
				pFileList_->InsertItem(pFileList_->GetItemCount(), file.name.c_str());
			}
		}
	}

	static void handle_cmd_thread_routine(const std::atomic<bool>& terminated) {
		while (!terminated.load()) {
			auto [flag, data_size, data] = Bqu_cmd_recv_.dequeue().value(); // will be blocked here
			if (flag == drive_info) {
				handle_drive_info(data_size, data);
			} else if (flag == invalid_dir) {
				pMainWnd->PostMessage(WM_DIR_TREE_INVALID_DIR);
			} else if (flag == dir_info) {
				handle_dir_info(data_size, data);
			} else if (flag == invalid_file) {
				if (IOqu_file_data_.isTerminated()) // 下载任务已取消
					return;
				
				pMainWnd->PostMessage(WM_DOWNLOAD_INVALID_FILE);
			} else if (flag == file_size) {
				if (IOqu_file_data_.isTerminated()) // 下载任务已取消
					return;

				assert(data_size == sizeof(long long));
				auto pFileSize = new long long(*reinterpret_cast<long long*>(data.get()));
				TRACE("download file size: %lld\n", *pFileSize);

				thread_pool_.add_task(&thread_download, *pFileSize);

				pMainWnd->PostMessage(WM_DOWNLOAD_FILE_SIZE, 0, (LPARAM)pFileSize); // Remember to delete pFileSize on the other side
			} else if (flag == file_data) {
				if (IOqu_file_data_.isTerminated()) // 下载任务已取消
					return;

				thread_pool_.add_task(&decltype(IOqu_file_data_)::enqueue, &IOqu_file_data_, std::make_pair(data_size, data));

				auto pDataSize = new size_t(data_size); // Remember to delete pDataSize on the other side
				pMainWnd->PostMessage(WM_DOWNLOAD_DATA_RECEIVED, 0, LPARAM(pDataSize));
			}
		}
	}

	static void thread_download(long long file_size) {
		if (IOqu_file_data_.isTerminated()) {
			// already aborted
			fclose(pDest_);
			pDest_ = nullptr;
			IOqu_file_data_.reset();
			return;
		}
		assert(pDest_ != nullptr && file_size != -1);

		if (file_size == 0) {
			TRACE("target file is empty\n");
			fclose(pDest_);
			pDest_ = nullptr;
			return;
		}

		long long total_written = 0;
		while (total_written < file_size) {
			auto item = IOqu_file_data_.dequeue();
			if (!item) // abort download
				break;
			auto [data_size, data] = item.value();
			size_t n = fwrite(data.get(), 1, data_size, pDest_);
			if (n < data_size) {
				TRACE("Writing to file failed\n");
				break;
			}
			total_written += n;
		}

		fclose(pDest_);
		pDest_ = nullptr;
		if (IOqu_file_data_.isTerminated()) {
			// 记得重置阻塞队列
			IOqu_file_data_.reset();
			return;
		}

		pMainWnd->PostMessage(WM_DOWNLOAD_COMPLETE);
	}


};

