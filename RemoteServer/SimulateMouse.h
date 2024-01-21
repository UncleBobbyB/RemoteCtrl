#pragma once

// 左键单击
void SimulateLeftClick() {
	INPUT input[2] = {}; // Array to store input event structures

	// Simulate left mouse button down
	input[0].type = INPUT_MOUSE;
	input[0].mi.dwFlags = MOUSEEVENTF_LEFTDOWN;

	// Simulate left mouse button up
	input[1].type = INPUT_MOUSE;
	input[1].mi.dwFlags = MOUSEEVENTF_LEFTUP;

	// Send the input events to the system
	SendInput(2, input, sizeof(INPUT));
}

// 左键按下
void SimulateLeftDown() {
	INPUT input[1] = {};

	input[0].type = INPUT_MOUSE;
	input->mi.dwFlags = MOUSEEVENTF_LEFTDOWN;

	SendInput(1, input, sizeof(INPUT));
}

// 左键松开
void SimulateLeftUp() {
	INPUT input[1] = {};

	input[0].type = INPUT_MOUSE;
	input->mi.dwFlags = MOUSEEVENTF_LEFTUP;

	SendInput(1, input, sizeof(INPUT));
}

// 右键按下
void SimulateRightDown() {
	INPUT input[1] = {};

	input[0].type = INPUT_MOUSE;
	input->mi.dwFlags = MOUSEEVENTF_RIGHTDOWN;

	SendInput(1, input, sizeof(INPUT));
}

// 右键松开
void SimulateRightUp() {
	INPUT input[1] = {};

	input[0].type = INPUT_MOUSE;
	input->mi.dwFlags = MOUSEEVENTF_RIGHTUP;

	SendInput(1, input, sizeof(INPUT));
}

// 右键单击
void SimulateRightClick() {
	INPUT input[2] = {};

	input[0].type = INPUT_MOUSE;
	input[0].mi.dwFlags = MOUSEEVENTF_RIGHTDOWN;

	input[1].type = INPUT_MOUSE;
	input[1].mi.dwFlags = MOUSEEVENTF_RIGHTUP;
	
	SendInput(2, input, sizeof(INPUT));
}

// 左键双击
void SimulateLeftDoubleClick() {
    SimulateLeftClick();
    Sleep(100); // Short delay to mimic human double-click speed
    SimulateLeftClick();
}

// 鼠标移动
void SimulateMouseMove(int dx, int dy) {
	INPUT input = {};
	input.type = INPUT_MOUSE;
	input.mi.dx = dx;
	input.mi.dy = dy;
	input.mi.dwFlags = MOUSEEVENTF_MOVE;

	SendInput(1, &input, sizeof(INPUT));
}

// 设置鼠标位置
void SetMousePosition(int x, int y) {
	if (!SetCursorPos(x, y)) {
		// Handle the error
		DWORD errorCode = GetLastError();
		throw std::runtime_error("cannot set mouse pose");
	}
}
