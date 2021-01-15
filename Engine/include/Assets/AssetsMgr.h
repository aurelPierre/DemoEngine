#pragma once

#include <unordered_map>

template<typename T>
class AssetsMgr
{
	typedef T AssetType;

	static AssetsMgr<AssetType>* _instance;

public:
	template<typename... Args>
	static void load(std::string key, Args&&... parameters)
	{
		_instance->_data.try_emplace(key, std::forward<Args>(parameters)...);
	}

	static T& get(std::string key)
	{
		return _instance->_data.at(key);
	}

	static void unload(std::string key)
	{
		_instance->_data.erase(key);
	}

private:
	std::unordered_map<std::string, AssetType> _data;

public:
	AssetsMgr() 
	{
		ASSERT(_instance == nullptr, "instance is already set")
		_instance = this;
	}
};

template<typename T>
AssetsMgr<T>* AssetsMgr<T>::_instance = nullptr;