#pragma once
#include <wtypes.h>
#include <memory>

#define HEADER_FLAG 0xFEFF

enum CMD_Flag {
	frame,
	drive_info,
	dir_info,
	invalid_dir,
	mouse_move,
	lb_down,
	lb_up,
	lb_2click,
	rb_down,
	rb_up,
	mb_clicked,
	mouse_wheel,
	unknown
};

class CPacketHeader {
public:
	WORD header; // FE FF
	CMD_Flag cmd;
	DWORD len;

	CPacketHeader(std::shared_ptr<BYTE[]> pData) {
		header = *reinterpret_cast<WORD*> (pData.get());
		if (header != HEADER_FLAG) {
			// not a header
			len = -2;
			return;
		}
		cmd = *reinterpret_cast<CMD_Flag*>(pData.get() + sizeof(header));
		len = *reinterpret_cast<DWORD*>(pData.get() + sizeof(header) + sizeof(cmd));
	}

	CPacketHeader(const CMD_Flag& cmd, const DWORD& len)
		: header(HEADER_FLAG), cmd(cmd), len(len) {
	}
};
