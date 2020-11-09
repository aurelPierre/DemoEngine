#pragma once

#include "imgui.h"

#include "glm/vec4.hpp"

#include <string>
#include <vector>
#include <ctime>

namespace ez
{
	typedef unsigned int LogFlags;

	enum __LogFlags
	{
		NONE		= 0,
		DEBUG		= 1 << 0,
		INFO		= 1 << 1,
		WARNING		= 1 << 2,
		ASSERT		= 1 << 3,
		ERROR		= 1 << 4,
		CRITICAL	= 1 << 5,
		ALL			= DEBUG | INFO | WARNING | ASSERT | ERROR | CRITICAL
	};

	struct Log
	{
		const std::time_t		_logTimestamp;
		const LogFlags			_logType;
		const std::string		_logText;

		constexpr glm::vec4		Color();
		constexpr const char*	Header();
		std::string	Text();

		Log(std::time_t timestamp, LogFlags type, std::string str) : _logTimestamp{ timestamp }, _logType { type }, _logText{ str } {}
	};

	class LogSystem
	{
		static std::vector<Log>		_buffer;
		static ImGuiTextFilter		_filter;
		static bool					_autoScroll;
		static LogFlags				_enabledType;

	public:
		static void Clear();
		static void AddLog(const LogFlags& type, const std::string& log);
		static void Draw(bool* p_open = nullptr);
		static void	Save();
	};
}

#define LOG(LEVEL, TEXT) ez::LogSystem::AddLog(LEVEL, TEXT);