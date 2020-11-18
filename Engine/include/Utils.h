#pragma once

#include <chrono>
#include <unordered_map>

namespace ez
{
	const char* GetReadableBytes(uint64_t bytes);

	class Timer final
	{
		std::chrono::high_resolution_clock::time_point _startTimestamp;
		std::chrono::high_resolution_clock::time_point _endTimestamp;

	public:
		void Start();
		void Stop();

		template<typename T>
		float Duration() const;
	};

	class ProfileCollector final
	{
		std::string _name;
		Timer		_timer;

	public:
		~ProfileCollector();
		ProfileCollector(std::string name);
	};

	class ProfileSystem final
	{
		static std::unordered_map<std::string, float> _data;

	public:
		static void Register(std::string, float);
		static void Draw();
	};
}

#include "Utils.inl"

#define TRACE(NAME) const ez::ProfileCollector __collector { NAME };