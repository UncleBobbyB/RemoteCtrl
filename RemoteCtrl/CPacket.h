#pragma once
#include <wtypes.h>
#include <memory>

#define HEADER_FLAG 0xFEFF

enum CMD {
	frame,
	mouse,
};

enum Mouse {
	mouse_move,
	lb_down,
	lb_up,
	lb_click,
	lb_2click,
	rb_down,
	rb_up,
	rb_click,
	mb_clicked,
	mouse_wheel
};

class CPacketHeader {
public:
	WORD header; // FE FF
	CMD cmd;
	DWORD len;

	CPacketHeader(std::shared_ptr<BYTE[]> pData) {
		header = *reinterpret_cast<WORD*> (pData.get());
		if (header != HEADER_FLAG) {
			// not a header
			len = -2;
			return;
		}
		cmd = *reinterpret_cast<CMD*>(pData.get() + sizeof(header));
		len = *reinterpret_cast<DWORD*>(pData.get() + sizeof(header) + sizeof(cmd));
	}

	CPacketHeader(const CMD& cmd, const DWORD& len)
		: header(HEADER_FLAG), cmd(cmd), len(len) {
	}
};
