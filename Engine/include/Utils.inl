#pragma once

#include "Utils.h"

namespace ez
{
	template<typename T>
	float Timer::Duration() const
	{
		return std::chrono::duration<float, T>(_endTimestamp - _startTimestamp).count();
	}
}
