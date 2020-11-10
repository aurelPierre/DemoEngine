#include "Utils.h"

#include "imgui.h"
#include "imgui_internal.h"

#include <string>

namespace ez
{
	const char* GetReadableBytes(uint64_t bytes)
	{
		const char suffix[5][3] = { "B", "KB", "MB", "GB", "TB" };
		char length = sizeof(suffix) / sizeof(suffix[0]);

		int i = 0;
		double dblBytes = bytes;

		if (bytes > 1024) {
			for (i = 0; (bytes / 1024) > 0 && i < length - 1; i++, bytes /= 1024)
				dblBytes = bytes / 1024.0;
		}

		static char output[200];
		sprintf(output, "%.02lf %s", dblBytes, suffix[i]);
		return output;
	}

	void Timer::Start()
	{
		_startTimestamp = std::chrono::high_resolution_clock::now();
		_endTimestamp	= _startTimestamp;
	}

	void Timer::Stop()
	{
		_endTimestamp = std::chrono::high_resolution_clock::now();
	}

	ProfileCollector::~ProfileCollector()
	{
		_timer.Stop();
		ProfileSystem::Register(_name, _timer.Duration<std::milli>());
	}

	ProfileCollector::ProfileCollector(std::string name)
		: _name { name }
	{
		_timer.Start();
	}

	std::unordered_map<std::string, float> ProfileSystem::_data;

	void ProfileSystem::Register(std::string name, float time)
	{
		_data[name] += time;
	}

	void ProfileSystem::Draw()
	{
		TRACE("ProfileSystem::Draw")

		if (!ImGui::Begin("Profiler"))
		{
			ImGui::End();
			return;
		}
		ImGui::Columns(2);

		// Header
		ImGui::Separator();
		ImGui::Text("Name"); ImGui::NextColumn();
		ImGui::Text("Time (ms)"); ImGui::NextColumn();
		ImGui::Separator();

		// Data
		for (auto it = _data.cbegin(); it != _data.cend(); ++it)
		{
			ImGui::Text("%s", it->first.c_str());
			ImGui::NextColumn();

			ImGui::Text("%s", std::to_string(it->second).c_str());
			ImGui::NextColumn();
		}

		ImGui::End();

		_data.clear();
	}
}