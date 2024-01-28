/*
#include <stdio.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "Ws2_32.lib")
#include <atlimage.h>
#include <Windows.h>
#include <iostream>
#include <memory>
#include "CPacket.h"
#include <cassert>
#include <thread>
#include <afx.h>

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


void frame_thread_routine() {
	SOCKET ListenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (ListenSocket == INVALID_SOCKET) {
		TRACE("Error at socket(): %ld\n", WSAGetLastError());
		WSACleanup();
		return ;
	}

	sockaddr_in service;
	service.sin_family = AF_INET;
	service.sin_addr.s_addr = INADDR_ANY; // or inet_addr("127.0.0.1");
	service.sin_port = htons(9527); // Port number

	if (bind(ListenSocket, (SOCKADDR*)&service, sizeof(service)) == SOCKET_ERROR) {
		TRACE("bind() failed.\n");
		closesocket(ListenSocket);
		WSACleanup();
		return ;
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

		HDC hScreen = GetDC(NULL);
		int nBitPerPixel = GetDeviceCaps(hScreen, BITSPIXEL);
		int nWidth = GetDeviceCaps(hScreen, HORZRES);
		int nHeight = GetDeviceCaps(hScreen, VERTRES);
		image.Create(nWidth, nHeight, nBitPerPixel);
		BitBlt(image.GetDC(), 0, 0, nWidth, nHeight, hScreen, 0, 0, SRCCOPY);
		ReleaseDC(NULL, hScreen);

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



int main1() {
	WSADATA wsaData;
	int iResult;

	// Initialize Winsock
	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0) {
		TRACE("WSAStartup failed: %d\n", iResult);
		return 1;
	}

	std::thread t(frame_thread_routine);
	t.detach();


	// Get the standard input handle
	HANDLE hStdin = GetStdHandle(STD_INPUT_HANDLE);
	DWORD fdwSaveOldMode;

	// Save the current input mode, to be restored on exit
	GetConsoleMode(hStdin, &fdwSaveOldMode);

	// Enable the window input events and mouse input events
	SetConsoleMode(hStdin, ENABLE_EXTENDED_FLAGS | ENABLE_WINDOW_INPUT | ENABLE_MOUSE_INPUT);

	// Event buffer
	INPUT_RECORD irInBuf[128];
	DWORD cNumRead;

	std::cout << "Listening for mouse events. Press 'q' to quit." << std::endl;

	// Main loop
	while (true) {
		// Wait for the events
		ReadConsoleInput(hStdin, irInBuf, 128, &cNumRead);

		// Handle each event in the buffer
		for (DWORD i = 0; i < cNumRead; i++) {
			switch (irInBuf[i].EventType) {
			case MOUSE_EVENT: // Mouse event
				MOUSE_EVENT_RECORD mer = irInBuf[i].Event.MouseEvent;
				std::cout << "Mouse event: ";

				if (mer.dwEventFlags == 0) {
					if (mer.dwButtonState == FROM_LEFT_1ST_BUTTON_PRESSED) {
						std::cout << "Left button press" << std::endl;
					}
				} else if (mer.dwEventFlags == MOUSE_MOVED) {
					std::cout << "Mouse moved" << std::endl;
				}
				break;

			case KEY_EVENT: // Keyboard event
				KEY_EVENT_RECORD ker = irInBuf[i].Event.KeyEvent;
				if (ker.bKeyDown && ker.uChar.AsciiChar == 'q') {
					// Restore the original console mode
					SetConsoleMode(hStdin, fdwSaveOldMode);
					return 0;
				}
				break;

			default:
				// Handle other events
				break;
			}
		}
	}

	// Restore the original console mode
	SetConsoleMode(hStdin, fdwSaveOldMode);

	getchar();

	return 0;
}

*/

#define _AFXDLL
#include <windows.h>
#include <iostream>
#include <vector>
#include <string>

struct FileInfo {
	std::string path;
	bool isDirectory;
};

void ListDirectoryContents(const std::string& directoryPath, std::vector<FileInfo>& fileInfos) {
	WIN32_FIND_DATA findFileData;
	HANDLE hFind = FindFirstFile((directoryPath + "\\*").c_str(), &findFileData);

	if (hFind == INVALID_HANDLE_VALUE) {
		return;
	}

	do {
		if (strcmp(findFileData.cFileName, ".") != 0 && strcmp(findFileData.cFileName, "..") != 0) {
			FileInfo fileInfo;
			fileInfo.path = directoryPath + "\\" + findFileData.cFileName;
			fileInfo.isDirectory = (findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
			fileInfos.push_back(fileInfo);

			if (fileInfo.isDirectory) {
				// Recursively list subdirectories
				std::cerr << fileInfo.path << std::endl;
				ListDirectoryContents(fileInfo.path, fileInfos);
			}
		}
	} while (FindNextFile(hFind, &findFileData) != 0);

	FindClose(hFind);
}

std::vector<FileInfo> GetSystemDrivesInfo() {
	std::vector<FileInfo> allFiles;
	DWORD driveMask = GetLogicalDrives();
	char driveLetter = 'A';

	while (driveMask) {
		if (driveMask & 1) {
			std::string driveStr = std::string(1, driveLetter) + ":";
			if (GetDriveType((driveStr + "\\").c_str()) == DRIVE_FIXED) {
				ListDirectoryContents(driveStr, allFiles);
			}
		}
		driveLetter++;
		driveMask >>= 1;
	}

	return allFiles;
}

int main() {
	std::vector<FileInfo> fileInfos = GetSystemDrivesInfo();

	for (const auto& fileInfo : fileInfos) {
		std::cout << (fileInfo.isDirectory ? "[D] " : "[F] ") << fileInfo.path << std::endl;
	}

	return 0;
}

