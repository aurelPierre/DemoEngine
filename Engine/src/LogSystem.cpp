#include "LogSystem.h"

#include <iomanip>
#include <sstream>
#include <chrono>

#include <iostream>
#include <fstream>
#include <ctime>

#include "Utils.h"

namespace ez
{
	std::vector<Log>	LogSystem::_buffer;
	ImGuiTextFilter		LogSystem::_filter;
	bool				LogSystem::_autoScroll	= true;
	LogFlags			LogSystem::_enabledType = ALL;

	constexpr glm::vec4 Log::Color()
	{
		switch (_logType)
		{
		case DEBUG:
			return { 0.f, 1.f, 0.f, 1.f };
		case INFO:
			return { 0.f, 1.f, 0.5f, 1.f };
		case WARNING:
			return { 1.f, 0.5f, 0.f, 1.f };
		case ASSERT:
			return { 1.f, 1.f, 0.f, 1.f };
		case ERROR:
			return { 1.f, 0.25f, 0.25f, 1.f };
		case CRITICAL:
			return { 1.f, 0.f, 0.f, 1.f };
		default:
			break;
		}
		return { 1.f, 1.f, 1.f, 1.f };
	}

	constexpr const char* Log::Header()
	{
		switch (_logType)
		{
		case DEBUG:
			return "[debug]";
		case INFO:
			return "[info]";
		case WARNING:
			return "[warning]";
		case ASSERT:
			return "[assert]";
		case ERROR:
			return "[error]";
		case CRITICAL:
			return "[critical]";
		default:
			break;
		}
		return "";
	}

	std::string Log::Text()
	{
		std::stringstream ss;
		struct tm newtime;
		localtime_s(&newtime, &_logTimestamp);
		ss << std::put_time(&newtime, "%T");
		return ss.str() + ' ' + Header() + ' ' + _logText;
	}

	void    LogSystem::Clear()
	{
		_buffer.clear();
	}

	void    LogSystem::AddLog(const LogFlags& type, const std::string& log)
	{
		_buffer.emplace_back(std::time(nullptr), type, log);
	}

	void    LogSystem::Draw(bool* p_open)
	{
		TRACE("LogSystem::Draw")

		if (!ImGui::Begin("Logs"))
		{
			ImGui::End();
			return;
		}

		// Options menu
		if (ImGui::BeginPopup("Show types"))
		{
			ImGui::CheckboxFlags("DEBUG",		&_enabledType, DEBUG);
			ImGui::CheckboxFlags("INFO",		&_enabledType, INFO);
			ImGui::CheckboxFlags("WARNING",		&_enabledType, WARNING);
			ImGui::CheckboxFlags("ASSERT",		&_enabledType, ASSERT);
			ImGui::CheckboxFlags("ERROR",		&_enabledType, ERROR);
			ImGui::CheckboxFlags("CRITICAL",	&_enabledType, CRITICAL);
			ImGui::EndPopup();
		}

		// Main window
		if (ImGui::Button("Show types"))
			ImGui::OpenPopup("Show types");
		ImGui::SameLine();
		bool clear = ImGui::Button("Clear");
		ImGui::SameLine();
		bool copy = ImGui::Button("Copy");
		ImGui::SameLine();
		_filter.Draw("Filter", -100.0f);

		ImGui::Separator();
		ImGui::BeginChild("scrolling", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);

		if (clear)
			Clear();
		if (copy)
			ImGui::LogToClipboard();

		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 5));

		ImGui::PushTextWrapPos();

		//const char* buf = Buf.begin();
		//const char* buf_end = Buf.end();
		if (_filter.IsActive())
		{
			// In this example we don't use the clipper when Filter is enabled.
			// This is because we don't have a random access on the result on our filter.
			// A real application processing logs with ten of thousands of entries may want to store the result of search/filter.
			// especially if the filtering function is not trivial (e.g. reg-exp).
			/*for (int line_no = 0; line_no < LineOffsets.Size; line_no++)
			{
				const char* line_start = buf + LineOffsets[line_no];
				const char* line_end = (line_no + 1 < LineOffsets.Size) ? (buf + LineOffsets[line_no + 1] - 1) : buf_end;
				if (Filter.PassFilter(line_start, line_end))
					ImGui::TextUnformatted(line_start, line_end);
			}*/

			for (size_t i = 0; i < _buffer.size(); ++i)
			{
				if (_filter.PassFilter(_buffer[i]._logText.c_str(), _buffer[i]._logText.c_str() + _buffer[i]._logText.size()))
				{
					auto c = _buffer[i].Color();
					ImGui::TextColored({c.x, c.y, c.z, c.w }, "%s", _buffer[i].Text().c_str());
				}
			}
		}
		else
		{
			for (size_t i = 0; i < _buffer.size(); ++i)
			{
				if (_enabledType & _buffer[i]._logType)
				{
					auto c = _buffer[i].Color();
					ImGui::TextColored({ c.x, c.y, c.z, c.w }, "%s", _buffer[i].Text().c_str());
				}
			}

			// The simplest and easy way to display the entire buffer:
			//   ImGui::TextUnformatted(buf_begin, buf_end);
			// And it'll just work. TextUnformatted() has specialization for large blob of text and will fast-forward to skip non-visible lines.
			// Here we instead demonstrate using the clipper to only process lines that are within the visible area.
			// If you have tens of thousands of items and their processing cost is non-negligible, coarse clipping them on your side is recommended.
			// Using ImGuiListClipper requires A) random access into your data, and B) items all being the  same height,
			// both of which we can handle since we an array pointing to the beginning of each line of text.
			// When using the filter (in the block of code above) we don't have random access into the data to display anymore, which is why we don't use the clipper.
			// Storing or skimming through the search result would make it possible (and would be recommended if you want to search through tens of thousands of entries)
			/*ImGuiListClipper clipper;
			clipper.Begin(_buffer.size());
			while (clipper.Step())
			{
				for (int line_no = clipper.DisplayStart; line_no < clipper.DisplayEnd; line_no++)
				{
					ImGui::TextColored(_buffer[line_no].SColor(), _buffer[line_no]._logText.c_str());
				}
			}
			clipper.End();*/
		}
		ImGui::PopStyleVar();


		ImGui::PopTextWrapPos();

		if (_autoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
			ImGui::SetScrollHereY(1.0f);

		ImGui::EndChild();
		ImGui::End();
	}

	void	LogSystem::Save()
	{
		if (_buffer.empty())
			return;

		std::time_t otime = time(0);
		struct tm newtime;
		localtime_s(&newtime, &otime);
		
		std::stringstream ss;
		ss << std::put_time(&newtime, "%Y_%m_%d_%H_%M_%S");

		std::ofstream file;
		file.open(ss.str() + ".log");
		if (file.is_open())
		{
			for (size_t i = 0; i < _buffer.size(); ++i)
				file << _buffer[i].Text() << '\n';

			file.flush();
		}
		file.close();
	}
}
