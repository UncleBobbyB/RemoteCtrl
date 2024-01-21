#pragma once
#include <functional>
#include <vector>
#include "concurrent.h"

template<typename T>
class observable {
private:
	using _callback = std::function<void(const T&)>;
	concurrent<std::vector<_callback>> observers;

public:
	void subscribe(const _callback& observer) {
		observers.invoke([](std::vector<_callback>& a) { a.push_back(observer); });
	}

	void notify_all(const T& message) {
		for (const auto& obs : observers) {
			obs(message);
		}
	}
};

